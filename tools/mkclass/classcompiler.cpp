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

#include "classcompiler.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <map>
#include <vector>
#include <cstring>
#ifndef _WIN32
#include <libgen.h>
#else /* _WIN32 */
#include <shlwapi.h>
#endif /* _WIN32 */

using namespace icinga;

ClassCompiler::ClassCompiler(const std::string& path, std::istream *input)
	: m_Path(path), m_Input(input)
{
	InitializeScanner();
}

ClassCompiler::~ClassCompiler(void)
{
	DestroyScanner();
}

std::string ClassCompiler::GetPath(void) const
{
	return m_Path;
}

void *ClassCompiler::GetScanner(void)
{
	return m_Scanner;
}

size_t ClassCompiler::ReadInput(char *buffer, size_t max_size)
{
	m_Input->read(buffer, max_size);
	return static_cast<size_t>(m_Input->gcount());
}

void ClassCompiler::HandleInclude(const std::string& path, const ClassDebugInfo&)
{
	std::cout << "#include \"" << path << "\"" << std::endl << std::endl;
}

void ClassCompiler::HandleAngleInclude(const std::string& path, const ClassDebugInfo&)
{
	std::cout << "#include <" << path << ">" << std::endl << std::endl;
}

void ClassCompiler::HandleNamespaceBegin(const std::string& name, const ClassDebugInfo&)
{
	std::cout << "namespace " << name << std::endl
			  << "{" << std::endl << std::endl;
}

void ClassCompiler::HandleNamespaceEnd(const ClassDebugInfo&)
{
	std::cout << "}" << std::endl;
}

void ClassCompiler::HandleCode(const std::string& code, const ClassDebugInfo&)
{
	std::cout << code << std::endl;
}

unsigned long ClassCompiler::SDBM(const std::string& str, size_t len = std::string::npos)
{
	unsigned long hash = 0;
	size_t current = 0;

	std::string::const_iterator it;

	for (it = str.begin(); it != str.end(); it++) {
		if (current >= len)
                        break;

		char c = *it;

                hash = c + (hash << 6) + (hash << 16) - hash;

                current++;
        }

        return hash;
}

static int TypePreference(const std::string& type)
{
	if (type == "Value")
		return 0;
	else if (type == "String")
		return 1;
	else if (type == "double")
		return 2;
	else if (type.find("::Ptr") != std::string::npos)
		return 3;
	else if (type == "int")
		return 4;
	else
		return 5;
}

static bool FieldLayoutCmp(const Field& a, const Field& b)
{
	return TypePreference(a.Type) < TypePreference(b.Type);
}

static bool FieldTypeCmp(const Field& a, const Field& b)
{
	return a.Type < b.Type;
}

void ClassCompiler::OptimizeStructLayout(std::vector<Field>& fields)
{
	std::sort(fields.begin(), fields.end(), FieldTypeCmp);
	std::stable_sort(fields.begin(), fields.end(), FieldLayoutCmp);
}

