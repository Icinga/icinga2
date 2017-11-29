/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef STATSFUNCTION_H
#define STATSFUNCTION_H

#include "base/i2-base.hpp"
#include "base/registry.hpp"
#include "base/value.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"

namespace icinga
{

/**
 * A stats function that can be used to execute a stats task.
 *
 * @ingroup base
 */
class I2_BASE_API StatsFunction : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(StatsFunction);

	typedef std::function<void (const Dictionary::Ptr& status, const Array::Ptr& perfdata)> Callback;

	StatsFunction(const Callback& function);

	void Invoke(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

private:
	Callback m_Callback;
};

/**
 * A registry for script functions.
 *
 * @ingroup base
 */
class I2_BASE_API StatsFunctionRegistry : public Registry<StatsFunctionRegistry, StatsFunction::Ptr>
{
public:
	static StatsFunctionRegistry *GetInstance(void);
};

#define REGISTER_STATSFUNCTION(name, callback) \
	INITIALIZE_ONCE([]() { \
		StatsFunction::Ptr stf = new StatsFunction(callback); \
		StatsFunctionRegistry::GetInstance()->Register(#name, stf); \
	})

}

#endif /* STATSFUNCTION_H */
