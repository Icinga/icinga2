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

#include "classcompiler.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

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

void ClassCompiler::HandleInclude(const std::string& path, const ClassDebugInfo& locp)
{
	std::cout << "#include \"" << path << "\"" << std::endl << std::endl;
}

void ClassCompiler::HandleAngleInclude(const std::string& path, const ClassDebugInfo& locp)
{
	std::cout << "#include <" << path << ">" << std::endl << std::endl;
}

void ClassCompiler::HandleNamespaceBegin(const std::string& name, const ClassDebugInfo& locp)
{
	std::cout << "namespace " << name << std::endl
			  << "{" << std::endl << std::endl;
}

void ClassCompiler::HandleNamespaceEnd(const ClassDebugInfo& locp)
{
	std::cout << "}" << std::endl;
}

void ClassCompiler::HandleCode(const std::string& code, const ClassDebugInfo& locp)
{
	std::cout << code << std::endl;
}

void ClassCompiler::HandleClass(const Klass& klass, const ClassDebugInfo& locp)
{
	std::vector<Field>::const_iterator it;

	/* forward declaration */
	if (klass.Name.find_first_of(':') == std::string::npos)
		std::cout << "class " << klass.Name << ";" << std::endl << std::endl;

	/* TypeImpl */
	std::cout << "template<>" << std::endl
		<< "class TypeImpl<" << klass.Name << ">"
		<< " : public Type, public Singleton<TypeImpl<" << klass.Name << "> >" << std::endl
		<< "{" << std::endl
		<< "public:" << std::endl;

	/* GetBaseType */
	std::cout << "\t" << "virtual Type *GetBaseType(void) const" << std::endl
		<< "\t" << "{" << std::endl;

	std::cout << "\t\t" << "return ";

	if (!klass.Parent.empty())
		std::cout << "Singleton<TypeImpl<" << klass.Parent << "> >::GetInstance()";
	else
		std::cout << "NULL";

	std::cout << ";" << std::endl
			  << "\t" << "}" << std::endl << std::endl;

	/* GetFieldId */
	std::cout << "\t" << "virtual int GetFieldId(const String& name) const" << std::endl
		<< "\t" << "{" << std::endl
		<< "\t\t" << "return StaticGetFieldId(name);" << std::endl
		<< "\t" << "}" << std::endl << std::endl;

	/* StaticGetFieldId */
	std::cout << "\t" << "static int StaticGetFieldId(const String& name)" << std::endl
		<< "\t" << "{" << std::endl
		<< "\t\t" << "int offset = ";

	if (!klass.Parent.empty())
		std::cout << "TypeImpl<" << klass.Parent << ">::StaticGetFieldCount()";
	else
		std::cout << "0";

	std::cout << ";" << std::endl << std::endl;

	int num = 0;
	for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
		std::cout << "\t\t" << "if (name == \"" << it->Name << "\")" << std::endl
			<< "\t\t\t" << "return offset + " << num << ";" << std::endl;
		num++;
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

	std::cout << "\t\t" << "switch (";

	if (!klass.Parent.empty())
		std::cout << "real_id";
	else
		std::cout << "id";

	std::cout << ") {" << std::endl;

	num = 0;
	for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
		std::cout << "\t\t\t" << "case " << num << ":" << std::endl
			<< "\t\t\t\t" << "return Field(" << num << ", \"" << it->Name << "\", " << it->Attributes << ");" << std::endl;
		num++;
	}

	std::cout << "\t\t\t" << "default:" << std::endl
		<< "\t\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
		<< "\t\t" << "}" << std::endl;

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

	std::cout << "};" << std::endl << std::endl;

	/* ObjectImpl */
	std::cout << "template<>" << std::endl
		  << "class ObjectImpl<" << klass.Name << ">"
		  << " : public " << (klass.Parent.empty() ? "Object" : klass.Parent) << std::endl
		  << "{" << std::endl
		  << "public:" << std::endl
		  << "\t" << "DECLARE_PTR_TYPEDEFS(ObjectImpl<" << klass.Name << ">);" << std::endl << std::endl;

	/* GetReflectionType */
	std::cout << "\t" << "virtual const Type *GetReflectionType(void) const" << std::endl
			  << "\t" << "{" << std::endl
			  << "\t\t" << "return TypeImpl<" << klass.Name << ">::GetInstance();" << std::endl
			  << "\t" << "}" << std::endl << std::endl;

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

		num = 0;
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
					  << "\t" << "void Set" << it->GetFriendlyName() << "(const " << it->Type << "& value)" << std::endl
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

void ClassCompiler::CompileStream(const std::string& path, std::istream *stream)
{
	stream->exceptions(std::istream::badbit);

	std::cout << "#include \"base/object.h\"" << std::endl
			  << "#include \"base/type.h\"" << std::endl
			  << "#include \"base/singleton.h\"" << std::endl
			  << "#include \"base/debug.h\"" << std::endl
			  << "#include \"base/value.h\"" << std::endl
			  << "#include \"base/array.h\"" << std::endl
			  << "#include \"base/dictionary.h\"" << std::endl << std::endl
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
}
