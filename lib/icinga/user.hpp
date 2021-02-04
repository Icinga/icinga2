/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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

	void AddGroup(const String& name);

	/* Notifications */
	TimePeriod::Ptr GetPeriod() const;

	void ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;
	void ValidateTypes(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;

protected:
	void Stop(bool runtimeRemoved) override;

	void OnConfigLoaded() override;
	void OnAllConfigLoaded() override;
private:
	mutable std::mutex m_UserMutex;
};

}

#endif /* USER_H */
