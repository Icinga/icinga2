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
map<int, String> Service::m_LegacyCommentsCache;
map<String, Service::WeakPtr> Service::m_CommentsCache;
bool Service::m_CommentsCacheValid;
Timer::Ptr Service::m_CommentsExpireTimer;

int Service::GetNextCommentID(void)
{
	return m_NextCommentID;
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
	comment->Set("legacy_id", m_NextCommentID++);

	Dictionary::Ptr comments = m_Comments;

	if (!comments)
		comments = boost::make_shared<Dictionary>();

	String id = Utility::NewUUID();
	comments->Set(id, comment);

	m_Comments = comments;
	Touch("comments");

	return id;
}

void Service::RemoveAllComments(void)
{
	Set("comments", Empty);
}

void Service::RemoveComment(const String& id)
{
	Service::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		return;

	Dictionary::Ptr comments = owner->m_Comments;

	if (comments) {
		comments->Remove(id);
		owner->Touch("comments");
	}
}

String Service::GetCommentIDFromLegacyID(int id)
{
	map<int, String>::iterator it = m_LegacyCommentsCache.find(id);

	if (it == m_LegacyCommentsCache.end())
		return Empty;

	return it->second;
}

Service::Ptr Service::GetOwnerByCommentID(const String& id)
{
	ValidateCommentsCache();

	return m_CommentsCache[id].lock();
}

Dictionary::Ptr Service::GetCommentByID(const String& id)
{
	Service::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		return Dictionary::Ptr();

	Dictionary::Ptr comments = owner->m_Comments;

	if (comments) {
		Dictionary::Ptr comment = comments->Get(id);
		return comment;
	}

	return Dictionary::Ptr();
}

bool Service::IsCommentExpired(const Dictionary::Ptr& comment)
{
	double expire_time = comment->Get("expire_time");

	return (expire_time != 0 && expire_time < Utility::GetTime());
}

void Service::InvalidateCommentsCache(void)
{
	m_CommentsCacheValid = false;
	m_CommentsCache.clear();
	m_LegacyCommentsCache.clear();
}

void Service::AddCommentsToCache(void)
{
	Dictionary::Ptr comments = m_Comments;

	if (!comments)
		return;

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(tie(id, comment), comments) {
		int legacy_id = comment->Get("legacy_id");

		if (legacy_id >= m_NextCommentID)
			m_NextCommentID = legacy_id + 1;

		if (m_LegacyCommentsCache.find(legacy_id) != m_LegacyCommentsCache.end()) {
			/* The legacy_id is already in use by another comment;
			 * this shouldn't usually happen - assign it a new ID */

			legacy_id = m_NextCommentID++;
			comment->Set("legacy_id", legacy_id);
			Touch("comments");
		}

		m_LegacyCommentsCache[legacy_id] = id;
		m_CommentsCache[id] = GetSelf();
	}
}

void Service::ValidateCommentsCache(void)
{
	if (m_CommentsCacheValid)
		return;

	m_CommentsCache.clear();
	m_LegacyCommentsCache.clear();

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);
		service->AddCommentsToCache();
	}

	m_CommentsCacheValid = true;

	if (!m_CommentsExpireTimer) {
		m_CommentsExpireTimer = boost::make_shared<Timer>();
		m_CommentsExpireTimer->SetInterval(300);
		m_CommentsExpireTimer->OnTimerExpired.connect(boost::bind(&Service::CommentsExpireTimerHandler));
		m_CommentsExpireTimer->Start();
	}
}

void Service::RemoveExpiredComments(void)
{
	Dictionary::Ptr comments = m_Comments;

	if (!comments)
		return;

	vector<String> expiredComments;

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(tie(id, comment), comments) {
		if (IsCommentExpired(comment))
			expiredComments.push_back(id);
	}

	if (expiredComments.size() > 0) {
		BOOST_FOREACH(id, expiredComments) {
			comments->Remove(id);
		}

		Touch("comments");
	}
}

void Service::CommentsExpireTimerHandler(void)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);
		ObjectLock olock(service);
		service->RemoveExpiredComments();
	}
}
