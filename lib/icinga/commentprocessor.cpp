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

int CommentProcessor::m_NextCommentID = 1;
map<int, String> CommentProcessor::m_LegacyCommentCache;
map<String, DynamicObject::WeakPtr> CommentProcessor::m_CommentCache;
bool CommentProcessor::m_CommentCacheValid;
Timer::Ptr CommentProcessor::m_CommentExpireTimer;

int CommentProcessor::GetNextCommentID(void)
{
	return m_NextCommentID;
}

String CommentProcessor::AddComment(const DynamicObject::Ptr& owner,
    CommentType entryType, const String& author, const String& text,
    double expireTime)
{
	Dictionary::Ptr comment = boost::make_shared<Dictionary>();
	comment->Set("entry_time", Utility::GetTime());
	comment->Set("entry_type", entryType);
	comment->Set("author", author);
	comment->Set("text", text);
	comment->Set("expire_time", expireTime);
	comment->Set("legacy_id", m_NextCommentID++);

	Dictionary::Ptr comments = owner->Get("comments");

	if (!comments)
		comments = boost::make_shared<Dictionary>();

	String id = Utility::NewUUID();
	comments->Set(id, comment);
	owner->Set("comments", comments);

	return id;
}

void CommentProcessor::RemoveAllComments(const DynamicObject::Ptr& owner)
{
	owner->Set("comments", Empty);
}

void CommentProcessor::RemoveComment(const String& id)
{
	DynamicObject::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		throw_exception(invalid_argument("Comment ID does not exist."));

	Dictionary::Ptr comments = owner->Get("comments");

	if (comments) {
		comments->Remove(id);
		owner->Touch("comments");
	}
}

String CommentProcessor::GetIDFromLegacyID(int id)
{
	map<int, String>::iterator it = m_LegacyCommentCache.find(id);

	if (it == m_LegacyCommentCache.end())
		throw_exception(invalid_argument("Invalid legacy comment ID specified."));

	return it->second;
}

DynamicObject::Ptr CommentProcessor::GetOwnerByCommentID(const String& id)
{
	ValidateCommentCache();

	return m_CommentCache[id].lock();
}

Dictionary::Ptr CommentProcessor::GetCommentByID(const String& id)
{
	DynamicObject::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		throw_exception(invalid_argument("Comment ID does not exist."));

	Dictionary::Ptr comments = owner->Get("comments");

	if (comments) {
		Dictionary::Ptr comment = comments->Get(id);
		return comment;
	}

	return Dictionary::Ptr();
}

bool CommentProcessor::IsCommentExpired(const Dictionary::Ptr& comment)
{
	double expire_time = comment->Get("expire_time");

	return (expire_time != 0 && expire_time < Utility::GetTime());
}

void CommentProcessor::InvalidateCommentCache(void)
{
	m_CommentCacheValid = false;
	m_CommentCache.clear();
	m_LegacyCommentCache.clear();
}

void CommentProcessor::AddCommentsToCache(const DynamicObject::Ptr& owner)
{
	Dictionary::Ptr comments = owner->Get("comments");

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
			owner->Touch("comments");
		}

		m_LegacyCommentCache[legacy_id] = id;
		m_CommentCache[id] = owner;
	}
}

void CommentProcessor::ValidateCommentCache(void)
{
	if (m_CommentCacheValid)
		return;

	m_CommentCache.clear();
	m_LegacyCommentCache.clear();

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Host")->GetObjects()) {
		AddCommentsToCache(object);
	}

	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		AddCommentsToCache(object);
	}

	m_CommentCacheValid = true;

	if (!m_CommentExpireTimer) {
		m_CommentExpireTimer = boost::make_shared<Timer>();
		m_CommentExpireTimer->SetInterval(300);
		m_CommentExpireTimer->OnTimerExpired.connect(boost::bind(&CommentProcessor::CommentExpireTimerHandler));
		m_CommentExpireTimer->Start();
	}
}

void CommentProcessor::RemoveExpiredComments(const DynamicObject::Ptr& owner)
{
	Dictionary::Ptr comments = owner->Get("comments");

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

		owner->Touch("comments");
	}
}

void CommentProcessor::CommentExpireTimerHandler(void)
{
	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Host")->GetObjects()) {
		RemoveExpiredComments(object);
	}

	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		RemoveExpiredComments(object);
	}
}

