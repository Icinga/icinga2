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

#include "icinga/service.hpp"
#include "remote/configobjectutility.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"

using namespace icinga;


void Checkable::RemoveAllComments()
{
	for (const Comment::Ptr& comment : GetComments()) {
		Comment::RemoveComment(comment->GetName());
	}
}

void Checkable::RemoveCommentsByType(int type)
{
	for (const Comment::Ptr& comment : GetComments()) {
		/* Do not remove persistent comments from an acknowledgement */
		if (comment->GetEntryType() == CommentAcknowledgement && comment->GetPersistent())
			continue;

		if (comment->GetEntryType() == type)
			Comment::RemoveComment(comment->GetName());
	}
}

std::set<Comment::Ptr> Checkable::GetComments() const
{
	boost::mutex::scoped_lock lock(m_CommentMutex);
	return m_Comments;
}

void Checkable::RegisterComment(const Comment::Ptr& comment)
{
	boost::mutex::scoped_lock lock(m_CommentMutex);
	m_Comments.insert(comment);
}

void Checkable::UnregisterComment(const Comment::Ptr& comment)
{
	boost::mutex::scoped_lock lock(m_CommentMutex);
	m_Comments.erase(comment);
}
