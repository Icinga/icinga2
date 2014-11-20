/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef SCRIPTSIGNAL_H
#define SCRIPTSIGNAL_H

#include "base/i2-base.hpp"
#include "base/value.hpp"
#include <vector>
#include <boost/function.hpp>

namespace icinga
{

/**
 * A signal that can be subscribed to by scripts.
 *
 * @ingroup base
 */
class I2_BASE_API ScriptSignal : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ScriptSignal);

	typedef boost::function<void (const std::vector<Value>& arguments)> Callback;

	void AddSlot(const Callback& slot);
	void Invoke(const std::vector<Value>& arguments = std::vector<Value>());

	static ScriptSignal::Ptr GetByName(const String& name);
	static void Register(const String& name, const ScriptSignal::Ptr& signal);
	static void Unregister(const String& name);

private:
	std::vector<Callback> m_Slots;
};

#define REGISTER_SCRIPTSIGNAL(name) \
	namespace { namespace UNIQUE_NAME(sig) { namespace sig ## name { \
		void RegisterSignal(void) { \
			ScriptSignal::Ptr sig = new icinga::ScriptSignal(); \
			ScriptSignal::Register(#name, sig); \
		} \
		INITIALIZE_ONCE(RegisterSignal); \
	} } }

}

#endif /* SCRIPTSIGNAL_H */
