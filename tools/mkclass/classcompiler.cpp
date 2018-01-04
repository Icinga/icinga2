/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <cstring>
#include <locale>
#ifndef _WIN32
#include <libgen.h>
#else /* _WIN32 */
#include <shlwapi.h>
#endif /* _WIN32 */

using namespace icinga;

ClassCompiler::ClassCompiler(std::string path, std::istream& input,
	std::ostream& oimpl, std::ostream& oheader)
	: m_Path(std::move(path)), m_Input(input), m_Impl(oimpl), m_Header(oheader)
{
	InitializeScanner();
}

ClassCompiler::~ClassCompiler()
{
	DestroyScanner();
}

std::string ClassCompiler::GetPath() const
{
	return m_Path;
}

void *ClassCompiler::GetScanner()
{
	return m_Scanner;
}

size_t ClassCompiler::ReadInput(char *buffer, size_t max_size)
{
	m_Input.read(buffer, max_size);
	return static_cast<size_t>(m_Input.gcount());
}

void ClassCompiler::HandleInclude(const std::string& path, const ClassDebugInfo&)
{
	m_Header << "#include \"" << path << "\"" << std::endl << std::endl;
}

void ClassCompiler::HandleAngleInclude(const std::string& path, const ClassDebugInfo&)
{
	m_Header << "#include <" << path << ">" << std::endl << std::endl;
}

void ClassCompiler::HandleImplInclude(const std::string& path, const ClassDebugInfo&)
{
	m_Impl << "#include \"" << path << "\"" << std::endl << std::endl;
}

void ClassCompiler::HandleAngleImplInclude(const std::string& path, const ClassDebugInfo&)
{
	m_Impl << "#include <" << path << ">" << std::endl << std::endl;
}

void ClassCompiler::HandleNamespaceBegin(const std::string& name, const ClassDebugInfo&)
{
	m_Header << "namespace " << name << std::endl
			<< "{" << std::endl << std::endl;

	m_Impl << "namespace " << name << std::endl
		<< "{" << std::endl << std::endl;
}

void ClassCompiler::HandleNamespaceEnd(const ClassDebugInfo&)
{
	HandleMissingValidators();

	m_Header << "}" << std::endl;

	m_Impl << "}" << std::endl;
}

void ClassCompiler::HandleCode(const std::string& code, const ClassDebugInfo&)
{
	m_Header << code << std::endl;
}

void ClassCompiler::HandleLibrary(const std::string& library, const ClassDebugInfo&)
{
	m_Library = library;
}

