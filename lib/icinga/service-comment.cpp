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
map<int, String> Service::m_LegacyCommentCache;
map<String, Service::WeakPtr> Service::m_CommentCache;
bool Service::m_CommentCacheValid;
Timer::Ptr Service::m_CommentExpireTimer;

int Service::GetNextCommentID(void)
{
	return m_NextCommentID;
}

Dictionary::Ptr Service::GetComments(void) const
{
	Service::ValidateCommentCache();

	return Get("comments");
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

	Dictionary::Ptr comments = Get("comments");

	if (!comments)
		comments = boost::make_shared<Dictionary>();

	String id = Utility::NewUUID();
	comments->Set(id, comment);
	Set("comments", comments);

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

	Dictionary::Ptr comments = owner->Get("comments");

	if (comments) {
		comments->Remove(id);
		owner->Touch("comments");
	}
}

String Service::GetCommentIDFromLegacyID(int id)
{
	map<int, String>::iterator it = m_LegacyCommentCache.find(id);

	if (it == m_LegacyCommentCache.end())
		return Empty;

	return it->second;
}

Service::Ptr Service::GetOwnerByCommentID(const String& id)
{
	ValidateCommentCache();

	return m_CommentCache[id].lock();
}

Dictionary::Ptr Service::GetCommentByID(const String& id)
{
	Service::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		return Dictionary::Ptr();

	Dictionary::Ptr comments = owner->Get("comments");

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

void Service::InvalidateCommentCache(void)
{
	m_CommentCacheValid = false;
	m_CommentCache.clear();
	m_LegacyCommentCache.clear();
}

void Service::AddCommentsToCache(void)
{
	Dictionary::Ptr comments = Get("comments");

	if (!comments)
		return;

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(tie(id, comment), comments) {
		int legacy_id = comment->Get("legacy_id");

		if (legacy_id >= m_NextCommentID)
			m_NextCommentID = legacy_id + 1;

		if (m_LegacyCommentCache.find(legacy_id) != m_LegacyCommentCache.end()) {
			/* The legacy_id is already in use by another comment;
			 * this shouldn't usually happen - assign it a new ID */

			legacy_id = m_NextCommentID++;
			comment->Set("legacy_id", legacy_id);
			Touch("comments");
		}

		m_LegacyCommentCache[legacy_id] = id;
		m_CommentCache[id] = GetSelf();
	}
}

void Service::ValidateCommentCache(void)
{
	if (m_CommentCacheValid)
		return;

	m_CommentCache.clear();
	m_LegacyCommentCache.clear();

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);
		service->AddCommentsToCache();
	}

	m_CommentCacheValid = true;

	if (!m_CommentExpireTimer) {
		m_CommentExpireTimer = boost::make_shared<Timer>();
		m_CommentExpireTimer->SetInterval(300);
		m_CommentExpireTimer->OnTimerExpired.connect(boost::bind(&Service::CommentExpireTimerHandler));
		m_CommentExpireTimer->Start();
	}
}

void Service::RemoveExpiredComments(void)
{
	Dictionary::Ptr comments = Get("comments");

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

void Service::CommentExpireTimerHandler(void)
{
	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);
		service->RemoveExpiredComments();
	}
}
