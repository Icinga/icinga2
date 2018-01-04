/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "icinga/comment.hpp"
#include "icinga/comment.tcpp"
#include "icinga/host.hpp"
#include "remote/configobjectutility.hpp"
#include "base/utility.hpp"
#include "base/configtype.hpp"
#include "base/timer.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/thread/once.hpp>

using namespace icinga;

static int l_NextCommentID = 1;
static boost::mutex l_CommentMutex;
static std::map<int, String> l_LegacyCommentsCache;
static Timer::Ptr l_CommentsExpireTimer;

boost::signals2::signal<void (const Comment::Ptr&)> Comment::OnCommentAdded;
boost::signals2::signal<void (const Comment::Ptr&)> Comment::OnCommentRemoved;

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
	std::vector<String> tokens;
	boost::algorithm::split(tokens, name, boost::is_any_of("!"));

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

	if (GetServiceName().IsEmpty())
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

	boost::call_once(once, [this]() {
		l_CommentsExpireTimer = new Timer();
		l_CommentsExpireTimer->SetInterval(60);
		l_CommentsExpireTimer->OnTimerExpired.connect(std::bind(&Comment::CommentsExpireTimerHandler));
		l_CommentsExpireTimer->Start();
	});

	{
		boost::mutex::scoped_lock lock(l_CommentMutex);

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
	boost::mutex::scoped_lock lock(l_CommentMutex);

	return l_NextCommentID;
}

String Comment::AddComment(const Checkable::Ptr& checkable, CommentType entryType, const String& author,
	const String& text, bool persistent, double expireTime, const String& id, const MessageOrigin::Ptr& origin)
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

	if (!ConfigObjectUtility::CreateObject(Comment::TypeInstance, fullName, config, errors)) {
		ObjectLock olock(errors);
		for (const String& error : errors) {
			Log(LogCritical, "Comment", error);
		}

		BOOST_THROW_EXCEPTION(std::runtime_error("Could not create comment."));
	}

	Comment::Ptr comment = Comment::GetByName(fullName);

	if (!comment)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not create comment."));

	Log(LogNotice, "Comment")
		<< "Added comment '" << comment->GetName() << "'.";

	return fullName;
}

void Comment::RemoveComment(const String& id, const MessageOrigin::Ptr& origin)
{
	Comment::Ptr comment = Comment::GetByName(id);

	if (!comment || comment->GetPackage() != "_api")
		return;

	Log(LogNotice, "Comment")
		<< "Removed comment '" << comment->GetName() << "' from object '" << comment->GetCheckable()->GetName() << "'.";

	Array::Ptr errors = new Array();

	if (!ConfigObjectUtility::DeleteObject(comment, false, errors)) {
		ObjectLock olock(errors);
		for (const String& error : errors) {
			Log(LogCritical, "Comment", error);
		}

		BOOST_THROW_EXCEPTION(std::runtime_error("Could not remove comment."));
	}
}

String Comment::GetCommentIDFromLegacyID(int id)
{
	boost::mutex::scoped_lock lock(l_CommentMutex);

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