void ClassCompiler::HandleClass(const Klass& klass, const ClassDebugInfo&)
{
	std::vector<Field>::const_iterator it;

	/* forward declaration */
	if (klass.Name.find_first_of(':') == std::string::npos)
		std::cout << "class " << klass.Name << ";" << std::endl << std::endl;

	/* TypeHelper */
	if (klass.Attributes & TAAbstract) {
		std::cout << "template<>" << std::endl
			  << "struct TypeHelper<" << klass.Name << ">" << std::endl
			  << "{" << std::endl
			  << "\t" << "static ObjectFactory GetFactory(void)" << std::endl
			  << "\t" << "{" << std::endl
			  << "\t\t" << "return NULL;" << std::endl
			  << "\t" << "}" << std::endl
			  << "};" << std::endl << std::endl;
	}

	/* TypeImpl */
	std::cout << "template<>" << std::endl
		<< "class TypeImpl<" << klass.Name << ">"
		<< " : public Type";
	
	if (!klass.TypeBase.empty())
		std::cout << ", public " + klass.TypeBase;

	std::cout << std::endl
		<< " {" << std::endl
		<< "public:" << std::endl;

	/* GetName */
	std::cout << "\t" << "virtual String GetName(void) const" << std::endl
		  << "\t" << "{" << std::endl
		  << "\t\t" << "return \"" << klass.Name << "\";" << std::endl
		  << "\t" << "}" << std::endl << std::endl;

	/* GetAttributes */
	std::cout << "\t" << "virtual int GetAttributes(void) const" << std::endl
		  << "\t" << "{" << std::endl
		  << "\t\t" << "return " << klass.Attributes << ";" << std::endl
		  << "\t" << "}" << std::endl << std::endl;

	/* GetBaseType */
	std::cout << "\t" << "virtual Type::Ptr GetBaseType(void) const" << std::endl
		<< "\t" << "{" << std::endl;

	std::cout << "\t\t" << "return ";

	if (!klass.Parent.empty())
		std::cout << "Type::GetByName(\"" << klass.Parent << "\")";
	else
		std::cout << "Type::Ptr()";

	std::cout << ";" << std::endl
			  << "\t" << "}" << std::endl << std::endl;

	/* GetFieldId */
	std::cout << "\t" << "virtual int GetFieldId(const String& name) const" << std::endl
		<< "\t" << "{" << std::endl
		<< "\t\t" << "return StaticGetFieldId(name);" << std::endl
		<< "\t" << "}" << std::endl << std::endl;

	/* StaticGetFieldId */
	std::cout << "\t" << "static int StaticGetFieldId(const String& name)" << std::endl
		<< "\t" << "{" << std::endl;

	if (!klass.Fields.empty()) {
		std::cout << "\t\t" << "int offset = ";

		if (!klass.Parent.empty())
			std::cout << "TypeImpl<" << klass.Parent << ">::StaticGetFieldCount()";
		else
			std::cout << "0";

		std::cout << ";" << std::endl << std::endl;
	}

	std::map<int, std::vector<std::pair<int, std::string> > > jumptable;

	int hlen = 0, collisions = 0;

	do {
		int num = 0;

		hlen++;
		jumptable.clear();
		collisions = 0;

		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			int hash = static_cast<int>(SDBM(it->Name, hlen));
			jumptable[hash].push_back(std::make_pair(num, it->Name));
			num++;

			if (jumptable[hash].size() > 1)
				collisions++;
		}
	} while (collisions >= 5 && hlen < 8);

	if (!klass.Fields.empty()) {
		std::cout << "\t\tswitch (static_cast<int>(Utility::SDBM(name, " << hlen << "))) {" << std::endl;

		std::map<int, std::vector<std::pair<int, std::string> > >::const_iterator itj;

		for (itj = jumptable.begin(); itj != jumptable.end(); itj++) {
			std::cout << "\t\t\tcase " << itj->first << ":" << std::endl;

			std::vector<std::pair<int, std::string> >::const_iterator itf;

			for (itf = itj->second.begin(); itf != itj->second.end(); itf++) {
				std::cout << "\t\t\t\t" << "if (name == \"" << itf->second << "\")" << std::endl
					<< "\t\t\t\t\t" << "return offset + " << itf->first << ";" << std::endl;
			}

			std::cout << std::endl
				  << "\t\t\t\tbreak;" << std::endl;
		}

		std::cout << "\t\t}" << std::endl;
	}

	std::cout << std::endl
		<< "\t\t" << "return ";

	if (!klass.Parent.empty())
		std::cout << "TypeImpl<" << klass.Parent << ">::StaticGetFieldId(name)";
	else
		std::cout << "-1";

	std::cout << ";" << std::endl
		<< "\t" << "}" << std::endl << std::endl;

	/* GetFieldInfo */
	std::cout << "\t" << "virtual Field GetFieldInfo(int id) const" << std::endl
		<< "\t" << "{" << std::endl
		<< "\t\t" << "return StaticGetFieldInfo(id);" << std::endl
		<< "\t" << "}" << std::endl << std::endl;

	/* StaticGetFieldInfo */
	std::cout << "\t" << "static Field StaticGetFieldInfo(int id)" << std::endl
		<< "\t" << "{" << std::endl;

	if (!klass.Parent.empty())
		std::cout << "\t\t" << "int real_id = id - " << "TypeImpl<" << klass.Parent << ">::StaticGetFieldCount();" << std::endl
		<< "\t\t" << "if (real_id < 0) { return " << "TypeImpl<" << klass.Parent << ">::StaticGetFieldInfo(id); }" << std::endl;

	if (klass.Fields.size() > 0) {
		std::cout << "\t\t" << "switch (";

		if (!klass.Parent.empty())
			std::cout << "real_id";
		else
			std::cout << "id";

		std::cout << ") {" << std::endl;

		size_t num = 0;
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::string ftype = it->Type;

			if (ftype == "int" || ftype == "double")
				ftype = "Number";
			else if (ftype == "bool")
				ftype = "Boolean";

			if (ftype.find("::Ptr") != std::string::npos)
				ftype = ftype.substr(0, ftype.size() - strlen("::Ptr"));

			if (it->Attributes & FAEnum)
				ftype = "Number";

			std::cout << "\t\t\t" << "case " << num << ":" << std::endl
				<< "\t\t\t\t" << "return Field(" << num << ", \"" << ftype << "\", \"" << it->Name << "\", " << it->Attributes << ");" << std::endl;
			num++;
		}

		std::cout << "\t\t\t" << "default:" << std::endl
				  << "\t\t";
	}

	std::cout << "\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl;

	if (klass.Fields.size() > 0)
		std::cout << "\t\t" << "}" << std::endl;

	std::cout << "\t" << "}" << std::endl << std::endl;

	/* GetFieldCount */
	std::cout << "\t" << "virtual int GetFieldCount(void) const" << std::endl
		<< "\t" << "{" << std::endl
		<< "\t\t" << "return StaticGetFieldCount();" << std::endl
		<< "\t" << "}" << std::endl << std::endl;

	/* StaticGetFieldCount */
	std::cout << "\t" << "static int StaticGetFieldCount(void)" << std::endl
		<< "\t" << "{" << std::endl
		<< "\t\t" << "return " << klass.Fields.size();

	if (!klass.Parent.empty())
		std::cout << " + " << "TypeImpl<" << klass.Parent << ">::StaticGetFieldCount()";

	std::cout << ";" << std::endl
		<< "\t" << "}" << std::endl << std::endl;

	/* GetFactory */
	std::cout << "\t" << "virtual ObjectFactory GetFactory(void) const" << std::endl
		  << "\t" << "{" << std::endl
		  << "\t\t" << "return TypeHelper<" << klass.Name << ">::GetFactory();" << std::endl
		  << "\t" << "}" << std::endl << std::endl;

	std::cout << "};" << std::endl << std::endl;

	/* ObjectImpl */
	std::cout << "template<>" << std::endl
		  << "class ObjectImpl<" << klass.Name << ">"
		  << " : public " << (klass.Parent.empty() ? "Object" : klass.Parent) << std::endl
		  << "{" << std::endl
		  << "public:" << std::endl
		  << "\t" << "DECLARE_PTR_TYPEDEFS(ObjectImpl<" << klass.Name << ">);" << std::endl;

	if (!klass.Fields.empty()) {
		/* constructor */
		std::cout << "public:" << std::endl
			  << "\t" << "ObjectImpl<" << klass.Name << ">(void)" << std::endl
			  << "\t" << "{" << std::endl;

		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::cout << "\t\t" << "Set" << it->GetFriendlyName() << "(" << "GetDefault" << it->GetFriendlyName() << "());" << std::endl;
		}

		std::cout << "\t" << "}" << std::endl << std::endl;

		/* SetField */
		std::cout << "protected:" << std::endl
				  << "\t" << "virtual void SetField(int id, const Value& value)" << std::endl
				  << "\t" << "{" << std::endl;

		if (!klass.Parent.empty())
			std::cout << "\t\t" << "int real_id = id - TypeImpl<" << klass.Parent << ">::StaticGetFieldCount(); " << std::endl
			          << "\t\t" << "if (real_id < 0) { " << klass.Parent << "::SetField(id, value); return; }" << std::endl;

		std::cout << "\t\t" << "switch (";

		if (!klass.Parent.empty())
			std::cout << "real_id";
		else
			std::cout << "id";

		std::cout << ") {" << std::endl;

		size_t num = 0;
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::cout << "\t\t\t" << "case " << num << ":" << std::endl
					  << "\t\t\t\t" << "Set" << it->GetFriendlyName() << "(";
			
			if (it->Attributes & FAEnum)
				std::cout << "static_cast<" << it->Type << ">(static_cast<int>(";

			std::cout << "value";
			
			if (it->Attributes & FAEnum)
				std::cout << "))";
			
			std::cout << ");" << std::endl
					  << "\t\t\t\t" << "break;" << std::endl;
			num++;
		}

		std::cout << "\t\t\t" << "default:" << std::endl
				  << "\t\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
				  << "\t\t" << "}" << std::endl;

		std::cout << "\t" << "}" << std::endl << std::endl;

		/* GetField */
		std::cout << "protected:" << std::endl
				  << "\t" << "virtual Value GetField(int id) const" << std::endl
				  << "\t" << "{" << std::endl;

		if (!klass.Parent.empty())
			std::cout << "\t\t" << "int real_id = id - TypeImpl<" << klass.Parent << ">::StaticGetFieldCount(); " << std::endl
					  << "\t\t" << "if (real_id < 0) { return " << klass.Parent << "::GetField(id); }" << std::endl;

		std::cout << "\t\t" << "switch (";

		if (!klass.Parent.empty())
			std::cout << "real_id";
		else
			std::cout << "id";

		std::cout << ") {" << std::endl;

		num = 0;
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::cout << "\t\t\t" << "case " << num << ":" << std::endl
					  << "\t\t\t\t" << "return Get" << it->GetFriendlyName() << "();" << std::endl;
			num++;
		}

		std::cout << "\t\t\t" << "default:" << std::endl
				  << "\t\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
				  << "\t\t" << "}" << std::endl;

		std::cout << "\t" << "}" << std::endl << std::endl;

		/* getters */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::string prot;

			if (it->Attributes & FAGetProtected)
				prot = "protected";
			else
				prot = "public";

			std::cout << prot << ":" << std::endl
					  << "\t" << it->Type << " Get" << it->GetFriendlyName() << "(void) const" << std::endl
					  << "\t" << "{" << std::endl;

			if (it->GetAccessor.empty())
				std::cout << "\t\t" << "return m_" << it->GetFriendlyName() << ";" << std::endl;
			else
				std::cout << it->GetAccessor << std::endl;

			std::cout << "\t" << "}" << std::endl << std::endl;
		}

		/* setters */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::string prot;

			if (it->Attributes & FASetProtected)
				prot = "protected";
			else if (it->Attributes & FAConfig)
				prot = "private";
			else
				prot = "public";

			std::cout << prot << ":" << std::endl
					  << "\t" << "void Set" << it->GetFriendlyName() << "(";

			if (it->Type == "bool" || it->Type == "double" || it->Type == "int")
				std::cout << it->Type;
			else
				std::cout << "const " << it->Type << "&";

			std::cout << " value)" << std::endl
					  << "\t" << "{" << std::endl;

			if (it->SetAccessor.empty())
				std::cout << "\t\t" << "m_" << it->GetFriendlyName() << " = value;" << std::endl;
			else
				std::cout << it->SetAccessor << std::endl;

			std::cout << "\t" << "}" << std::endl << std::endl;
		}

		/* default */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::string prot;

			std::cout << "private:" << std::endl
					  << "\t" << it->Type << " GetDefault" << it->GetFriendlyName() << "(void) const" << std::endl
					  << "\t" << "{" << std::endl;

			if (it->DefaultAccessor.empty())
				std::cout << "\t\t" << "return " << it->Type << "();" << std::endl;
			else
				std::cout << it->DefaultAccessor << std::endl;

			std::cout << "\t" << "}" << std::endl;
		}

		/* instance variables */
		std::cout << "private:" << std::endl;

		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::cout << "\t" << it->Type << " m_" << it->GetFriendlyName() << ";" << std::endl;
		}
	}

	if (klass.Name == "DynamicObject")
		std::cout << "\t" << "friend class ConfigItem;" << std::endl;

	if (!klass.TypeBase.empty())
		std::cout << "\t" << "friend class " << klass.TypeBase << ";" << std::endl;

	std::cout << "};" << std::endl << std::endl;
}

