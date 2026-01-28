// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef USER_H
#define USER_H

#include "icinga/i2-icinga.hpp"
#include "icinga/user-ti.hpp"
#include "icinga/timeperiod.hpp"
#include "remote/messageorigin.hpp"

namespace icinga
{

/**
 * A User.
 *
 * @ingroup icinga
 */
class User final : public ObjectImpl<User>
{
public:
	DECLARE_OBJECT(User);
	DECLARE_OBJECTNAME(User);

	User();

	void AddGroup(const String& name);

	/* Notifications */
	TimePeriod::Ptr GetPeriod() const;

	Array::Ptr GetTypes() const override;
	void SetTypes(const Array::Ptr& value, bool suppress_events, const Value& cookie) override;

	Array::Ptr GetStates() const override;
	void SetStates(const Array::Ptr& value, bool suppress_events, const Value& cookie) override;

	void ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;
	void ValidateTypes(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;

protected:
	void Stop(bool runtimeRemoved) override;

	void OnAllConfigLoaded() override;
private:
	mutable std::mutex m_UserMutex;
	// These attributes represent the actual User "types" and "states" attributes from the "user.ti".
	// However, since we want to ensure that the type and state bitsets are always in sync with those attributes,
	// we need to override their setters, and this on the hand introduces another problem: The virtual setters are
	// called from within the ObjectImpl<User> constructor, which obviously violates the C++ standard [^1].
	// So, in order to avoid al this kind of mess, these two attributes have the "no_storage" flag set, and
	// their getters/setters are pure virtual, which means this class has to provide the implementation of them.
	//
	// [^1]: https://isocpp.org/wiki/faq/strange-inheritance#calling-virtuals-from-ctors
	AtomicOrLocked<Array::Ptr> m_Types;
	AtomicOrLocked<Array::Ptr> m_States;
};

}

#endif /* USER_H */
