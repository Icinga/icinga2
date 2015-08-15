/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/service.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

static int l_NextCommentID = 1;
static boost::mutex l_CommentMutex;
static std::map<int, String> l_LegacyCommentsCache;
static std::map<String, Checkable::Ptr> l_CommentsCache;
static Timer::Ptr l_CommentsExpireTimer;

boost::signals2::signal<void (const Checkable::Ptr&, const Comment::Ptr&, const MessageOrigin::Ptr&)> Checkable::OnCommentAdded;
boost::signals2::signal<void (const Checkable::Ptr&, const Comment::Ptr&, const MessageOrigin::Ptr&)> Checkable::OnCommentRemoved;

int Checkable::GetNextCommentID(void)
{
	boost::mutex::scoped_lock lock(l_CommentMutex);

	return l_NextCommentID;
}

String Checkable::AddComment(CommentType entryType, const String& author,
    const String& text, double expireTime, const String& id, const MessageOrigin::Ptr& origin)
{
	String uid;

	if (id.IsEmpty())
		uid = Utility::NewUniqueID();
	else
		uid = id;

	Comment::Ptr comment = new Comment();
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
		l_CommentsCache[uid] = this;
	}

	OnCommentAdded(this, comment, origin);

	return uid;
}

void Checkable::RemoveAllComments(void)
{
	std::vector<String> ids;
	Dictionary::Ptr comments = GetComments();

	{
		ObjectLock olock(comments);
		BOOST_FOREACH(const Dictionary::Pair& kv, comments) {
			ids.push_back(kv.first);
		}
	}

	BOOST_FOREACH(const String& id, ids) {
		RemoveComment(id);
	}
}

void Checkable::RemoveComment(const String& id, const MessageOrigin::Ptr& origin)
{
	Checkable::Ptr owner = GetOwnerByCommentID(id);

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

	OnCommentRemoved(owner, comment, origin);
}

String Checkable::GetCommentIDFromLegacyID(int id)
{
	boost::mutex::scoped_lock lock(l_CommentMutex);

	std::map<int, String>::iterator it = l_LegacyCommentsCache.find(id);

	if (it == l_LegacyCommentsCache.end())
		return Empty;

	return it->second;
}

Checkable::Ptr Checkable::GetOwnerByCommentID(const String& id)
{
	boost::mutex::scoped_lock lock(l_CommentMutex);

	return l_CommentsCache[id];
}

Comment::Ptr Checkable::GetCommentByID(const String& id)
{
	Checkable::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		return Comment::Ptr();

	Dictionary::Ptr comments = owner->GetComments();

	if (comments)
		return comments->Get(id);

	return Comment::Ptr();
}

void Checkable::AddCommentsToCache(void)
{
#ifdef I2_DEBUG
	Log(LogDebug, "Checkable", "Updating Checkable comments cache.");
#endif /* I2_DEBUG */

	Dictionary::Ptr comments = GetComments();

	ObjectLock olock(comments);

	boost::mutex::scoped_lock lock(l_CommentMutex);

	BOOST_FOREACH(const Dictionary::Pair& kv, comments) {
		Comment::Ptr comment = kv.second;

		int legacy_id = comment->GetLegacyId();

		if (legacy_id >= l_NextCommentID)
			l_NextCommentID = legacy_id + 1;

		l_LegacyCommentsCache[legacy_id] = kv.first;
		l_CommentsCache[kv.first] = this;
	}
}

void Checkable::RemoveCommentsByType(int type)
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

void Checkable::RemoveExpiredComments(void)
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

void Checkable::CommentsExpireTimerHandler(void)
{
	BOOST_FOREACH(const Host::Ptr& host, ConfigType::GetObjectsByType<Host>()) {
		host->RemoveExpiredComments();
	}

	BOOST_FOREACH(const Service::Ptr& service, ConfigType::GetObjectsByType<Service>()) {
		service->RemoveExpiredComments();
	}
}