void ClassCompiler::CompileFile(const std::string& path)
{
	std::ifstream stream;
	stream.open(path.c_str(), std::ifstream::in);

	if (!stream)
		throw std::invalid_argument("Could not open config file: " + path);

	return CompileStream(path, &stream);
}

std::string ClassCompiler::BaseName(const  std::string& path)
{
	char *dir = strdup(path.c_str());
	std::string result;

	if (dir == NULL)
		throw std::bad_alloc();

#ifndef _WIN32
	result = basename(dir);
#else /* _WIN32 */
	result = PathFindFileName(dir);
#endif /* _WIN32 */

	free(dir);

	return result;
}

std::string ClassCompiler::FileNameToGuardName(const std::string& fname)
{
	std::string result = fname;

	for (int i = 0; i < result.size(); i++) {
		result[i] = toupper(result[i]);

		if (result[i] == '.')
			result[i] = '_';
	}

	return result;
}

void ClassCompiler::CompileStream(const std::string& path, std::istream *stream)
{
	stream->exceptions(std::istream::badbit);

	std::string guard_name = FileNameToGuardName(BaseName(path));

	std::cout << "#ifndef " << guard_name << std::endl
			  << "#define " << guard_name << std::endl << std::endl;

	std::cout << "#include \"base/object.hpp\"" << std::endl
			  << "#include \"base/type.hpp\"" << std::endl
			  << "#include \"base/debug.hpp\"" << std::endl
			  << "#include \"base/value.hpp\"" << std::endl
			  << "#include \"base/array.hpp\"" << std::endl
			  << "#include \"base/dictionary.hpp\"" << std::endl
			  << "#include \"base/utility.hpp\"" << std::endl << std::endl
			  << "#ifdef _MSC_VER" << std::endl
			  << "#pragma warning( push )" << std::endl
			  << "#pragma warning( disable : 4244 )" << std::endl
			  << "#pragma warning( disable : 4800 )" << std::endl
			  << "#endif /* _MSC_VER */" << std::endl << std::endl;

	ClassCompiler ctx(path, stream);
	ctx.Compile();

	std::cout << "#ifdef _MSC_VER" << std::endl
			  << "#pragma warning ( pop )" << std::endl
			  << "#endif /* _MSC_VER */" << std::endl;

	std::cout << "#endif /* " << guard_name << " */" << std::endl;
}
