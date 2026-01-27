// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/comment.hpp"
#include "icinga/comment-ti.cpp"
#include "icinga/host.hpp"
#include "remote/configobjectutility.hpp"
#include "base/utility.hpp"
#include "base/configtype.hpp"
#include "base/timer.hpp"
#include <boost/thread/once.hpp>

using namespace icinga;

static int l_NextCommentID = 1;
static std::mutex l_CommentMutex;
static std::map<int, String> l_LegacyCommentsCache;
static Timer::Ptr l_CommentsExpireTimer;

boost::signals2::signal<void (const Comment::Ptr&)> Comment::OnCommentAdded;
boost::signals2::signal<void (const Comment::Ptr&)> Comment::OnCommentRemoved;
boost::signals2::signal<void (const Comment::Ptr&, const String&, double, const MessageOrigin::Ptr&)> Comment::OnRemovalInfoChanged;

REGISTER_TYPE(Comment);

String CommentNameComposer::MakeName(const String& shortName, const Object::Ptr& context) const
{
	Comment::Ptr comment = dynamic_pointer_cast<Comment>(context);

	if (!comment)
		return "";

	String name = comment->GetHostName();

	if (!comment->GetServiceName().IsEmpty())
		name += "!" + comment->GetServiceName();

	name += "!" + shortName;

	return name;
}

Dictionary::Ptr CommentNameComposer::ParseName(const String& name) const
{
	std::vector<String> tokens = name.Split("!");

	if (tokens.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid Comment name."));

	Dictionary::Ptr result = new Dictionary();
	result->Set("host_name", tokens[0]);

	if (tokens.size() > 2) {
		result->Set("service_name", tokens[1]);
		result->Set("name", tokens[2]);
	} else {
		result->Set("name", tokens[1]);
	}

	return result;
}

void Comment::OnAllConfigLoaded()
{
	ConfigObject::OnAllConfigLoaded();

	Host::Ptr host = Host::GetByName(GetHostName());

	if (GetServiceName().IsEmpty() || ! host)
		m_Checkable = host;
	else
		m_Checkable = host->GetServiceByShortName(GetServiceName());

	if (!m_Checkable)
		BOOST_THROW_EXCEPTION(ScriptError("Comment '" + GetName() + "' references a host/service which doesn't exist.", GetDebugInfo()));
}

void Comment::Start(bool runtimeCreated)
{
	ObjectImpl<Comment>::Start(runtimeCreated);

	static boost::once_flag once = BOOST_ONCE_INIT;

	boost::call_once(once, [] {
		l_CommentsExpireTimer = Timer::Create();
		l_CommentsExpireTimer->SetInterval(60);
		l_CommentsExpireTimer->OnTimerExpired.connect([](const Timer * const&) { CommentsExpireTimerHandler(); });
		l_CommentsExpireTimer->Start();
	});

	{
		std::unique_lock<std::mutex> lock(l_CommentMutex);

		SetLegacyId(l_NextCommentID);
		l_LegacyCommentsCache[l_NextCommentID] = GetName();
		l_NextCommentID++;
	}

	GetCheckable()->RegisterComment(this);

	if (runtimeCreated)
		OnCommentAdded(this);
}

void Comment::Stop(bool runtimeRemoved)
{
	GetCheckable()->UnregisterComment(this);

	if (runtimeRemoved)
		OnCommentRemoved(this);

	ObjectImpl<Comment>::Stop(runtimeRemoved);
}

Checkable::Ptr Comment::GetCheckable() const
{
	return static_pointer_cast<Checkable>(m_Checkable);
}

bool Comment::IsExpired() const
{
	double expire_time = GetExpireTime();

	return (expire_time != 0 && expire_time < Utility::GetTime());
}

int Comment::GetNextCommentID()
{
	std::unique_lock<std::mutex> lock(l_CommentMutex);

	return l_NextCommentID;
}

Comment::Ptr Comment::AddComment(const Checkable::Ptr& checkable, CommentType entryType, const String& author,
	const String& text, bool persistent, double expireTime, bool sticky, const String& id)
{
	String fullName;

	if (id.IsEmpty())
		fullName = checkable->GetName() + "!" + Utility::NewUniqueID();
	else
		fullName = id;

	Dictionary::Ptr attrs = new Dictionary();

	attrs->Set("author", author);
	attrs->Set("text", text);
	attrs->Set("persistent", persistent);
	attrs->Set("expire_time", expireTime);
	attrs->Set("entry_type", entryType);
	attrs->Set("sticky", sticky);
	attrs->Set("entry_time", Utility::GetTime());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	attrs->Set("host_name", host->GetName());
	if (service)
		attrs->Set("service_name", service->GetShortName());

	String zone = checkable->GetZoneName();

	if (!zone.IsEmpty())
		attrs->Set("zone", zone);

	String config = ConfigObjectUtility::CreateObjectConfig(Comment::TypeInstance, fullName, true, nullptr, attrs);

	Array::Ptr errors = new Array();

	if (!ConfigObjectUtility::CreateObject(Comment::TypeInstance, fullName, config, errors, nullptr)) {
		ObjectLock olock(errors);
		for (String error : errors) {
			Log(LogCritical, "Comment", error);
		}

		BOOST_THROW_EXCEPTION(std::runtime_error("Could not create comment."));
	}

	Comment::Ptr comment = Comment::GetByName(fullName);

	if (!comment)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not create comment."));

	Log(LogNotice, "Comment")
		<< "Added comment '" << comment->GetName() << "'.";

	return comment;
}

void Comment::RemoveComment(const String& id, bool removedManually, const String& removedBy)
{
	Comment::Ptr comment = Comment::GetByName(id);

	if (!comment || comment->GetPackage() != "_api")
		return;

	Log(LogNotice, "Comment")
		<< "Removed comment '" << comment->GetName() << "' from object '" << comment->GetCheckable()->GetName() << "'.";

	if (removedManually) {
		comment->SetRemovalInfo(removedBy, Utility::GetTime());
	}

	Array::Ptr errors = new Array();

	if (!ConfigObjectUtility::DeleteObject(comment, false, errors, nullptr)) {
		ObjectLock olock(errors);
		for (String error : errors) {
			Log(LogCritical, "Comment", error);
		}

		BOOST_THROW_EXCEPTION(std::runtime_error("Could not remove comment."));
	}
}

void Comment::SetRemovalInfo(const String& removedBy, double removeTime, const MessageOrigin::Ptr& origin) {
	{
		ObjectLock olock(this);

		SetRemovedBy(removedBy, false, origin);
		SetRemoveTime(removeTime, false, origin);
	}

	OnRemovalInfoChanged(this, removedBy, removeTime, origin);
}

String Comment::GetCommentIDFromLegacyID(int id)
{
	std::unique_lock<std::mutex> lock(l_CommentMutex);

	auto it = l_LegacyCommentsCache.find(id);

	if (it == l_LegacyCommentsCache.end())
		return Empty;

	return it->second;
}

void Comment::CommentsExpireTimerHandler()
{
	std::vector<Comment::Ptr> comments;

	for (const Comment::Ptr& comment : ConfigType::GetObjectsByType<Comment>()) {
		comments.push_back(comment);
	}

	for (const Comment::Ptr& comment : comments) {
		/* Only remove comments which are activated after daemon start. */
		if (comment->IsActive() && comment->IsExpired()) {
			/* Do not remove persistent comments from an acknowledgement */
			if (comment->GetEntryType() == CommentAcknowledgement && comment->GetPersistent())
				continue;

			RemoveComment(comment->GetName());
		}
	}
}
