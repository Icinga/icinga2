/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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

void Checkable::RemoveCommentsByType(int type, const String& removedBy)
{
	for (const Comment::Ptr& comment : GetComments()) {
		/* Do not remove persistent comments from an acknowledgement */
		if (comment->GetEntryType() == CommentAcknowledgement && comment->GetPersistent())
			continue;

		if (comment->GetEntryType() == type) {
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
