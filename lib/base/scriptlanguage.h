/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef SCRIPTLANGUAGE_H
#define SCRIPTLANGUAGE_H

#include "base/i2-base.h"
#include "base/scriptinterpreter.h"
#include <map>

namespace icinga
{

/**
 * A scripting language.
 *
 * @ingroup base
 */
class I2_BASE_API ScriptLanguage : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ScriptLanguage);

	static void Register(const String& name, const ScriptLanguage::Ptr& language);
	static void Unregister(const String& name);
	static ScriptLanguage::Ptr GetByName(const String& name);

	virtual ScriptInterpreter::Ptr CreateInterpreter(const Script::Ptr& script) = 0;

	void SubscribeFunction(const String& name);
	void UnsubscribeFunction(const String& name);

protected:
	ScriptLanguage(void);

private:
	static boost::mutex& GetMutex(void);
	static std::map<String, ScriptLanguage::Ptr>& GetLanguages(void);
};

/**
 * Helper class for registering ScriptLanguage implementation classes.
 *
 * @ingroup base
 */
class RegisterLanguageHelper
{
public:
	RegisterLanguageHelper(const String& name, const ScriptLanguage::Ptr& language)
	{
		if (!ScriptLanguage::GetByName(name))
			ScriptLanguage::Register(name, language);
	}
};

#define REGISTER_SCRIPTLANGUAGE(name, klass) \
	static RegisterLanguageHelper g_RegisterSL_ ## type(name, boost::make_shared<klass>())

}

#endif /* SCRIPTLANGUAGE_H */
