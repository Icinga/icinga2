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
#include <boost/foreach.hpp>

using namespace icinga;

static int l_NextCommentID = 1;
static boost::mutex l_CommentMutex;
static std::map<int, String> l_LegacyCommentsCache;
static std::map<String, Service::WeakPtr> l_CommentsCache;
static Timer::Ptr l_CommentsExpireTimer;

boost::signals2::signal<void (const Service::Ptr&, const Comment::Ptr&, const String&)> Service::OnCommentAdded;
boost::signals2::signal<void (const Service::Ptr&, const Comment::Ptr&, const String&)> Service::OnCommentRemoved;

int Service::GetNextCommentID(void)
{
	boost::mutex::scoped_lock lock(l_CommentMutex);

	return l_NextCommentID;
}

String Service::AddComment(CommentType entryType, const String& author,
    const String& text, double expireTime, const String& id, const String& authority)
{
	String uid;

	if (id.IsEmpty())
		uid = Utility::NewUniqueID();
	else
		uid = id;

	Comment::Ptr comment = make_shared<Comment>();
	comment->SetId(uid);;
	comment->SetEntryTime(Utility::GetTime());
	comment->SetEntryType(entryType);
	comment->SetAuthor(author);
	comment->SetText(text);
	comment->SetExpireTime(expireTime);

	int legacy_id;

	{
		boost::mutex::scoped_lock lock(l_CommentMutex);
		legacy_id = l_NextCommentID++;
	}

	comment->SetLegacyId(legacy_id);

	GetComments()->Set(uid, comment);

	{
		boost::mutex::scoped_lock lock(l_CommentMutex);
		l_LegacyCommentsCache[legacy_id] = uid;
		l_CommentsCache[uid] = GetSelf();
	}

	OnCommentAdded(GetSelf(), comment, authority);

	return uid;
}

void Service::RemoveAllComments(void)
{
	std::vector<String> ids;
	Dictionary::Ptr comments = GetComments();

	ObjectLock olock(comments);
	BOOST_FOREACH(const Dictionary::Pair& kv, comments) {
		ids.push_back(kv.first);
	}

	BOOST_FOREACH(const String& id, ids) {
		RemoveComment(id);
	}
}

void Service::RemoveComment(const String& id, const String& authority)
{
	Service::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		return;

	Dictionary::Ptr comments = owner->GetComments();

	ObjectLock olock(owner);

	Comment::Ptr comment = comments->Get(id);

	if (!comment)
		return;

	int legacy_id = comment->GetLegacyId();

	comments->Remove(id);

	{
		boost::mutex::scoped_lock lock(l_CommentMutex);
		l_LegacyCommentsCache.erase(legacy_id);
		l_CommentsCache.erase(id);
	}

	OnCommentRemoved(owner, comment, authority);
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

Comment::Ptr Service::GetCommentByID(const String& id)
{
	Service::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		return Comment::Ptr();

	Dictionary::Ptr comments = owner->GetComments();

	if (comments)
		return comments->Get(id);

	return Comment::Ptr();
}

void Service::AddCommentsToCache(void)
{
#ifdef _DEBUG
	Log(LogDebug, "icinga", "Updating Service comments cache.");
#endif /* _DEBUG */

	Dictionary::Ptr comments = GetComments();

	ObjectLock olock(comments);

	boost::mutex::scoped_lock lock(l_CommentMutex);

	BOOST_FOREACH(const Dictionary::Pair& kv, comments) {
		Comment::Ptr comment = kv.second;

		int legacy_id = comment->GetLegacyId();

		if (legacy_id >= l_NextCommentID)
			l_NextCommentID = legacy_id + 1;

		l_LegacyCommentsCache[legacy_id] = kv.first;
		l_CommentsCache[kv.first] = GetSelf();
	}
}

void Service::RemoveCommentsByType(int type)
{
	Dictionary::Ptr comments = GetComments();

	std::vector<String> removedComments;

	{
		ObjectLock olock(comments);

		BOOST_FOREACH(const Dictionary::Pair& kv, comments) {
			Comment::Ptr comment = kv.second;

			if (comment->GetEntryType() == type)
				removedComments.push_back(kv.first);
		}
	}

	BOOST_FOREACH(const String& id, removedComments) {
		RemoveComment(id);
	}
}

void Service::RemoveExpiredComments(void)
{
	Dictionary::Ptr comments = GetComments();

	std::vector<String> expiredComments;

	{
		ObjectLock olock(comments);

		BOOST_FOREACH(const Dictionary::Pair& kv, comments) {
			Comment::Ptr comment = kv.second;

			if (comment->IsExpired())
				expiredComments.push_back(kv.first);
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
