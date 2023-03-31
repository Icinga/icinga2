/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef COMMENT_H
#define COMMENT_H

#include "icinga/i2-icinga.hpp"
#include "icinga/comment-ti.hpp"
#include "icinga/checkable-ti.hpp"
#include "remote/messageorigin.hpp"

namespace icinga
{

/**
 * A comment.
 *
 * @ingroup icinga
 */
class Comment final : public ObjectImpl<Comment>
{
public:
	DECLARE_OBJECT(Comment);
	DECLARE_OBJECTNAME(Comment);

	static boost::signals2::signal<void (const Comment::Ptr&)> OnCommentAdded;
	static boost::signals2::signal<void (const Comment::Ptr&)> OnCommentRemoved;
	static boost::signals2::signal<void (const Comment::Ptr&, const String&, double, const MessageOrigin::Ptr&)> OnRemovalInfoChanged;

	intrusive_ptr<Checkable> GetCheckable() const;

	bool IsExpired() const;

	void SetRemovalInfo(const String& removedBy, double removeTime, const MessageOrigin::Ptr& origin = nullptr);

	static int GetNextCommentID();

	static Ptr AddComment(const intrusive_ptr<Checkable>& checkable, CommentType entryType,
		const String& author, const String& text, bool persistent, double expireTime, bool sticky = false,
		const String& id = String());

	static void RemoveComment(const String& id, bool removedManually = false, const String& removedBy = "");

	static String GetCommentIDFromLegacyID(int id);

protected:
	void OnAllConfigLoaded() override;
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	ObjectImpl<Checkable>::Ptr m_Checkable;

	static void CommentsExpireTimerHandler();
};

}

#endif /* COMMENT_H */
