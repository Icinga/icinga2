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

class I2_DYN_API ConfigCompiler
{
public:
	typedef function<vector<ConfigItem::Ptr> (const string& include)> HandleIncludeFunc;

	ConfigCompiler(istream *input = &cin,
	    HandleIncludeFunc includeHandler = &ConfigCompiler::HandleFileInclude);
	virtual ~ConfigCompiler(void);

	void Compile(void);

	static vector<ConfigItem::Ptr> CompileStream(istream *stream);
	static vector<ConfigItem::Ptr> CompileFile(const string& filename);
	static vector<ConfigItem::Ptr> CompileText(const string& text);

	static vector<ConfigItem::Ptr> HandleFileInclude(const string& include);

	vector<ConfigItem::Ptr> GetResult(void) const;

	/* internally used methods */
	void HandleInclude(const string& include);
	void AddObject(const ConfigItem::Ptr& object);
	size_t ReadInput(char *buffer, size_t max_bytes);
	void *GetScanner(void) const;

private:
	HandleIncludeFunc m_HandleInclude;
	istream *m_Input;
	void *m_Scanner;
	vector<ConfigItem::Ptr> m_Result;

	void InitializeScanner(void);
	void DestroyScanner(void);
};

}

#endif /* CONFIGCOMPILER_H */
