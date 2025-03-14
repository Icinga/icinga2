/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef USERGROUP_H
#define USERGROUP_H

#include "icinga/i2-icinga.hpp"
#include "icinga/notification.hpp"
#include "icinga/usergroup-ti.hpp"
#include "icinga/user.hpp"

namespace icinga
{

class ConfigItem;
class Notification;

/**
 * An Icinga user group.
 *
 * @ingroup icinga
 */
class UserGroup final : public ObjectImpl<UserGroup>
{
public:
	DECLARE_OBJECT(UserGroup);
	DECLARE_OBJECTNAME(UserGroup);

	std::set<User::Ptr> GetMembers() const;
	void AddMember(const User::Ptr& user);
	void RemoveMember(const User::Ptr& user);

	std::set<intrusive_ptr<Notification>> GetNotifications() const;
	void AddNotification(const intrusive_ptr<Notification>& notification);
	void RemoveNotification(const intrusive_ptr<Notification>& notification);

	bool ResolveGroupMembership(const User::Ptr& user, bool add = true, int rstack = 0);

	static void EvaluateObjectRules(const User::Ptr& user);

private:
	mutable std::mutex m_UserGroupMutex;
	std::set<User::Ptr> m_Members;
	std::set<intrusive_ptr<Notification>> m_Notifications;

	static bool EvaluateObjectRule(const User::Ptr& user, const intrusive_ptr<ConfigItem>& group);
};

}

#endif /* USERGROUP_H */
