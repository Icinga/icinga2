/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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
static bool l_CommentsCacheNeedsUpdate = false;
static Timer::Ptr l_CommentsCacheTimer;
static Timer::Ptr l_CommentsExpireTimer;

boost::signals2::signal<void (const Service::Ptr&, const String&, CommentChangedType)> Service::OnCommentsChanged;

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
    const String& text, double expireTime)
{
	Dictionary::Ptr comment = boost::make_shared<Dictionary>();
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

	String id = Utility::NewUniqueID();

	{
		ObjectLock olock(this);

		comments->Set(id, comment);
		Touch("comments");
	}

	{
		boost::mutex::scoped_lock lock(l_CommentMutex);
		l_CommentsCache[id] = GetSelf();
	}

	OnCommentsChanged(GetSelf(), id, CommentChangedAdded);

	return id;
}

void Service::RemoveAllComments(void)
{
	OnCommentsChanged(GetSelf(), Empty, CommentChangedDeleted);

	m_Comments = Empty;
	Touch("comments");
}

void Service::RemoveComment(const String& id)
{
	Service::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		return;

	Dictionary::Ptr comments = owner->GetComments();

	if (comments) {
		ObjectLock olock(owner);

		comments->Remove(id);
		owner->Touch("comments");

		OnCommentsChanged(owner, id, CommentChangedDeleted);
	}
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

void Service::InvalidateCommentsCache(void)
{
	boost::mutex::scoped_lock lock(l_CommentMutex);

	if (l_CommentsCacheNeedsUpdate)
		return; /* Someone else has already requested a refresh. */

	if (!l_CommentsCacheTimer) {
		l_CommentsCacheTimer = boost::make_shared<Timer>();
		l_CommentsCacheTimer->SetInterval(0.5);
		l_CommentsCacheTimer->OnTimerExpired.connect(boost::bind(&Service::RefreshCommentsCache));
		l_CommentsCacheTimer->Start();
	}

	l_CommentsCacheNeedsUpdate = true;
}

void Service::RefreshCommentsCache(void)
{
	{
		boost::mutex::scoped_lock lock(l_CommentMutex);

		if (!l_CommentsCacheNeedsUpdate)
			return;

		l_CommentsCacheNeedsUpdate = false;
	}

	Log(LogDebug, "icinga", "Updating Service comments cache.");

	std::map<int, String> newLegacyCommentsCache;
	std::map<String, Service::WeakPtr> newCommentsCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);

		Dictionary::Ptr comments = service->GetComments();

		if (!comments)
			continue;

		ObjectLock olock(comments);

		String id;
		Dictionary::Ptr comment;
		BOOST_FOREACH(tie(id, comment), comments) {
			int legacy_id = comment->Get("legacy_id");

			if (legacy_id >= l_NextCommentID)
				l_NextCommentID = legacy_id + 1;

			if (newLegacyCommentsCache.find(legacy_id) != newLegacyCommentsCache.end()) {
				/* The legacy_id is already in use by another comment;
				 * this shouldn't usually happen - assign it a new ID */

				legacy_id = l_NextCommentID++;
				comment->Set("legacy_id", legacy_id);

				{
					ObjectLock olock(service);
					service->Touch("comments");
				}
			}

			newLegacyCommentsCache[legacy_id] = id;
			newCommentsCache[id] = service;
		}
	}

	boost::mutex::scoped_lock lock(l_CommentMutex);

	l_CommentsCache.swap(newCommentsCache);
	l_LegacyCommentsCache.swap(newLegacyCommentsCache);

	if (!l_CommentsExpireTimer) {
		l_CommentsExpireTimer = boost::make_shared<Timer>();
		l_CommentsExpireTimer->SetInterval(300);
		l_CommentsExpireTimer->OnTimerExpired.connect(boost::bind(&Service::CommentsExpireTimerHandler));
		l_CommentsExpireTimer->Start();
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

	if (!removedComments.empty()) {
		BOOST_FOREACH(const String& id, removedComments) {
			comments->Remove(id);
		}

		ObjectLock olock(this);
		Touch("comments");
	}

	OnCommentsChanged(GetSelf(), Empty, CommentChangedDeleted);
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

	if (!expiredComments.empty()) {
		BOOST_FOREACH(const String& id, expiredComments) {
			comments->Remove(id);
		}

		ObjectLock olock(this);
		Touch("comments");
	}

	OnCommentsChanged(GetSelf(), Empty, CommentChangedDeleted);
}

void Service::CommentsExpireTimerHandler(void)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);
		service->RemoveExpiredComments();
	}
}