unsigned long ClassCompiler::SDBM(const std::string& str, size_t len = std::string::npos)
{
	unsigned long hash = 0;
	size_t current = 0;

	for (const char& ch : str) {
		if (current >= len)
			break;

		hash = ch + (hash << 6) + (hash << 16) - hash;

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
	return TypePreference(a.Type.GetRealType()) < TypePreference(b.Type.GetRealType());
}

static bool FieldTypeCmp(const Field& a, const Field& b)
{
	return a.Type.GetRealType() < b.Type.GetRealType();
}

static std::string FieldTypeToIcingaName(const Field& field, bool inner)
{
	std::string ftype = field.Type.TypeName;

	if (!inner && field.Type.ArrayRank > 0)
		return "Array";

	if (field.Type.IsName)
		return "String";

	if (field.Attributes & FAEnum)
		return "Number";

	if (ftype == "bool" || ftype == "int" || ftype == "double")
		return "Number";

	if (ftype == "int" || ftype == "double")
		return "Number";
	else if (ftype == "bool")
		return "Boolean";

	if (ftype.find("::Ptr") != std::string::npos)
		return ftype.substr(0, ftype.size() - strlen("::Ptr"));

	return ftype;
}

void ClassCompiler::OptimizeStructLayout(std::vector<Field>& fields)
{
	std::sort(fields.begin(), fields.end(), FieldTypeCmp);
	std::stable_sort(fields.begin(), fields.end(), FieldLayoutCmp);
}

void ClassCompiler::HandleClass(const Klass& klass, const ClassDebugInfo&)
{
	/* forward declaration */
	if (klass.Name.find_first_of(':') == std::string::npos)
		m_Header << "class " << klass.Name << ";" << std::endl << std::endl;

	/* TypeHelper */
	if (klass.Attributes & TAAbstract) {
		m_Header << "template<>" << std::endl
			<< "struct TypeHelper<" << klass.Name << ", " << ((klass.Attributes & TAVarArgConstructor) ? "true" : "false") << ">" << std::endl
			<< "{" << std::endl
			<< "\t" << "static ObjectFactory GetFactory()" << std::endl
			<< "\t" << "{" << std::endl
			<< "\t\t" << "return nullptr;" << std::endl
			<< "\t" << "}" << std::endl
			<< "};" << std::endl << std::endl;
	}

	/* TypeImpl */
	m_Header << "template<>" << std::endl
		<< "class TypeImpl<" << klass.Name << ">"
		<< " : public Type";

	if (!klass.Parent.empty())
		m_Header << "Impl<" << klass.Parent << ">";
	
	if (!klass.TypeBase.empty())
		m_Header << ", public " + klass.TypeBase;

	m_Header << std::endl
			<< "{" << std::endl
			<< "public:" << std::endl
			<< "\t" << "DECLARE_PTR_TYPEDEFS(TypeImpl<" << klass.Name << ">);" << std::endl << std::endl
			<< "\t" << "TypeImpl();" << std::endl
			<< "\t" << "~TypeImpl() override;" << std::endl << std::endl;

	m_Impl << "TypeImpl<" << klass.Name << ">::TypeImpl()" << std::endl
		<< "{ }" << std::endl << std::endl
		<< "TypeImpl<" << klass.Name << ">::~TypeImpl()" << std::endl
		<< "{ }" << std::endl << std::endl;

	/* GetName */
	m_Header << "\t" << "String GetName() const override;" << std::endl;

	m_Impl << "String TypeImpl<" << klass.Name << ">::GetName() const" << std::endl
		<< "{" << std::endl
		<< "\t" << "return \"" << klass.Name << "\";" << std::endl
		<< "}" << std::endl << std::endl;

	/* GetAttributes */
	m_Header << "\t" << "int GetAttributes() const override;" << std::endl;

	m_Impl << "int TypeImpl<" << klass.Name << ">::GetAttributes() const" << std::endl
		<< "{" << std::endl
		<< "\t" << "return " << klass.Attributes << ";" << std::endl
		<< "}" << std::endl << std::endl;

	/* GetBaseType */
	m_Header << "\t" << "Type::Ptr GetBaseType() const override;" << std::endl;

	m_Impl << "Type::Ptr TypeImpl<" << klass.Name << ">::GetBaseType() const" << std::endl
		<< "{" << std::endl
		<< "\t" << "return ";

	if (!klass.Parent.empty())
		m_Impl << klass.Parent << "::TypeInstance";
	else
		m_Impl << "Object::TypeInstance";

	m_Impl << ";" << std::endl
		<< "}" << std::endl << std::endl;

	/* GetFieldId */
	m_Header << "\t" << "int GetFieldId(const String& name) const override;" << std::endl;

	m_Impl << "int TypeImpl<" << klass.Name << ">::GetFieldId(const String& name) const" << std::endl
		<< "{" << std::endl;

	if (!klass.Fields.empty()) {
		m_Impl << "\t" << "int offset = ";

		if (!klass.Parent.empty())
			m_Impl << klass.Parent << "::TypeInstance->GetFieldCount()";
		else
			m_Impl << "0";

		m_Impl << ";" << std::endl << std::endl;
	}

	std::map<int, std::vector<std::pair<int, std::string> > > jumptable;

	int hlen = 0, collisions = 0;

	do {
		int num = 0;

		hlen++;
		jumptable.clear();
		collisions = 0;

		for (const Field& field : klass.Fields) {
			int hash = static_cast<int>(SDBM(field.Name, hlen));
			jumptable[hash].push_back(std::make_pair(num, field.Name));
			num++;

			if (jumptable[hash].size() > 1)
				collisions++;
		}
	} while (collisions >= 5 && hlen < 8);

	if (!klass.Fields.empty()) {
		m_Impl << "\tswitch (static_cast<int>(Utility::SDBM(name, " << hlen << "))) {" << std::endl;

		for (const auto& itj : jumptable) {
			m_Impl << "\t\tcase " << itj.first << ":" << std::endl;

			for (const auto& itf : itj.second) {
				m_Impl << "\t\t\t" << "if (name == \"" << itf.second << "\")" << std::endl
					<< "\t\t\t\t" << "return offset + " << itf.first << ";" << std::endl;
			}

			m_Impl << std::endl
					<< "\t\t\t\tbreak;" << std::endl;
		}

		m_Impl << "\t}" << std::endl;
	}

	m_Impl << std::endl
		<< "\t" << "return ";

	if (!klass.Parent.empty())
		m_Impl << klass.Parent << "::TypeInstance->GetFieldId(name)";
	else
		m_Impl << "-1";

	m_Impl << ";" << std::endl
		<< "}" << std::endl << std::endl;

	/* GetFieldInfo */
	m_Header << "\t" << "Field GetFieldInfo(int id) const override;" << std::endl;

	m_Impl << "Field TypeImpl<" << klass.Name << ">::GetFieldInfo(int id) const" << std::endl
		<< "{" << std::endl;

	if (!klass.Parent.empty())
		m_Impl << "\t" << "int real_id = id - " << klass.Parent << "::TypeInstance->GetFieldCount();" << std::endl
			<< "\t" << "if (real_id < 0) { return " << klass.Parent << "::TypeInstance->GetFieldInfo(id); }" << std::endl;

	if (!klass.Fields.empty()) {
		m_Impl << "\t" << "switch (";

		if (!klass.Parent.empty())
			m_Impl << "real_id";
		else
			m_Impl << "id";

		m_Impl << ") {" << std::endl;

		size_t num = 0;
		for (const Field& field : klass.Fields) {
			std::string ftype = FieldTypeToIcingaName(field, false);

			std::string nameref;

			if (field.Type.IsName)
				nameref = "\"" + field.Type.TypeName + "\"";
			else
				nameref = "nullptr";

			m_Impl << "\t\t" << "case " << num << ":" << std::endl
					<< "\t\t\t" << "return Field(" << num << ", \"" << ftype << "\", \"" << field.Name << "\", \"" << (field.NavigationName.empty() ? field.Name : field.NavigationName) << "\", "  << nameref << ", " << field.Attributes << ", " << field.Type.ArrayRank << ");" << std::endl;
			num++;
		}

		m_Impl << "\t\t" << "default:" << std::endl
				<< "\t\t";
	}

	m_Impl << "\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl;

	if (!klass.Fields.empty())
		m_Impl << "\t" << "}" << std::endl;

	m_Impl << "}" << std::endl << std::endl;

	/* GetFieldCount */
	m_Header << "\t" << "int GetFieldCount() const override;" << std::endl;

	m_Impl << "int TypeImpl<" << klass.Name << ">::GetFieldCount() const" << std::endl
		<< "{" << std::endl
		<< "\t" << "return " << klass.Fields.size();

	if (!klass.Parent.empty())
		m_Impl << " + " << klass.Parent << "::TypeInstance->GetFieldCount()";

	m_Impl << ";" << std::endl
		<< "}" << std::endl << std::endl;

	/* GetFactory */
	m_Header << "\t" << "ObjectFactory GetFactory() const override;" << std::endl;

	m_Impl << "ObjectFactory TypeImpl<" << klass.Name << ">::GetFactory() const" << std::endl
		<< "{" << std::endl
		<< "\t" << "return TypeHelper<" << klass.Name << ", " << ((klass.Attributes & TAVarArgConstructor) ? "true" : "false") << ">::GetFactory();" << std::endl
		<< "}" << std::endl << std::endl;

	/* GetLoadDependencies */
	m_Header << "\t" << "std::vector<String> GetLoadDependencies() const override;" << std::endl;

	m_Impl << "std::vector<String> TypeImpl<" << klass.Name << ">::GetLoadDependencies() const" << std::endl
		<< "{" << std::endl
		<< "\t" << "std::vector<String> deps;" << std::endl;

	for (const std::string& dep : klass.LoadDependencies)
		m_Impl << "\t" << "deps.push_back(\"" << dep << "\");" << std::endl;

	m_Impl << "\t" << "return deps;" << std::endl
		<< "}" << std::endl << std::endl;

	/* RegisterAttributeHandler */
	m_Header << "public:" << std::endl
			<< "\t" << "void RegisterAttributeHandler(int fieldId, const Type::AttributeHandler& callback) override;" << std::endl;

	m_Impl << "void TypeImpl<" << klass.Name << ">::RegisterAttributeHandler(int fieldId, const Type::AttributeHandler& callback)" << std::endl
		<< "{" << std::endl;

	if (!klass.Parent.empty())
		m_Impl << "\t" << "int real_id = fieldId - " << klass.Parent << "::TypeInstance->GetFieldCount(); " << std::endl
			<< "\t" << "if (real_id < 0) { " << klass.Parent << "::TypeInstance->RegisterAttributeHandler(fieldId, callback); return; }" << std::endl;

	if (!klass.Fields.empty()) {
		m_Impl << "\t" << "switch (";

		if (!klass.Parent.empty())
			m_Impl << "real_id";
		else
			m_Impl << "fieldId";

		m_Impl << ") {" << std::endl;

		int num = 0;
		for (const Field& field : klass.Fields) {
			m_Impl << "\t\t" << "case " << num << ":" << std::endl
				<< "\t\t\t" << "ObjectImpl<" << klass.Name << ">::On" << field.GetFriendlyName() << "Changed.connect(callback);" << std::endl
				<< "\t\t\t" << "break;" << std::endl;
			num++;
		}

		m_Impl << "\t\t" << "default:" << std::endl
			<< "\t\t";
	}
	m_Impl << "\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl;

	if (!klass.Fields.empty())
		m_Impl << "\t" << "}" << std::endl;

	m_Impl << "}" << std::endl << std::endl;

	m_Header << "};" << std::endl << std::endl;

	m_Header << std::endl;

	/* ObjectImpl */
	m_Header << "template<>" << std::endl
		<< "class ObjectImpl<" << klass.Name << ">"
		<< " : public " << (klass.Parent.empty() ? "Object" : klass.Parent) << std::endl
		<< "{" << std::endl
		<< "public:" << std::endl
		<< "\t" << "DECLARE_PTR_TYPEDEFS(ObjectImpl<" << klass.Name << ">);" << std::endl << std::endl;

	/* Validate */
	m_Header << "\t" << "void Validate(int types, const ValidationUtils& utils) override;" << std::endl;

	m_Impl << "void ObjectImpl<" << klass.Name << ">::Validate(int types, const ValidationUtils& utils)" << std::endl
		<< "{" << std::endl;

	if (!klass.Parent.empty())
		m_Impl << "\t" << klass.Parent << "::Validate(types, utils);" << std::endl << std::endl;

	for (const Field& field : klass.Fields) {
		m_Impl << "\t" << "if (" << (field.Attributes & (FAEphemeral|FAConfig|FAState)) << " & types)" << std::endl
				<< "\t\t" << "Validate" << field.GetFriendlyName() << "(Get" << field.GetFriendlyName() << "(), utils);" << std::endl;
	}

	m_Impl << "}" << std::endl << std::endl;

	for (const Field& field : klass.Fields) {
		std::string argName;

		if (field.Type.ArrayRank > 0)
			argName = "avalue";
		else
			argName = "value";

		m_Header << "\t" << "void SimpleValidate" << field.GetFriendlyName() << "(" << field.Type.GetArgumentType() << " " << argName << ", const ValidationUtils& utils);" << std::endl;

		m_Impl << "void ObjectImpl<" << klass.Name << ">::SimpleValidate" << field.GetFriendlyName() << "(" << field.Type.GetArgumentType() << " " << argName << ", const ValidationUtils& utils)" << std::endl
			<< "{" << std::endl;

		if (field.Attributes & FARequired) {
			if (field.Type.GetRealType().find("::Ptr") != std::string::npos)
				m_Impl << "\t" << "if (!" << argName << ")" << std::endl;
			else
				m_Impl << "\t" << "if (" << argName << ".IsEmpty())" << std::endl;

			m_Impl << "\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_cast<ConfigObject *>(this), { \"" << field.Name << "\" }, \"Attribute must not be empty.\"));" << std::endl << std::endl;
		}

		if (field.Attributes & FADeprecated) {
			if (field.Type.GetRealType().find("::Ptr") != std::string::npos)
				m_Impl << "\t" << "if (" << argName << ")" << std::endl;
			else
				m_Impl << "\t" << "if (" << argName << " != GetDefault" << field.GetFriendlyName() << "())" << std::endl;

			m_Impl << "\t\t" << "Log(LogWarning, \"" << klass.Name << "\") << \"Attribute '" << field.Name << "' for object '\" << dynamic_cast<ConfigObject *>(this)->GetName() << \"' of type '\" << dynamic_cast<ConfigObject *>(this)->GetReflectionType()->GetName() << \"' is deprecated and should not be used.\";" << std::endl;
		}

		if (field.Type.ArrayRank > 0) {
			m_Impl << "\t" << "if (avalue) {" << std::endl
				<< "\t\t" << "ObjectLock olock(avalue);" << std::endl
				<< "\t\t" << "for (const Value& value : avalue) {" << std::endl;
		}

		std::string ftype = FieldTypeToIcingaName(field, true);

		if (ftype == "Value") {
			m_Impl << "\t" << "if (value.IsObjectType<Function>()) {" << std::endl
				<< "\t\t" << "Function::Ptr func = value;" << std::endl
				<< "\t\t" << "if (func->IsDeprecated())" << std::endl
				<< "\t\t\t" << "Log(LogWarning, \"" << klass.Name << "\") << \"Attribute '" << field.Name << "' for object '\" << dynamic_cast<ConfigObject *>(this)->GetName() << \"' of type '\" << dynamic_cast<ConfigObject *>(this)->GetReflectionType()->GetName() << \"' is set to a deprecated function: \" << func->GetName();" << std::endl
				<< "\t" << "}" << std::endl << std::endl;
		}

		if (field.Type.IsName) {
			m_Impl << "\t" << "if (";

			if (field.Type.ArrayRank > 0)
				m_Impl << "value.IsEmpty() || ";
			else
				m_Impl << "!value.IsEmpty() && ";

			m_Impl << "!utils.ValidateName(\"" << field.Type.TypeName << "\", value))" << std::endl
				<< "\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_cast<ConfigObject *>(this), { \"" << field.Name << "\" }, \"Object '\" + value + \"' of type '" << field.Type.TypeName
				<< "' does not exist.\"));" << std::endl;
		} else if (field.Type.ArrayRank > 0 && (ftype == "Number" || ftype == "Boolean")) {
			m_Impl << "\t" << "try {" << std::endl
				<< "\t\t" << "Convert::ToDouble(value);" << std::endl
				<< "\t" << "} catch (const std::invalid_argument&) {" << std::endl
				<< "\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_cast<ConfigObject *>(this), { \"" << field.Name << "\", \"Array element '\" + value + \"' of type '\" + value.GetReflectionType()->GetName() + \"' is not valid here; expected type '" << ftype << "'.\"));" << std::endl
				<< "\t" << "}" << std::endl;
		}

		if (field.Type.ArrayRank > 0) {
			m_Impl << "\t\t" << "}" << std::endl
				<< "\t" << "}" << std::endl;
		}

		m_Impl << "}" << std::endl << std::endl;
	}

	/* constructor */
	m_Header << "public:" << std::endl
			<< "\t" << "ObjectImpl<" << klass.Name << ">();" << std::endl;

	m_Impl << "ObjectImpl<" << klass.Name << ">::ObjectImpl()" << std::endl
		<< "{" << std::endl;

	for (const Field& field : klass.Fields) {
		m_Impl << "\t" << "Set" << field.GetFriendlyName() << "(" << "GetDefault" << field.GetFriendlyName() << "(), true);" << std::endl;
	}

	m_Impl << "}" << std::endl << std::endl;

	/* destructor */
	m_Header << "public:" << std::endl
			<< "\t" << "~ObjectImpl<" << klass.Name << ">() override;" << std::endl;

	m_Impl << "ObjectImpl<" << klass.Name << ">::~ObjectImpl()" << std::endl
		<< "{ }" << std::endl << std::endl;

	if (!klass.Fields.empty()) {
		/* SetField */
		m_Header << "public:" << std::endl
				<< "\t" << "void SetField(int id, const Value& value, bool suppress_events = false, const Value& cookie = Empty) override;" << std::endl;

		m_Impl << "void ObjectImpl<" << klass.Name << ">::SetField(int id, const Value& value, bool suppress_events, const Value& cookie)" << std::endl
			<< "{" << std::endl;

		if (!klass.Parent.empty())
			m_Impl << "\t" << "int real_id = id - " << klass.Parent << "::TypeInstance->GetFieldCount(); " << std::endl
				<< "\t" << "if (real_id < 0) { " << klass.Parent << "::SetField(id, value, suppress_events, cookie); return; }" << std::endl;

		m_Impl << "\t" << "switch (";

		if (!klass.Parent.empty())
			m_Impl << "real_id";
		else
			m_Impl << "id";

		m_Impl << ") {" << std::endl;

		size_t num = 0;
		for (const Field& field : klass.Fields) {
			m_Impl << "\t\t" << "case " << num << ":" << std::endl
				<< "\t\t\t" << "Set" << field.GetFriendlyName() << "(";
			
			if (field.Attributes & FAEnum)
				m_Impl << "static_cast<" << field.Type.GetRealType() << ">(static_cast<int>(";

			m_Impl << "value";
			
			if (field.Attributes & FAEnum)
				m_Impl << "))";
			
			m_Impl << ", suppress_events, cookie);" << std::endl
				<< "\t\t\t" << "break;" << std::endl;
			num++;
		}

		m_Impl << "\t\t" << "default:" << std::endl
			<< "\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
			<< "\t" << "}" << std::endl;

		m_Impl << "}" << std::endl << std::endl;

		/* GetField */
		m_Header << "public:" << std::endl
				<< "\t" << "Value GetField(int id) const override;" << std::endl;

		m_Impl << "Value ObjectImpl<" << klass.Name << ">::GetField(int id) const" << std::endl
			<< "{" << std::endl;

		if (!klass.Parent.empty())
			m_Impl << "\t" << "int real_id = id - " << klass.Parent << "::TypeInstance->GetFieldCount(); " << std::endl
				<< "\t" << "if (real_id < 0) { return " << klass.Parent << "::GetField(id); }" << std::endl;

		m_Impl << "\t" << "switch (";

		if (!klass.Parent.empty())
			m_Impl << "real_id";
		else
			m_Impl << "id";

		m_Impl << ") {" << std::endl;

		num = 0;
		for (const Field& field : klass.Fields) {
			m_Impl << "\t\t" << "case " << num << ":" << std::endl
				<< "\t\t\t" << "return Get" << field.GetFriendlyName() << "();" << std::endl;
			num++;
		}

		m_Impl << "\t\t" << "default:" << std::endl
			<< "\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
			<< "\t" << "}" << std::endl;

		m_Impl << "}" << std::endl << std::endl;
		
		/* ValidateField */
		m_Header << "public:" << std::endl
				<< "\t" << "void ValidateField(int id, const Value& value, const ValidationUtils& utils) override;" << std::endl;

		m_Impl << "void ObjectImpl<" << klass.Name << ">::ValidateField(int id, const Value& value, const ValidationUtils& utils)" << std::endl
			<< "{" << std::endl;

		if (!klass.Parent.empty())
			m_Impl << "\t" << "int real_id = id - " << klass.Parent << "::TypeInstance->GetFieldCount(); " << std::endl
				<< "\t" << "if (real_id < 0) { " << klass.Parent << "::ValidateField(id, value, utils); return; }" << std::endl;

		m_Impl << "\t" << "switch (";

		if (!klass.Parent.empty())
			m_Impl << "real_id";
		else
			m_Impl << "id";

		m_Impl << ") {" << std::endl;

		num = 0;
		for (const Field& field : klass.Fields) {
			m_Impl << "\t\t" << "case " << num << ":" << std::endl
				<< "\t\t\t" << "Validate" << field.GetFriendlyName() << "(";
			
			if (field.Attributes & FAEnum)
				m_Impl << "static_cast<" << field.Type.GetRealType() << ">(static_cast<int>(";

			m_Impl << "value";
			
			if (field.Attributes & FAEnum)
				m_Impl << "))";
			
			m_Impl << ", utils);" << std::endl
				<< "\t\t\t" << "break;" << std::endl;
			num++;
		}

		m_Impl << "\t\t" << "default:" << std::endl
			<< "\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
			<< "\t" << "}" << std::endl;

		m_Impl << "}" << std::endl << std::endl;

		/* NotifyField */
		m_Header << "public:" << std::endl
				<< "\t" << "void NotifyField(int id, const Value& cookie = Empty) override;" << std::endl;

		m_Impl << "void ObjectImpl<" << klass.Name << ">::NotifyField(int id, const Value& cookie)" << std::endl
			<< "{" << std::endl;

		if (!klass.Parent.empty())
			m_Impl << "\t" << "int real_id = id - " << klass.Parent << "::TypeInstance->GetFieldCount(); " << std::endl
				<< "\t" << "if (real_id < 0) { " << klass.Parent << "::NotifyField(id, cookie); return; }" << std::endl;

		m_Impl << "\t" << "switch (";

		if (!klass.Parent.empty())
			m_Impl << "real_id";
		else
			m_Impl << "id";

		m_Impl << ") {" << std::endl;

		num = 0;
		for (const Field& field : klass.Fields) {
			m_Impl << "\t\t" << "case " << num << ":" << std::endl
				<< "\t\t\t" << "Notify" << field.GetFriendlyName() << "(cookie);" << std::endl
				<< "\t\t\t" << "break;" << std::endl;
			num++;
		}

		m_Impl << "\t\t" << "default:" << std::endl
			<< "\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
			<< "\t" << "}" << std::endl;

		m_Impl << "}" << std::endl << std::endl;

		/* NavigateField */
		m_Header << "public:" << std::endl
				<< "\t" << "Object::Ptr NavigateField(int id) const override;" << std::endl;

		m_Impl << "Object::Ptr ObjectImpl<" << klass.Name << ">::NavigateField(int id) const" << std::endl
			<< "{" << std::endl;

		if (!klass.Parent.empty())
			m_Impl << "\t" << "int real_id = id - " << klass.Parent << "::TypeInstance->GetFieldCount(); " << std::endl
				<< "\t" << "if (real_id < 0) { return " << klass.Parent << "::NavigateField(id); }" << std::endl;

		bool haveNavigationFields = false;

		for (const Field& field : klass.Fields) {
			if (field.Attributes & FANavigation) {
				haveNavigationFields = true;
				break;
			}
		}

		if (haveNavigationFields) {
			m_Impl << "\t" << "switch (";

			if (!klass.Parent.empty())
				m_Impl << "real_id";
			else
				m_Impl << "id";

			m_Impl << ") {" << std::endl;

			num = 0;
			for (const Field& field : klass.Fields) {
				if (field.Attributes & FANavigation) {
					m_Impl << "\t\t" << "case " << num << ":" << std::endl
						<< "\t\t\t" << "return Navigate" << field.GetFriendlyName() << "();" << std::endl;
				}

				num++;
			}

			m_Impl << "\t\t" << "default:" << std::endl
				<< "\t\t";
		}

		m_Impl << "\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl;

		if (haveNavigationFields)
			m_Impl << "\t" << "}" << std::endl;

		m_Impl << "}" << std::endl << std::endl;

		/* getters */
		for (const Field& field : klass.Fields) {
			std::string prot;

			if (field.Attributes & FAGetProtected)
				prot = "protected";
			else
				prot = "public";

			m_Header << prot << ":" << std::endl
					<< "\t";

			if (field.Attributes & FAGetVirtual || field.PureGetAccessor)
				m_Header << "virtual ";

			m_Header << field.Type.GetRealType() << " Get" << field.GetFriendlyName() << "() const";

			if (field.PureGetAccessor) {
				m_Header << " = 0;" << std::endl;
			} else {
				m_Header << ";" << std::endl;

				m_Impl << field.Type.GetRealType() << " ObjectImpl<" << klass.Name << ">::Get" << field.GetFriendlyName() << "() const" << std::endl
					<< "{" << std::endl;

				if (field.GetAccessor.empty() && !(field.Attributes & FANoStorage))
					m_Impl << "\t" << "return m_" << field.GetFriendlyName() << ";" << std::endl;
				else
					m_Impl << field.GetAccessor << std::endl;

				m_Impl << "}" << std::endl << std::endl;
			}
		}

		/* setters */
		for (const Field& field : klass.Fields) {
			std::string prot;

			if (field.Attributes & FASetProtected)
				prot = "protected";
			else
				prot = "public";

			m_Header << prot << ":" << std::endl
				<< "\t";

			if (field.Attributes & FASetVirtual || field.PureSetAccessor)
				m_Header << "virtual ";

			m_Header << "void Set" << field.GetFriendlyName() << "(" << field.Type.GetArgumentType() << " value, bool suppress_events = false, const Value& cookie = Empty)";

			if (field.PureSetAccessor) {
				m_Header << " = 0;" << std::endl;
			} else {
				m_Header << ";" << std::endl;

				m_Impl << "void ObjectImpl<" << klass.Name << ">::Set" << field.GetFriendlyName() << "(" << field.Type.GetArgumentType() << " value, bool suppress_events, const Value& cookie)" << std::endl
					<< "{" << std::endl;

				if (field.Type.IsName || !field.TrackAccessor.empty())
					m_Impl << "\t" << "Value oldValue = Get" << field.GetFriendlyName() << "();" << std::endl;

					
				if (field.SetAccessor.empty() && !(field.Attributes & FANoStorage))
					m_Impl << "\t" << "m_" << field.GetFriendlyName() << " = value;" << std::endl;
				else
					m_Impl << field.SetAccessor << std::endl << std::endl;

				if (field.Type.IsName || !field.TrackAccessor.empty()) {
					if (field.Name != "active") {
						m_Impl << "\t" << "ConfigObject *dobj = dynamic_cast<ConfigObject *>(this);" << std::endl
							<< "\t" << "if (!dobj || dobj->IsActive())" << std::endl
							<< "\t";
					}

					m_Impl << "\t" << "Track" << field.GetFriendlyName() << "(oldValue, value);" << std::endl;
				}

				m_Impl << "\t" << "if (!suppress_events)" << std::endl
					<< "\t\t" << "Notify" << field.GetFriendlyName() << "(cookie);" << std::endl
					<< "}" << std::endl << std::endl;
			}
		}

		m_Header << "protected:" << std::endl;

		bool needs_tracking = false;

		/* tracking */
		for (const Field& field : klass.Fields) {
			if (!field.Type.IsName && field.TrackAccessor.empty())
				continue;

			needs_tracking = true;

			m_Header << "\t" << "void Track" << field.GetFriendlyName() << "(" << field.Type.GetArgumentType() << " oldValue, " << field.Type.GetArgumentType() << " newValue);" << std::endl;

			m_Impl << "void ObjectImpl<" << klass.Name << ">::Track" << field.GetFriendlyName() << "(" << field.Type.GetArgumentType() << " oldValue, " << field.Type.GetArgumentType() << " newValue)" << std::endl
				<< "{" << std::endl;

			if (!field.TrackAccessor.empty())
				m_Impl << "\t" << field.TrackAccessor << std::endl;

			if (field.Type.TypeName != "String") {
				if (field.Type.ArrayRank > 0) {
					m_Impl << "\t" << "if (oldValue) {" << std::endl
						<< "\t\t" << "ObjectLock olock(oldValue);" << std::endl
						<< "\t\t" << "for (const String& ref : oldValue) {" << std::endl
						<< "\t\t\t" << "DependencyGraph::RemoveDependency(this, ConfigObject::GetObject";

					/* Ew */
					if (field.Type.TypeName == "Zone" && m_Library == "base")
						m_Impl << "(\"Zone\", ";
					else
						m_Impl << "<" << field.Type.TypeName << ">(";

					m_Impl << "ref).get());" << std::endl
						<< "\t\t" << "}" << std::endl
						<< "\t" << "}" << std::endl
						<< "\t" << "if (newValue) {" << std::endl
						<< "\t\t" << "ObjectLock olock(newValue);" << std::endl
						<< "\t\t" << "for (const String& ref : newValue) {" << std::endl
						<< "\t\t\t" << "DependencyGraph::AddDependency(this, ConfigObject::GetObject";

					/* Ew */
					if (field.Type.TypeName == "Zone" && m_Library == "base")
						m_Impl << "(\"Zone\", ";
					else
						m_Impl << "<" << field.Type.TypeName << ">(";

					m_Impl << "ref).get());" << std::endl
						<< "\t\t" << "}" << std::endl
						<< "\t" << "}" << std::endl;
				} else {
					m_Impl << "\t" << "if (!oldValue.IsEmpty())" << std::endl
						<< "\t\t" << "DependencyGraph::RemoveDependency(this, ConfigObject::GetObject";

					/* Ew */
					if (field.Type.TypeName == "Zone" && m_Library == "base")
						m_Impl << "(\"Zone\", ";
					else
						m_Impl << "<" << field.Type.TypeName << ">(";

					m_Impl << "oldValue).get());" << std::endl
						<< "\t" << "if (!newValue.IsEmpty())" << std::endl
						<< "\t\t" << "DependencyGraph::AddDependency(this, ConfigObject::GetObject";

					/* Ew */
					if (field.Type.TypeName == "Zone" && m_Library == "base")
						m_Impl << "(\"Zone\", ";
					else
						m_Impl << "<" << field.Type.TypeName << ">(";

					m_Impl << "newValue).get());" << std::endl;
				}
			}

			m_Impl << "}" << std::endl << std::endl;
		}

		/* navigation */
		for (const Field& field : klass.Fields) {
			if ((field.Attributes & FANavigation) == 0)
				continue;

			m_Header << "public:" << std::endl
					<< "\t" << "Object::Ptr Navigate" << field.GetFriendlyName() << "() const";

			if (field.PureNavigateAccessor) {
				m_Header << " = 0;" << std::endl;
			} else {
				m_Header << ";" << std::endl;

				m_Impl << "Object::Ptr ObjectImpl<" << klass.Name << ">::Navigate" << field.GetFriendlyName() << "() const" << std::endl
					<< "{" << std::endl;

				if (field.NavigateAccessor.empty())
					m_Impl << "\t" << "return Get" << field.GetFriendlyName() << "();" << std::endl;
				else
					m_Impl << "\t" << field.NavigateAccessor << std::endl;

				m_Impl << "}" << std::endl << std::endl;
			}
		}

		/* start/stop */
		if (needs_tracking) {
			m_Header << "protected:" << std::endl
					<< "\tvoid Start(bool runtimeCreated = false) override;" << std::endl
					<< "\tvoid Stop(bool runtimeRemoved = false) override;" << std::endl;

			m_Impl << "void ObjectImpl<" << klass.Name << ">::Start(bool runtimeCreated)" << std::endl
				<< "{" << std::endl
				<< "\t" << klass.Parent << "::Start(runtimeCreated);" << std::endl << std::endl;

			for (const Field& field : klass.Fields) {
				if (!field.Type.IsName && field.TrackAccessor.empty())
					continue;

				m_Impl << "\t" << "Track" << field.GetFriendlyName() << "(Empty, Get" << field.GetFriendlyName() << "());" << std::endl;
			}

			m_Impl << "}" << std::endl << std::endl
				<< "void ObjectImpl<" << klass.Name << ">::Stop(bool runtimeRemoved)" << std::endl
				<< "{" << std::endl
				<< "\t" << klass.Parent << "::Stop(runtimeRemoved);" << std::endl << std::endl;

			for (const Field& field : klass.Fields) {
				if (!field.Type.IsName && field.TrackAccessor.empty())
					continue;

				m_Impl << "\t" << "Track" << field.GetFriendlyName() << "(Get" << field.GetFriendlyName() << "(), Empty);" << std::endl;
			}

			m_Impl << "}" << std::endl << std::endl;
		}

		/* notify */
		for (const Field& field : klass.Fields) {
			std::string prot;

			if (field.Attributes & FASetProtected)
				prot = "protected";
			else
				prot = "public";

			m_Header << prot << ":" << std::endl
					<< "\t" << "virtual void Notify" << field.GetFriendlyName() << "(const Value& cookie = Empty);" << std::endl;

			m_Impl << "void ObjectImpl<" << klass.Name << ">::Notify" << field.GetFriendlyName() << "(const Value& cookie)" << std::endl
				<< "{" << std::endl;

			if (field.Name != "active") {
				m_Impl << "\t" << "ConfigObject *dobj = dynamic_cast<ConfigObject *>(this);" << std::endl
					<< "\t" << "if (!dobj || dobj->IsActive())" << std::endl
					<< "\t";
			}

			m_Impl << "\t" << "On" << field.GetFriendlyName() << "Changed(static_cast<" << klass.Name << " *>(this), cookie);" << std::endl
				<< "}" << std::endl << std::endl;
		}
		
		/* default */
		for (const Field& field : klass.Fields) {
			std::string realType = field.Type.GetRealType();

			m_Header << "private:" << std::endl
					<< "\t" << "inline " << realType << " GetDefault" << field.GetFriendlyName() << "() const;" << std::endl;

			m_Impl << realType << " ObjectImpl<" << klass.Name << ">::GetDefault" << field.GetFriendlyName() << "() const" << std::endl
				<< "{" << std::endl;

			if (field.DefaultAccessor.empty())
				m_Impl << "\t" << "return " << realType << "();" << std::endl;
			else
				m_Impl << "\t" << field.DefaultAccessor << std::endl;

			m_Impl << "}" << std::endl << std::endl;
		}

		/* validators */
		for (const Field& field : klass.Fields) {
			m_Header << "protected:" << std::endl
					<< "\t" << "virtual void Validate" << field.GetFriendlyName() << "(" << field.Type.GetArgumentType() << " value, const ValidationUtils& utils);" << std::endl;
		}

		/* instance variables */
		m_Header << "private:" << std::endl;

		for (const Field& field : klass.Fields) {
			if (field.Attributes & FANoStorage)
				continue;

			m_Header << "\t" << field.Type.GetRealType() << " m_" << field.GetFriendlyName() << ";" << std::endl;
		}
		
		/* signal */
		m_Header << "public:" << std::endl;
		
		for (const Field& field : klass.Fields) {
			m_Header << "\t" << "static boost::signals2::signal<void (const intrusive_ptr<" << klass.Name << ">&, const Value&)> On" << field.GetFriendlyName() << "Changed;" << std::endl;
			m_Impl << std::endl << "boost::signals2::signal<void (const intrusive_ptr<" << klass.Name << ">&, const Value&)> ObjectImpl<" << klass.Name << ">::On" << field.GetFriendlyName() << "Changed;" << std::endl << std::endl;
		}
	}

	if (klass.Name == "ConfigObject")
		m_Header << "\t" << "friend class ConfigItem;" << std::endl;

	if (!klass.TypeBase.empty())
		m_Header << "\t" << "friend class " << klass.TypeBase << ";" << std::endl;

	m_Header << "};" << std::endl << std::endl;

	for (const Field& field : klass.Fields) {
		m_MissingValidators[std::make_pair(klass.Name, field.GetFriendlyName())] = field;
	}
}

void ClassCompiler::CodeGenValidator(const std::string& name, const std::string& klass, const std::vector<Rule>& rules, const std::string& field, const FieldType& fieldType, ValidatorType validatorType)
{
	m_Impl << "static void TIValidate" << name << "(const intrusive_ptr<ObjectImpl<" << klass << "> >& object, ";

	if (validatorType != ValidatorField)
		m_Impl << "const String& key, ";

	m_Impl << fieldType.GetArgumentType() << " value, std::vector<String>& location, const ValidationUtils& utils)" << std::endl
		<< "{" << std::endl;

	if (validatorType == ValidatorField) {
		bool required = false;

		for (const Rule& rule : rules) {
			if ((rule.Attributes & RARequired) && rule.Pattern == field) {
				required = true;
				break;
			}
		}

		if (fieldType.GetRealType() != "int" && fieldType.GetRealType() != "double") {
			if (fieldType.GetRealType() == "Value" || fieldType.GetRealType() == "String")
				m_Impl << "\t" << "if (value.IsEmpty())" << std::endl;
			else
				m_Impl << "\t" << "if (!value)" << std::endl;

			if (required)
				m_Impl << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_cast<ConfigObject *>(this), location, \"This attribute must not be empty.\"));" << std::endl;
			else
				m_Impl << "\t\t" << "return;" << std::endl;

			m_Impl << std::endl;
		}
	}

	if (validatorType != ValidatorField)
		m_Impl << "\t" << "bool known_attribute = false;" << std::endl;

	bool type_check = false;
	int i = 0;

	for (const Rule& rule : rules) {
		if (rule.Attributes & RARequired)
			continue;

		i++;

		if (validatorType == ValidatorField && rule.Pattern != field)
			continue;

		m_Impl << "\t" << "do {" << std::endl;

		if (validatorType != ValidatorField) {
			if (rule.Pattern != "*") {
				if (rule.Pattern.find_first_of("*?") != std::string::npos)
					m_Impl << "\t\t" << "if (!Utility::Match(\"" << rule.Pattern << "\", key))" << std::endl;
				else
					m_Impl << "\t\t" << "if (key != \"" << rule.Pattern << "\")" << std::endl;

				m_Impl << "\t\t\t" << "break;" << std::endl;
			}

			m_Impl << "\t\t" << "known_attribute = true;" << std::endl;
		}

		if (rule.IsName) {
			m_Impl << "\t\t" << "if (value.IsScalar()) {" << std::endl
				<< "\t\t\t" << "if (utils.ValidateName(\"" << rule.Type << "\", value))" << std::endl
				<< "\t\t\t\t" << "return;" << std::endl
				<< "\t\t\t" << "else" << std::endl
				<< "\t\t\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_pointer_cast<ConfigObject>(object), location, \"Object '\" + value + \"' of type '" << rule.Type << "' does not exist.\"));" << std::endl
				<< "\t\t" << "}" << std::endl;
		}

		if (fieldType.GetRealType() == "Value") {
			if (rule.Type == "String")
				m_Impl << "\t\t" << "if (value.IsEmpty() || value.IsScalar())" << std::endl
					<< "\t\t\t" << "return;" << std::endl;
			else if (rule.Type == "Number") {
				m_Impl << "\t\t" << "try {" << std::endl
					<< "\t\t\t" << "Convert::ToDouble(value);" << std::endl
					<< "\t\t\t" << "return;" << std::endl
					<< "\t\t" << "} catch (...) { }" << std::endl;
			}
		}

		if (rule.Type == "Dictionary" || rule.Type == "Array" || rule.Type == "Function") {
			if (fieldType.GetRealType() == "Value") {
				m_Impl << "\t\t" << "if (value.IsObjectType<" << rule.Type << ">()) {" << std::endl;
				type_check = true;
			} else if (fieldType.GetRealType() != rule.Type + "::Ptr") {
				m_Impl << "\t\t" << "if (dynamic_pointer_cast<" << rule.Type << ">(value)) {" << std::endl;
				type_check = true;
			}

			if (!rule.Rules.empty()) {
				bool indent = false;

				if (rule.Type == "Dictionary") {
					if (type_check)
						m_Impl << "\t\t\t" << "Dictionary::Ptr dict = value;" << std::endl;
					else
						m_Impl << "\t\t" << "const Dictionary::Ptr& dict = value;" << std::endl;

					m_Impl << (type_check ? "\t" : "") << "\t\t" << "{" << std::endl
						<< (type_check ? "\t" : "") << "\t\t\t" << "ObjectLock olock(dict);" << std::endl
						<< (type_check ? "\t" : "") << "\t\t\t" << "for (const Dictionary::Pair& kv : dict) {" << std::endl
						<< (type_check ? "\t" : "") << "\t\t\t\t" << "const String& akey = kv.first;" << std::endl
						<< (type_check ? "\t" : "") << "\t\t\t\t" << "const Value& avalue = kv.second;" << std::endl;
					indent = true;
				} else if (rule.Type == "Array") {
					if (type_check)
						m_Impl << "\t\t\t" << "Array::Ptr arr = value;" << std::endl;
					else
						m_Impl << "\t\t" << "const Array::Ptr& arr = value;" << std::endl;

					m_Impl << (type_check ? "\t" : "") << "\t\t" << "Array::SizeType anum = 0;" << std::endl
						<< (type_check ? "\t" : "") << "\t\t" << "{" << std::endl
						<< (type_check ? "\t" : "") << "\t\t\t" << "ObjectLock olock(arr);" << std::endl
						<< (type_check ? "\t" : "") << "\t\t\t" << "for (const Value& avalue : arr) {" << std::endl
						<< (type_check ? "\t" : "") << "\t\t\t\t" << "String akey = Convert::ToString(anum);" << std::endl;
					indent = true;
				} else {
					m_Impl << (type_check ? "\t" : "") << "\t\t" << "String akey = \"\";" << std::endl
						<< (type_check ? "\t" : "") << "\t\t" << "const Value& avalue = value;" << std::endl;
				}

				std::string subvalidator_prefix;

				if (validatorType == ValidatorField)
					subvalidator_prefix = klass;
				else
					subvalidator_prefix = name;

				m_Impl << (type_check ? "\t" : "") << (indent ? "\t\t" : "") << "\t\t" << "location.push_back(akey);" << std::endl
					<< (type_check ? "\t" : "") << (indent ? "\t\t" : "") << "\t\t" << "TIValidate" << subvalidator_prefix << "_" << i << "(object, akey, avalue, location, utils);" << std::endl
					<< (type_check ? "\t" : "") << (indent ? "\t\t" : "") << "\t\t" << "location.pop_back();" << std::endl;

				if (rule.Type == "Array")
					m_Impl << (type_check ? "\t" : "") << "\t\t\t\t" << "anum++;" << std::endl;

				if (rule.Type == "Dictionary" || rule.Type == "Array") {
					m_Impl << (type_check ? "\t" : "") << "\t\t\t" << "}" << std::endl
						<< (type_check ? "\t" : "") << "\t\t" << "}" << std::endl;
				}

				for (const Rule& srule : rule.Rules) {
					if ((srule.Attributes & RARequired) == 0)
						continue;

					if (rule.Type == "Dictionary") {
						m_Impl << (type_check ? "\t" : "") << "\t\t" << "if (dict->Get(\"" << srule.Pattern << "\").IsEmpty())" << std::endl
							<< (type_check ? "\t" : "") << "\t\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_pointer_cast<ConfigObject>(object), location, \"Required dictionary item '" << srule.Pattern << "' is not set.\"));" << std::endl;
					} else if (rule.Type == "Array") {
						int index = -1;
						std::stringstream idxbuf;
						idxbuf << srule.Pattern;
						idxbuf >> index;

						if (index == -1) {
							std::cerr << "Invalid index for 'required' keyword: " << srule.Pattern;
							std::exit(1);
						}

						m_Impl << (type_check ? "\t" : "") << "\t\t" << "if (arr.GetLength() < " << (index + 1) << ")" << std::endl
							<< (type_check ? "\t" : "") << "\t\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_cast<ConfigObject *>(this), location, \"Required index '" << index << "' is not set.\"));" << std::endl;
					}
				}
			}

			m_Impl << (type_check ? "\t" : "") << "\t\t" << "return;" << std::endl;

			if (fieldType.GetRealType() == "Value" || fieldType.GetRealType() != rule.Type + "::Ptr")
				m_Impl << "\t\t" << "}" << std::endl;
		}

		m_Impl << "\t" << "} while (0);" << std::endl << std::endl;
	}

	if (type_check || validatorType != ValidatorField) {
		if (validatorType != ValidatorField) {
			m_Impl << "\t" << "if (!known_attribute)" << std::endl
				<< "\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_pointer_cast<ConfigObject>(object), location, \"Invalid attribute: \" + key));" << std::endl
				<< "\t" << "else" << std::endl;
		}

		m_Impl << (validatorType != ValidatorField ? "\t" : "") << "\t" << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_pointer_cast<ConfigObject>(object), location, \"Invalid type.\"));" << std::endl;
	}

	m_Impl << "}" << std::endl << std::endl;
}

void ClassCompiler::CodeGenValidatorSubrules(const std::string& name, const std::string& klass, const std::vector<Rule>& rules)
{
	int i = 0;

	for (const Rule& rule : rules) {
		if (rule.Attributes & RARequired)
			continue;

		i++;

		if (!rule.Rules.empty()) {
			ValidatorType subtype;

			if (rule.Type == "Array")
				subtype = ValidatorArray;
			else if (rule.Type == "Dictionary")
				subtype = ValidatorDictionary;
			else {
				std::cerr << "Invalid sub-validator type: " << rule.Type << std::endl;
				std::exit(EXIT_FAILURE);
			}

			std::ostringstream namebuf;
			namebuf << name << "_" << i;

			CodeGenValidatorSubrules(namebuf.str(), klass, rule.Rules);

			FieldType ftype;
			ftype.IsName = false;
			ftype.TypeName = "Value";
			CodeGenValidator(namebuf.str(), klass, rule.Rules, rule.Pattern, ftype, subtype);
		}
	}
}

void ClassCompiler::HandleValidator(const Validator& validator, const ClassDebugInfo&)
{
	CodeGenValidatorSubrules(validator.Name, validator.Name, validator.Rules);

	for (const auto& it : m_MissingValidators)
		CodeGenValidator(it.first.first + it.first.second, it.first.first, validator.Rules, it.second.Name, it.second.Type, ValidatorField);

	for (const auto& it : m_MissingValidators) {
		m_Impl << "void ObjectImpl<" << it.first.first << ">::Validate" << it.first.second << "(" << it.second.Type.GetArgumentType() << " value, const ValidationUtils& utils)" << std::endl
			<< "{" << std::endl
			<< "\t" << "SimpleValidate" << it.first.second << "(value, utils);" << std::endl
			<< "\t" << "std::vector<String> location;" << std::endl
			<< "\t" << "location.push_back(\"" << it.second.Name << "\");" << std::endl
			<< "\t" << "TIValidate" << it.first.first << it.first.second << "(this, value, location, utils);" << std::endl
			<< "\t" << "location.pop_back();" << std::endl
			<< "}" << std::endl << std::endl;
	}

	m_MissingValidators.clear();
}

void ClassCompiler::HandleMissingValidators()
{
	for (const auto& it : m_MissingValidators) {
		m_Impl << "void ObjectImpl<" << it.first.first << ">::Validate" << it.first.second << "(" << it.second.Type.GetArgumentType() << " value, const ValidationUtils& utils)" << std::endl
			<< "{" << std::endl
			<< "\t" << "SimpleValidate" << it.first.second << "(value, utils);" << std::endl
			<< "}" << std::endl << std::endl;
	}

	m_MissingValidators.clear();
}

void ClassCompiler::CompileFile(const std::string& inputpath,
	const std::string& implpath, const std::string& headerpath)
{
	std::ifstream input;
	input.open(inputpath.c_str(), std::ifstream::in);

	if (!input) {
		std::cerr << "Could not open input file: " << inputpath << std::endl;
		std::exit(EXIT_FAILURE);
	}

	std::string tmpheaderpath = headerpath + ".tmp";
	std::ofstream oheader;
	oheader.open(tmpheaderpath.c_str(), std::ofstream::out);

	if (!oheader) {
		std::cerr << "Could not open header file: " << tmpheaderpath << std::endl;
		std::exit(EXIT_FAILURE);
	}

	std::string tmpimplpath = implpath + ".tmp";
	std::ofstream oimpl;
	oimpl.open(tmpimplpath.c_str(), std::ofstream::out);

	if (!oimpl) {
		std::cerr << "Could not open implementation file: " << tmpimplpath << std::endl;
		std::exit(EXIT_FAILURE);
	}

	CompileStream(inputpath, input, oimpl, oheader);

	input.close();
	oimpl.close();
	oheader.close();

#ifdef _WIN32
	_unlink(headerpath.c_str());
#endif /* _WIN32 */

	if (rename(tmpheaderpath.c_str(), headerpath.c_str()) < 0) {
		std::cerr << "Could not rename header file: " << tmpheaderpath << " -> " << headerpath << std::endl;
		std::exit(EXIT_FAILURE);
	}

#ifdef _WIN32
	_unlink(implpath.c_str());
#endif /* _WIN32 */

	if (rename(tmpimplpath.c_str(), implpath.c_str()) < 0) {
		std::cerr << "Could not rename implementation file: " << tmpimplpath << " -> " << implpath << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

std::string ClassCompiler::BaseName(const std::string& path)
{
	char *dir = strdup(path.c_str());
	std::string result;

	if (!dir)
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
	std::locale locale;

	for (auto& ch : result) {
		ch = std::toupper(ch, locale);

		if (ch == '.')
			ch = '_';
	}

	return result;
}

void ClassCompiler::CompileStream(const std::string& path, std::istream& input,
	std::ostream& oimpl, std::ostream& oheader)
{
	input.exceptions(std::istream::badbit);

	std::string guard_name = FileNameToGuardName(BaseName(path));

	oheader << "#ifndef " << guard_name << std::endl
		<< "#define " << guard_name << std::endl << std::endl;

	oheader << "#include \"base/object.hpp\"" << std::endl
		<< "#include \"base/type.hpp\"" << std::endl
		<< "#include \"base/value.hpp\"" << std::endl
		<< "#include \"base/array.hpp\"" << std::endl
		<< "#include \"base/dictionary.hpp\"" << std::endl
		<< "#include <boost/signals2.hpp>" << std::endl << std::endl;

	oimpl << "#include \"base/exception.hpp\"" << std::endl
		<< "#include \"base/objectlock.hpp\"" << std::endl
		<< "#include \"base/utility.hpp\"" << std::endl
		<< "#include \"base/convert.hpp\"" << std::endl
		<< "#include \"base/dependencygraph.hpp\"" << std::endl
		<< "#include \"base/logger.hpp\"" << std::endl
		<< "#include \"base/function.hpp\"" << std::endl
		<< "#include \"base/configtype.hpp\"" << std::endl
		<< "#ifdef _MSC_VER" << std::endl
		<< "#pragma warning( push )" << std::endl
		<< "#pragma warning( disable : 4244 )" << std::endl
		<< "#pragma warning( disable : 4800 )" << std::endl
		<< "#endif /* _MSC_VER */" << std::endl << std::endl;


	ClassCompiler ctx(path, input, oimpl, oheader);
	ctx.Compile();

	oheader << "#endif /* " << guard_name << " */" << std::endl;

	oimpl << "#ifdef _MSC_VER" << std::endl
		<< "#pragma warning ( pop )" << std::endl
		<< "#endif /* _MSC_VER */" << std::endl;
}
