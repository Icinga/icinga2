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

#include "i2-icinga.h"

using namespace icinga;

int Service::m_NextCommentID = 1;
boost::mutex Service::m_CommentMutex;
map<int, String> Service::m_LegacyCommentsCache;
map<String, Service::WeakPtr> Service::m_CommentsCache;
bool Service::m_CommentsCacheNeedsUpdate = false;
Timer::Ptr Service::m_CommentsCacheTimer;
Timer::Ptr Service::m_CommentsExpireTimer;

/**
 * @threadsafety Always.
 */
int Service::GetNextCommentID(void)
{
	boost::mutex::scoped_lock lock(m_CommentMutex);

	return m_NextCommentID;
}

/**
 * @threadsafety Always.
 */
Dictionary::Ptr Service::GetComments(void) const
{
	return m_Comments;
}

/**
 * @threadsafety Always.
 */
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
		boost::mutex::scoped_lock lock(m_CommentMutex);
		legacy_id = m_NextCommentID++;
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

	String id = Utility::NewUUID();
	comments->Set(id, comment);

	Touch("comments");

	return id;
}

/**
 * @threadsafety Always.
 */
void Service::RemoveAllComments(void)
{
	m_Comments = Empty;
	Touch("comments");
}

/**
 * @threadsafety Always.
 */
void Service::RemoveComment(const String& id)
{
	Service::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		return;

	Dictionary::Ptr comments = owner->GetComments();

	if (comments) {
		comments->Remove(id);
		owner->Touch("comments");
	}
}

/**
 * @threadsafety Always.
 */
String Service::GetCommentIDFromLegacyID(int id)
{
	boost::mutex::scoped_lock lock(m_CommentMutex);

	map<int, String>::iterator it = m_LegacyCommentsCache.find(id);

	if (it == m_LegacyCommentsCache.end())
		return Empty;

	return it->second;
}

/**
 * @threadsafety Always.
 */
Service::Ptr Service::GetOwnerByCommentID(const String& id)
{
	boost::mutex::scoped_lock lock(m_CommentMutex);

	return m_CommentsCache[id].lock();
}

/**
 * @threadsafety Always.
 */
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

/**
 * @threadsafety Always.
 */
bool Service::IsCommentExpired(const Dictionary::Ptr& comment)
{
	double expire_time = comment->Get("expire_time");

	return (expire_time != 0 && expire_time < Utility::GetTime());
}

/**
 * @threadsafety Always.
 */
void Service::InvalidateCommentsCache(void)
{
	boost::mutex::scoped_lock lock(m_CommentMutex);

	if (m_CommentsCacheNeedsUpdate)
		return; /* Someone else has already requested a refresh. */

	if (!m_CommentsCacheTimer) {
		m_CommentsCacheTimer = boost::make_shared<Timer>();
		m_CommentsCacheTimer->SetInterval(0.5);
		m_CommentsCacheTimer->OnTimerExpired.connect(boost::bind(&Service::RefreshCommentsCache));
		m_CommentsCacheTimer->Start();
	}

	m_CommentsCacheNeedsUpdate = true;
}

/**
 * @threadsafety Always.
 */
void Service::RefreshCommentsCache(void)
{
	{
		boost::mutex::scoped_lock lock(m_CommentMutex);

		if (!m_CommentsCacheNeedsUpdate)
			return;

		m_CommentsCacheNeedsUpdate = false;
	}

	Logger::Write(LogDebug, "icinga", "Updating Service comments cache.");

	map<int, String> newLegacyCommentsCache;
	map<String, Service::WeakPtr> newCommentsCache;

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

			if (legacy_id >= m_NextCommentID)
				m_NextCommentID = legacy_id + 1;

			if (newLegacyCommentsCache.find(legacy_id) != newLegacyCommentsCache.end()) {
				/* The legacy_id is already in use by another comment;
				 * this shouldn't usually happen - assign it a new ID */

				legacy_id = m_NextCommentID++;
				comment->Set("legacy_id", legacy_id);
				service->Touch("comments");
			}

			newLegacyCommentsCache[legacy_id] = id;
			newCommentsCache[id] = service;
		}
	}

	boost::mutex::scoped_lock lock(m_CommentMutex);

	m_CommentsCache.swap(newCommentsCache);
	m_LegacyCommentsCache.swap(newLegacyCommentsCache);

	if (!m_CommentsExpireTimer) {
		m_CommentsExpireTimer = boost::make_shared<Timer>();
		m_CommentsExpireTimer->SetInterval(300);
		m_CommentsExpireTimer->OnTimerExpired.connect(boost::bind(&Service::CommentsExpireTimerHandler));
		m_CommentsExpireTimer->Start();
	}
}

/**
 * @threadsafety Always.
 */
void Service::RemoveExpiredComments(void)
{
	Dictionary::Ptr comments = GetComments();

	if (!comments)
		return;

	vector<String> expiredComments;

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

		Touch("comments");
	}
}

/**
 * @threadsafety Always.
 */
void Service::CommentsExpireTimerHandler(void)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);
		service->RemoveExpiredComments();
	}
}
