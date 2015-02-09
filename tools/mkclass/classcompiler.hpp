/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef CLASSCOMPILER_H
#define CLASSCOMPILER_H

#include <string>
#include <istream>
#include <vector>
#include <algorithm>

namespace icinga
{

struct ClassDebugInfo
{
	std::string path;
	int first_line;
	int first_column;
	int last_line;
	int last_column;
};

enum FieldAccessorType
{
	FTGet,
	FTSet,
	FTDefault
};

struct FieldAccessor
{
	FieldAccessorType Type;
	std::string Accessor;
	bool Pure;

	FieldAccessor(FieldAccessorType type, const std::string& accessor, bool pure)
		: Type(type), Accessor(accessor), Pure(pure)
	{ }
};

enum FieldAttribute
{
	FAConfig = 1,
	FAState = 2,
	FAEnum = 4,
	FAGetProtected = 8,
	FASetProtected = 16,
	FAInternal = 32,
	FANoStorage = 64
};

struct Field
{
	int Attributes;
	std::string Type;
	std::string Name;
	std::string AlternativeName;
	std::string GetAccessor;
	bool PureGetAccessor;
	std::string SetAccessor;
	bool PureSetAccessor;
	std::string DefaultAccessor;

	std::string GetFriendlyName(void) const
	{
		if (!AlternativeName.empty())
			return AlternativeName;

		bool cap = true;
		std::string name = Name;

		for (size_t i = 0; i < name.size(); i++) {
			if (name[i] == '_') {
				cap = true;
				continue;
			}

			if (cap) {
				name[i] = toupper(name[i]);
				cap = false;
			}
		}

		name.erase(
			std::remove(name.begin(), name.end(), '_'),
			name.end()
			);

		/* TODO: figure out name */
		return name;
	}
};

enum TypeAttribute
{
	TAAbstract = 1
};

struct Klass
{
	std::string Name;
	std::string Parent;
	std::string TypeBase;
	int Attributes;
	std::vector<Field> Fields;
};

class ClassCompiler
{
public:
	ClassCompiler(const std::string& path, std::istream *input);
	~ClassCompiler(void);

	void Compile(void);

	std::string GetPath(void) const;

	void InitializeScanner(void);
	void DestroyScanner(void);

	void *GetScanner(void);

	size_t ReadInput(char *buffer, size_t max_size);

	void HandleInclude(const std::string& path, const ClassDebugInfo& locp);
	void HandleAngleInclude(const std::string& path, const ClassDebugInfo& locp);
	void HandleClass(const Klass& klass, const ClassDebugInfo& locp);
	void HandleNamespaceBegin(const std::string& name, const ClassDebugInfo& locp);
	void HandleNamespaceEnd(const ClassDebugInfo& locp);
	void HandleCode(const std::string& code, const ClassDebugInfo& locp);

	static void CompileFile(const std::string& path);
	static void CompileStream(const std::string& path, std::istream *stream);

	static void OptimizeStructLayout(std::vector<Field>& fields);

private:
	std::string m_Path;
	std::istream *m_Input;
	void *m_Scanner;

	static unsigned long SDBM(const std::string& str, size_t len);
	static std::string BaseName(const std::string& path);
	static std::string FileNameToGuardName(const std::string& path);
};

}

#endif /* CLASSCOMPILER_H */

