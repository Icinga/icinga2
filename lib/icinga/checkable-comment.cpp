/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/service.hpp"
#include "remote/configobjectutility.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include <utility>

using namespace icinga;


void Checkable::RemoveAllComments()
{
	for (const Comment::Ptr& comment : GetComments()) {
		Comment::RemoveComment(comment->GetName());
	}
}

void Checkable::RemoveAckComments(const String& removedBy, double createdBefore)
{
	for (const Comment::Ptr& comment : GetComments()) {
		if (comment->GetEntryType() == CommentAcknowledgement) {
			/* Do not remove persistent comments from an acknowledgement */
			if (comment->GetPersistent()) {
				continue;
			}

			if (comment->GetEntryTime() > createdBefore) {
				continue;
			}

			{
				ObjectLock oLock (comment);
				comment->SetRemovedBy(removedBy);
			}

			Comment::RemoveComment(comment->GetName());
		}
	}
}

std::set<Comment::Ptr> Checkable::GetComments() const
{
	std::unique_lock<std::mutex> lock(m_CommentMutex);
	return m_Comments;
}

Comment::Ptr Checkable::GetLastComment() const
{
	std::unique_lock<std::mutex> lock (m_CommentMutex);
	Comment::Ptr lastComment;

	for (auto& comment : m_Comments) {
		if (!lastComment || comment->GetEntryTime() > lastComment->GetEntryTime()) {
			lastComment = comment;
		}
	}

	return lastComment;
}

void Checkable::RegisterComment(const Comment::Ptr& comment)
{
	std::unique_lock<std::mutex> lock(m_CommentMutex);
	m_Comments.insert(comment);
}

void Checkable::UnregisterComment(const Comment::Ptr& comment)
{
	std::unique_lock<std::mutex> lock(m_CommentMutex);
	m_Comments.erase(comment);
}
