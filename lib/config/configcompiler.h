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

#ifndef CONFIGCOMPILER_H
#define CONFIGCOMPILER_H

namespace icinga
{

/**
 * The configuration compiler can be used to compile a configuration file
 * into a number of configuration objects.
 *
 * @ingroup config
 */
class I2_CONFIG_API ConfigCompiler
{
public:
	typedef function<vector<ConfigItem::Ptr> (const String& include)> HandleIncludeFunc;

	ConfigCompiler(const String& path, istream *input = &cin,
	    HandleIncludeFunc includeHandler = &ConfigCompiler::HandleFileInclude);
	virtual ~ConfigCompiler(void);

	void Compile(void);

	static vector<ConfigItem::Ptr> CompileStream(const String& path, istream *stream);
	static vector<ConfigItem::Ptr> CompileFile(const String& path);
	static vector<ConfigItem::Ptr> CompileText(const String& path, const String& text);

	static vector<ConfigItem::Ptr> HandleFileInclude(const String& include);

	vector<ConfigItem::Ptr> GetResult(void) const;

	String GetPath(void) const;

	/* internally used methods */
	void HandleInclude(const String& include);
	void AddObject(const ConfigItem::Ptr& object);
	size_t ReadInput(char *buffer, size_t max_bytes);
	void *GetScanner(void) const;

private:
	String m_Path;
	istream *m_Input;

	HandleIncludeFunc m_HandleInclude;

	void *m_Scanner;
	vector<ConfigItem::Ptr> m_Result;

	void InitializeScanner(void);
	void DestroyScanner(void);
};

}

#endif /* CONFIGCOMPILER_H */
