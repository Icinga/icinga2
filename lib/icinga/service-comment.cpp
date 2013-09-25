/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/utility.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

using namespace icinga;

static int l_NextCommentID = 1;
static boost::mutex l_CommentMutex;
static std::map<int, String> l_LegacyCommentsCache;
static std::map<String, Service::WeakPtr> l_CommentsCache;
static Timer::Ptr l_CommentsExpireTimer;

boost::signals2::signal<void (const Service::Ptr&, const Dictionary::Ptr&, const String&)> Service::OnCommentAdded;
boost::signals2::signal<void (const Service::Ptr&, const Dictionary::Ptr&, const String&)> Service::OnCommentRemoved;

int Service::GetNextCommentID(void)
{
	boost::mutex::scoped_lock lock(l_CommentMutex);

	return l_NextCommentID;
}

Dictionary::Ptr Service::GetComments(void) const
{
	return m_Comments;
}

String Service::AddComment(CommentType entryType, const String& author,
    const String& text, double expireTime, const String& id, const String& authority)
{
	String uid;

	if (id.IsEmpty())
		uid = Utility::NewUniqueID();
	else
		uid = id;

	Dictionary::Ptr comment = boost::make_shared<Dictionary>();
	comment->Set("id", uid);
	comment->Set("entry_time", Utility::GetTime());
	comment->Set("entry_type", entryType);
	comment->Set("author", author);
	comment->Set("text", text);
	comment->Set("expire_time", expireTime);

	int legacy_id;

	{
		boost::mutex::scoped_lock lock(l_CommentMutex);
		legacy_id = l_NextCommentID++;
	}

	comment->Set("legacy_id", legacy_id);

	Dictionary::Ptr comments;

	{
		ObjectLock olock(this);

		comments = GetComments();

		if (!comments)
			comments = boost::make_shared<Dictionary>();

		m_Comments = comments;
	}

	{
		ObjectLock olock(this);

		comments->Set(uid, comment);
	}

	{
		boost::mutex::scoped_lock lock(l_CommentMutex);
		l_LegacyCommentsCache[legacy_id] = uid;
		l_CommentsCache[uid] = GetSelf();
	}

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnCommentAdded), GetSelf(), comment, authority));

	return uid;
}

void Service::RemoveAllComments(void)
{
	std::vector<String> ids;
	Dictionary::Ptr comments = m_Comments;

	if (!comments)
		return;

	ObjectLock olock(comments);
	String id;
	BOOST_FOREACH(boost::tie(id, boost::tuples::ignore), comments) {
		ids.push_back(id);
	}

	BOOST_FOREACH(id, ids) {
		RemoveComment(id);
	}
}

void Service::RemoveComment(const String& id, const String& authority)
{
	Service::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		return;

	Dictionary::Ptr comments = owner->GetComments();

	if (!comments)
		return;

	ObjectLock olock(owner);

	Dictionary::Ptr comment = comments->Get(id);

	if (!comment)
		return;

	int legacy_id = comment->Get("legacy_id");

	comments->Remove(id);

	{
		boost::mutex::scoped_lock lock(l_CommentMutex);
		l_LegacyCommentsCache.erase(legacy_id);
		l_CommentsCache.erase(id);
	}

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnCommentRemoved), owner, comment, authority));
}

String Service::GetCommentIDFromLegacyID(int id)
{
	boost::mutex::scoped_lock lock(l_CommentMutex);

	std::map<int, String>::iterator it = l_LegacyCommentsCache.find(id);

	if (it == l_LegacyCommentsCache.end())
		return Empty;

	return it->second;
}

Service::Ptr Service::GetOwnerByCommentID(const String& id)
{
	boost::mutex::scoped_lock lock(l_CommentMutex);

	return l_CommentsCache[id].lock();
}

Dictionary::Ptr Service::GetCommentByID(const String& id)
{
	Service::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		return Dictionary::Ptr();

	Dictionary::Ptr comments = owner->GetComments();

	if (comments)
		return comments->Get(id);

	return Dictionary::Ptr();
}

bool Service::IsCommentExpired(const Dictionary::Ptr& comment)
{
	double expire_time = comment->Get("expire_time");

	return (expire_time != 0 && expire_time < Utility::GetTime());
}

void Service::AddCommentsToCache(void)
{
	Log(LogDebug, "icinga", "Updating Service comments cache.");

	Dictionary::Ptr comments = GetComments();

	if (!comments)
		return;

	ObjectLock olock(comments);

	boost::mutex::scoped_lock lock(l_CommentMutex);

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(tie(id, comment), comments) {
		int legacy_id = comment->Get("legacy_id");

		if (legacy_id >= l_NextCommentID)
			l_NextCommentID = legacy_id + 1;

		l_LegacyCommentsCache[legacy_id] = id;
		l_CommentsCache[id] = GetSelf();
	}
}

void Service::RemoveCommentsByType(int type)
{
	Dictionary::Ptr comments = GetComments();

	if (!comments)
		return;

	std::vector<String> removedComments;

	{
		ObjectLock olock(comments);

		String id;
		Dictionary::Ptr comment;
		BOOST_FOREACH(tie(id, comment), comments) {
			if (comment->Get("entry_type") == type)
				removedComments.push_back(id);
		}
	}

	BOOST_FOREACH(const String& id, removedComments) {
		RemoveComment(id);
	}
}

void Service::RemoveExpiredComments(void)
{
	Dictionary::Ptr comments = GetComments();

	if (!comments)
		return;

	std::vector<String> expiredComments;

	{
		ObjectLock olock(comments);

		String id;
		Dictionary::Ptr comment;
		BOOST_FOREACH(tie(id, comment), comments) {
			if (IsCommentExpired(comment))
				expiredComments.push_back(id);
		}
	}

	BOOST_FOREACH(const String& id, expiredComments) {
		RemoveComment(id);
	}
}

void Service::CommentsExpireTimerHandler(void)
{
	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		service->RemoveExpiredComments();
	}
}
