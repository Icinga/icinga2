/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef CONFIGCOMPILERCONTEXT_H
#define CONFIGCOMPILERCONTEXT_H

namespace icinga
{

/**
 * @ingroup config
 */
enum ConfigCompilerFlag
{
        CompilerStrict = 1, /**< Treat warnings as errors. */
        CompilerLinkExisting = 2 /**< Link objects to existing config items. */
};

struct I2_CONFIG_API ConfigCompilerError
{
	bool Warning;
	String Message;

	ConfigCompilerError(bool warning, const String& message)
		: Warning(warning), Message(message)
	{ }
};

/*
 * @ingroup config
 */
class I2_CONFIG_API ConfigCompilerContext
{
public:
	ConfigCompilerContext(void);

	void AddItem(const ConfigItem::Ptr& item);
	ConfigItem::Ptr GetItem(const String& type, const String& name) const;
	vector<ConfigItem::Ptr> GetItems(void) const;

	void AddType(const ConfigType::Ptr& type);
	ConfigType::Ptr GetType(const String& name) const;

	void AddError(bool warning, const String& message);
	vector<ConfigCompilerError> GetErrors(void) const;

	void SetFlags(int flags);
	int GetFlags(void) const;

	String GetUnit(void) const;

	void Validate(void);
	void ActivateItems(void);

	static void SetContext(ConfigCompilerContext *context);
	static ConfigCompilerContext *GetContext(void);

private:
	String m_Unit;

        int m_Flags;

	vector<shared_ptr<ConfigItem> > m_Items;
        map<pair<String, String>, shared_ptr<ConfigItem> > m_ItemsMap;

        map<String, shared_ptr<ConfigType> > m_Types;

        vector<ConfigCompilerError> m_Errors;

	static ConfigCompilerContext *m_Context;
};

}

#endif /* CONFIGCOMPILERCONTEXT_H */
