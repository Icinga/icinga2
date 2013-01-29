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
map<int, DynamicObject::WeakPtr> CommentProcessor::m_CommentCache;
bool CommentProcessor::m_CommentCacheValid;

int CommentProcessor::GetNextCommentID(void)
{
	return m_NextCommentID;
}

int CommentProcessor::AddComment(const DynamicObject::Ptr& owner,
    CommentType entryType, const String& author, const String& text,
    double expireTime)
{
	Dictionary::Ptr comment = boost::make_shared<Dictionary>();
	comment->Set("entry_time", Utility::GetTime());
	comment->Set("entry_type", entryType);
	comment->Set("author", author);
	comment->Set("text", text);
	comment->Set("expire_time", expireTime);

	Dictionary::Ptr comments = owner->Get("comments");

	if (!comments)
		comments = boost::make_shared<Dictionary>();

	int id = m_NextCommentID;
	m_NextCommentID++;

	comments->Set(Convert::ToString(id), comment);
	owner->Set("comments", comments);

	return id;
}

void CommentProcessor::RemoveAllComments(const DynamicObject::Ptr& owner)
{
	owner->Set("comments", Empty);
}

void CommentProcessor::RemoveComment(int id)
{
	DynamicObject::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		throw_exception(invalid_argument("Comment ID does not exist."));

	Dictionary::Ptr comments = owner->Get("comments");

	if (comments) {
		comments->Remove(Convert::ToString(id));
		owner->Touch("comments");
	}
}

DynamicObject::Ptr CommentProcessor::GetOwnerByCommentID(int id)
{
	ValidateCommentCache();

	return m_CommentCache[id].lock();
}

Dictionary::Ptr CommentProcessor::GetCommentByID(int id)
{
	DynamicObject::Ptr owner = GetOwnerByCommentID(id);

	if (!owner)
		throw_exception(invalid_argument("Comment ID does not exist."));

	Dictionary::Ptr comments = owner->Get("comments");

	if (comments) {
		Dictionary::Ptr comment = comments->Get(Convert::ToString(id));
		return comment;
	}

	return Dictionary::Ptr();
}

void CommentProcessor::InvalidateCommentCache(void)
{
	m_CommentCacheValid = false;
	m_CommentCache.clear();
}

void CommentProcessor::AddCommentsToCache(const DynamicObject::Ptr& owner)
{
	Dictionary::Ptr comments = owner->Get("comments");

	if (!comments)
		return;

	String sid;
	BOOST_FOREACH(tie(sid, tuples::ignore), comments) {
		int id = Convert::ToLong(sid);

		if (id > m_NextCommentID)
			m_NextCommentID = id;

		m_CommentCache[id] = owner;
	}
}

void CommentProcessor::ValidateCommentCache(void)
{
	if (m_CommentCacheValid)
		return;

	m_CommentCache.clear();

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Host")->GetObjects()) {
		AddCommentsToCache(object);
	}

	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		AddCommentsToCache(object);
	}

	m_CommentCacheValid = true;
}

