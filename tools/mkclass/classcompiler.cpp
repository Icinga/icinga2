/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include <vector>
#include <cstring>
#include <locale>
#ifndef _WIN32
#include <libgen.h>
#else /* _WIN32 */
#include <shlwapi.h>
#endif /* _WIN32 */

using namespace icinga;

ClassCompiler::ClassCompiler(const std::string& path, std::istream& input,
    std::ostream& oimpl, std::ostream& oheader)
	: m_Path(path), m_Input(input), m_Impl(oimpl), m_Header(oheader)
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

void ClassCompiler::HandleLibrary(const std::string& library, const ClassDebugInfo& locp)
{
	m_Library = library;
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
	return TypePreference(a.Type.GetRealType()) < TypePreference(b.Type.GetRealType());
}

static bool FieldTypeCmp(const Field& a, const Field& b)
{
	return a.Type.GetRealType() < b.Type.GetRealType();
}

void ClassCompiler::OptimizeStructLayout(std::vector<Field>& fields)
{
	std::sort(fields.begin(), fields.end(), FieldTypeCmp);
	std::stable_sort(fields.begin(), fields.end(), FieldLayoutCmp);
}

void ClassCompiler::HandleClass(const Klass& klass, const ClassDebugInfo&)
{
	std::vector<Field>::const_iterator it;

	std::string apiMacro;

	if (!m_Library.empty()) {
		std::string libName = m_Library;
		std::locale locale;

		for (std::string::size_type i = 0; i < libName.size(); i++)
			libName[i] = std::toupper(libName[i], locale);

		m_Header << "#ifndef I2_" << libName << "_API" << std::endl
			 << "#	ifdef I2_" << libName << "_BUILD" << std::endl
			 << "#		define I2_" << libName << "_API I2_EXPORT" << std::endl
			 << "#	else /* I2_" << libName << "_BUILD */" << std::endl
			 << "#		define I2_" << libName << "_API I2_IMPORT" << std::endl
			 << "#	endif /* I2_" << libName << "_BUILD */" << std::endl
			 << "#endif /* I2_" << libName << "_API */" << std::endl << std::endl;

		apiMacro = "I2_" + libName + "_API ";
	}

	/* forward declaration */
	if (klass.Name.find_first_of(':') == std::string::npos)
		m_Header << "class " << klass.Name << ";" << std::endl << std::endl;

	/* TypeHelper */
	if (klass.Attributes & TAAbstract) {
		m_Header << "template<>" << std::endl
			 << "struct TypeHelper<" << klass.Name << ">" << std::endl
			 << "{" << std::endl
			 << "\t" << "static ObjectFactory GetFactory(void)" << std::endl
			 << "\t" << "{" << std::endl
			 << "\t\t" << "return NULL;" << std::endl
			 << "\t" << "}" << std::endl
			 << "};" << std::endl << std::endl;
	}

	/* TypeImpl */
	m_Header << "template<>" << std::endl
		 << "class TypeImpl<" << klass.Name << ">"
		 << " : public Type";
	
	if (!klass.TypeBase.empty())
		m_Header << ", public " + klass.TypeBase;

	m_Header << std::endl
		 << "{" << std::endl
		 << "public:" << std::endl;

	m_Impl << "template class TypeImpl<" << klass.Name << ">;" << std::endl << std::endl;

	/* GetName */
	m_Header << "\t" << "virtual String GetName(void) const;" << std::endl;

	m_Impl << "String TypeImpl<" << klass.Name << ">::GetName(void) const" << std::endl
	       << "{" << std::endl
	       << "\t" << "return \"" << klass.Name << "\";" << std::endl
	       << "}" << std::endl << std::endl;

	/* GetAttributes */
	m_Header << "\t" << "virtual int GetAttributes(void) const;" << std::endl;

	m_Impl << "int TypeImpl<" << klass.Name << ">::GetAttributes(void) const" << std::endl
	       << "{" << std::endl
	       << "\t" << "return " << klass.Attributes << ";" << std::endl
	       << "}" << std::endl << std::endl;

	/* GetBaseType */
	m_Header << "\t" << "virtual Type::Ptr GetBaseType(void) const;" << std::endl;

	m_Impl << "Type::Ptr TypeImpl<" << klass.Name << ">::GetBaseType(void) const" << std::endl
	       << "{" << std::endl
	       << "\t" << "return ";

	if (!klass.Parent.empty())
		m_Impl << klass.Parent << "::TypeInstance";
	else
		m_Impl << "Object::TypeInstance";

	m_Impl << ";" << std::endl
	       << "}" << std::endl << std::endl;

	/* GetFieldId */
	m_Header << "\t" << "virtual int GetFieldId(const String& name) const;" << std::endl;

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

		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			int hash = static_cast<int>(SDBM(it->Name, hlen));
			jumptable[hash].push_back(std::make_pair(num, it->Name));
			num++;

			if (jumptable[hash].size() > 1)
				collisions++;
		}
	} while (collisions >= 5 && hlen < 8);

	if (!klass.Fields.empty()) {
		m_Impl << "\tswitch (static_cast<int>(Utility::SDBM(name, " << hlen << "))) {" << std::endl;

		std::map<int, std::vector<std::pair<int, std::string> > >::const_iterator itj;

		for (itj = jumptable.begin(); itj != jumptable.end(); itj++) {
			m_Impl << "\t\tcase " << itj->first << ":" << std::endl;

			std::vector<std::pair<int, std::string> >::const_iterator itf;

			for (itf = itj->second.begin(); itf != itj->second.end(); itf++) {
				m_Impl << "\t\t\t" << "if (name == \"" << itf->second << "\")" << std::endl
				       << "\t\t\t\t" << "return offset + " << itf->first << ";" << std::endl;
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
	m_Header << "\t" << "virtual Field GetFieldInfo(int id) const;" << std::endl;

	m_Impl << "Field TypeImpl<" << klass.Name << ">::GetFieldInfo(int id) const" << std::endl
	       << "{" << std::endl;

	if (!klass.Parent.empty())
		m_Impl << "\t" << "int real_id = id - " << klass.Parent << "::TypeInstance->GetFieldCount();" << std::endl
		       << "\t" << "if (real_id < 0) { return " << klass.Parent << "::TypeInstance->GetFieldInfo(id); }" << std::endl;

	if (klass.Fields.size() > 0) {
		m_Impl << "\t" << "switch (";

		if (!klass.Parent.empty())
			m_Impl << "real_id";
		else
			m_Impl << "id";

		m_Impl << ") {" << std::endl;

		size_t num = 0;
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::string ftype = it->Type.GetRealType();

			if (ftype == "bool" || ftype == "int" || ftype == "double")
				ftype = "Number";

			if (ftype == "int" || ftype == "double")
				ftype = "Number";
			else if (ftype == "bool")
				ftype = "Boolean";

			if (ftype.find("::Ptr") != std::string::npos)
				ftype = ftype.substr(0, ftype.size() - strlen("::Ptr"));

			if (it->Attributes & FAEnum)
				ftype = "Number";

			std::string nameref;

			if (it->Type.IsName)
				nameref = "\"" + it->Type.TypeName + "\"";
			else
				nameref = "NULL";

			m_Impl << "\t\t" << "case " << num << ":" << std::endl
				 << "\t\t\t" << "return Field(" << num << ", \"" << ftype << "\", \"" << it->Name << "\", \"" << (it->NavigationName.empty() ? it->Name : it->NavigationName) << "\", "  << nameref << ", " << it->Attributes << ", " << it->Type.ArrayRank << ");" << std::endl;
			num++;
		}

		m_Impl << "\t\t" << "default:" << std::endl
			 << "\t\t";
	}

	m_Impl << "\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl;

	if (klass.Fields.size() > 0)
		m_Impl << "\t" << "}" << std::endl;

	m_Impl << "}" << std::endl << std::endl;

	/* GetFieldCount */
	m_Header << "\t" << "virtual int GetFieldCount(void) const;" << std::endl;

	m_Impl << "int TypeImpl<" << klass.Name << ">::GetFieldCount(void) const" << std::endl
	       << "{" << std::endl
	       << "\t" << "return " << klass.Fields.size();

	if (!klass.Parent.empty())
		m_Impl << " + " << klass.Parent << "::TypeInstance->GetFieldCount()";

	m_Impl << ";" << std::endl
	       << "}" << std::endl << std::endl;

	/* GetFactory */
	m_Header << "\t" << "virtual ObjectFactory GetFactory(void) const;" << std::endl;

	m_Impl << "ObjectFactory TypeImpl<" << klass.Name << ">::GetFactory(void) const" << std::endl
	       << "{" << std::endl
	       << "\t" << "return TypeHelper<" << klass.Name << ">::GetFactory();" << std::endl
	       << "}" << std::endl << std::endl;

	/* GetLoadDependencies */
	m_Header << "\t" << "virtual std::vector<String> GetLoadDependencies(void) const;" << std::endl;

	m_Impl << "std::vector<String> TypeImpl<" << klass.Name << ">::GetLoadDependencies(void) const" << std::endl
	       << "{" << std::endl
	       << "\t" << "std::vector<String> deps;" << std::endl;

	for (std::vector<std::string>::const_iterator itd = klass.LoadDependencies.begin(); itd != klass.LoadDependencies.end(); itd++)
		m_Impl << "\t" << "deps.push_back(\"" << *itd << "\");" << std::endl;

	m_Impl << "\t" << "return deps;" << std::endl
	       << "}" << std::endl << std::endl;

	/* RegisterAttributeHandler */
	m_Header << "public:" << std::endl
		 << "\t" << "virtual void RegisterAttributeHandler(int fieldId, const Type::AttributeHandler& callback);" << std::endl;

	m_Impl << "void TypeImpl<" << klass.Name << ">::RegisterAttributeHandler(int fieldId, const Type::AttributeHandler& callback)" << std::endl
	       << "{" << std::endl;

	if (!klass.Parent.empty())
		m_Impl << "\t" << "int real_id = fieldId - " << klass.Parent << "::TypeInstance->GetFieldCount(); " << std::endl
		       << "\t" << "if (real_id < 0) { " << klass.Parent << "::TypeInstance->RegisterAttributeHandler(fieldId, callback); return; }" << std::endl;

	m_Impl << "\t" << "switch (";

	if (!klass.Parent.empty())
		m_Impl << "real_id";
	else
		m_Impl << "fieldId";

	m_Impl << ") {" << std::endl;

	int num = 0;
	for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
		m_Impl << "\t\t" << "case " << num << ":" << std::endl
		       << "\t\t\t" << "ObjectImpl<" << klass.Name << ">::On" << it->GetFriendlyName() << "Changed.connect(callback);" << std::endl
		       << "\t\t\t" << "break;" << std::endl;
		num++;
	}

	m_Impl << "\t\t" << "default:" << std::endl
	       << "\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
	       << "\t" << "}" << std::endl;

	m_Impl << "}" << std::endl << std::endl;
		
	m_Header << "};" << std::endl << std::endl;

	m_Header << std::endl;

	/* ObjectImpl */
	m_Header << "template<>" << std::endl
		 << "class " << apiMacro << "ObjectImpl<" << klass.Name << ">"
		 << " : public " << (klass.Parent.empty() ? "Object" : klass.Parent) << std::endl
		 << "{" << std::endl
		 << "public:" << std::endl
		 << "\t" << "DECLARE_PTR_TYPEDEFS(ObjectImpl<" << klass.Name << ">);" << std::endl << std::endl;

	m_Impl << "template class ObjectImpl<" << klass.Name << ">;" << std::endl << std::endl;

	/* Validate */
	m_Header << "\t" << "virtual void Validate(int types, const ValidationUtils& utils) override;" << std::endl;

	m_Impl << "void ObjectImpl<" << klass.Name << ">::Validate(int types, const ValidationUtils& utils)" << std::endl
	       << "{" << std::endl;

	if (!klass.Parent.empty())
		m_Impl << "\t" << klass.Parent << "::Validate(types, utils);" << std::endl << std::endl;

	for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
		m_Impl << "\t" << "if (" << (it->Attributes & (FAEphemeral|FAConfig|FAState)) << " & types)" << std::endl
			 << "\t\t" << "Validate" << it->GetFriendlyName() << "(Get" << it->GetFriendlyName() << "(), utils);" << std::endl;
	}

	m_Impl << "}" << std::endl << std::endl;

	for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
		m_Header << "\t" << "void SimpleValidate" << it->GetFriendlyName() << "(" << it->Type.GetArgumentType() << " value, const ValidationUtils& utils);" << std::endl;

		m_Impl << "void ObjectImpl<" << klass.Name << ">::SimpleValidate" << it->GetFriendlyName() << "(" << it->Type.GetArgumentType() << " value, const ValidationUtils& utils)" << std::endl
		       << "{" << std::endl;

		const Field& field = *it;

		if ((field.Attributes & (FARequired)) || field.Type.IsName) {
			if (field.Attributes & FARequired) {
				if (field.Type.GetRealType().find("::Ptr") != std::string::npos)
					m_Impl << "\t" << "if (!value)" << std::endl;
				else
					m_Impl << "\t" << "if (value.IsEmpty())" << std::endl;

				m_Impl << "\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_cast<ConfigObject *>(this), boost::assign::list_of(\"" << field.Name << "\"), \"Attribute must not be empty.\"));" << std::endl << std::endl;
			}

			if (field.Type.IsName) {
				if (field.Type.ArrayRank > 0) {
					m_Impl << "\t" << "if (value) {" << std::endl
					       << "\t\t" << "ObjectLock olock(value);" << std::endl
					       << "\t\t" << "BOOST_FOREACH(const String& ref, value) {" << std::endl;
				} else
					m_Impl << "\t" << "String ref = value;" << std::endl;

				m_Impl << "\t" << "if (!ref.IsEmpty() && !utils.ValidateName(\"" << field.Type.TypeName << "\", ref))" << std::endl
				       << "\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(dynamic_cast<ConfigObject *>(this), boost::assign::list_of(\"" << field.Name << "\"), \"Object '\" + ref + \"' of type '" << field.Type.TypeName
				       << "' does not exist.\"));" << std::endl;

				if (field.Type.ArrayRank > 0) {
					m_Impl << "\t\t" << "}" << std::endl
					       << "\t" << "}" << std::endl;
				}
			}
		}

		m_Impl << "}" << std::endl << std::endl;
	}

	if (!klass.Fields.empty()) {
		/* constructor */
		m_Header << "public:" << std::endl
			 << "\t" << "ObjectImpl<" << klass.Name << ">(void);" << std::endl;

		m_Impl << "ObjectImpl<" << klass.Name << ">::ObjectImpl(void)" << std::endl
		       << "{" << std::endl;

		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			m_Impl << "\t" << "Set" << it->GetFriendlyName() << "(" << "GetDefault" << it->GetFriendlyName() << "(), true);" << std::endl;
		}

		m_Impl << "}" << std::endl << std::endl;

		/* destructor */
		m_Header << "public:" << std::endl
			 << "\t" << "~ObjectImpl<" << klass.Name << ">(void);" << std::endl;

		m_Impl << "ObjectImpl<" << klass.Name << ">::~ObjectImpl(void)" << std::endl
		       << "{ }" << std::endl << std::endl;

		/* SetField */
		m_Header << "public:" << std::endl
			 << "\t" << "virtual void SetField(int id, const Value& value, bool suppress_events = false, const Value& cookie = Empty) override;" << std::endl;

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
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			m_Impl << "\t\t" << "case " << num << ":" << std::endl
			       << "\t\t\t" << "Set" << it->GetFriendlyName() << "(";
			
			if (it->Attributes & FAEnum)
				m_Impl << "static_cast<" << it->Type.GetRealType() << ">(static_cast<int>(";

			m_Impl << "value";
			
			if (it->Attributes & FAEnum)
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
			 << "\t" << "virtual Value GetField(int id) const override;" << std::endl;

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
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			m_Impl << "\t\t" << "case " << num << ":" << std::endl
			       << "\t\t\t" << "return Get" << it->GetFriendlyName() << "();" << std::endl;
			num++;
		}

		m_Impl << "\t\t" << "default:" << std::endl
		       << "\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
		       << "\t" << "}" << std::endl;

		m_Impl << "}" << std::endl << std::endl;
		
		/* ValidateField */
		m_Header << "public:" << std::endl
			 << "\t" << "virtual void ValidateField(int id, const Value& value, const ValidationUtils& utils) override;" << std::endl;

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
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			m_Impl << "\t\t" << "case " << num << ":" << std::endl
			       << "\t\t\t" << "Validate" << it->GetFriendlyName() << "(";
			
			if (it->Attributes & FAEnum)
				m_Impl << "static_cast<" << it->Type.GetRealType() << ">(static_cast<int>(";

			m_Impl << "value";
			
			if (it->Attributes & FAEnum)
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
			 << "\t" << "virtual void NotifyField(int id, const Value& cookie = Empty) override;" << std::endl;

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
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			m_Impl << "\t\t" << "case " << num << ":" << std::endl
			       << "\t\t\t" << "Notify" << it->GetFriendlyName() << "(cookie);" << std::endl
			       << "\t\t\t" << "break;" << std::endl;
			num++;
		}

		m_Impl << "\t\t" << "default:" << std::endl
		       << "\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
		       << "\t" << "}" << std::endl;

		m_Impl << "}" << std::endl << std::endl;

		/* NavigateField */
		m_Header << "public:" << std::endl
			 << "\t" << "virtual Object::Ptr NavigateField(int id) const override;" << std::endl;

		m_Impl << "Object::Ptr ObjectImpl<" << klass.Name << ">::NavigateField(int id) const" << std::endl
		       << "{" << std::endl;

		if (!klass.Parent.empty())
			m_Impl << "\t" << "int real_id = id - " << klass.Parent << "::TypeInstance->GetFieldCount(); " << std::endl
			       << "\t" << "if (real_id < 0) { return " << klass.Parent << "::NavigateField(id); }" << std::endl;

		m_Impl << "\t" << "switch (";

		if (!klass.Parent.empty())
			m_Impl << "real_id";
		else
			m_Impl << "id";

		m_Impl << ") {" << std::endl;

		num = 0;
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			if (it->Attributes & FANavigation) {
				m_Impl << "\t\t" << "case " << num << ":" << std::endl
				       << "\t\t\t" << "return Navigate" << it->GetFriendlyName() << "();" << std::endl;
			}

			num++;
		}

		m_Impl << "\t\t" << "default:" << std::endl
		       << "\t\t\t" << "throw std::runtime_error(\"Invalid field ID.\");" << std::endl
		       << "\t" << "}" << std::endl;

		m_Impl << "}" << std::endl << std::endl;

		/* getters */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::string prot;

			if (it->Attributes & FAGetProtected)
				prot = "protected";
			else
				prot = "public";

			m_Header << prot << ":" << std::endl
			 	 << "\t" << "virtual " << it->Type.GetRealType() << " Get" << it->GetFriendlyName() << "(void) const";

			if (it->PureGetAccessor) {
				m_Header << " = 0;" << std::endl;
			} else {
				m_Header << ";" << std::endl;

				m_Impl << it->Type.GetRealType() << " ObjectImpl<" << klass.Name << ">::Get" << it->GetFriendlyName() << "(void) const" << std::endl
				       << "{" << std::endl;

				if (it->GetAccessor.empty() && !(it->Attributes & FANoStorage))
					m_Impl << "\t" << "return m_" << it->GetFriendlyName() << ";" << std::endl;
				else
					m_Impl << it->GetAccessor << std::endl;

				m_Impl << "}" << std::endl << std::endl;
			}
		}

		/* setters */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::string prot;

			if (it->Attributes & FASetProtected)
				prot = "protected";
			else
				prot = "public";

			m_Header << prot << ":" << std::endl
				 << "\t" << "virtual void Set" << it->GetFriendlyName() << "(" << it->Type.GetArgumentType() << " value, bool suppress_events = false, const Value& cookie = Empty)";

			if (it->PureSetAccessor) {
				m_Header << " = 0;" << std::endl;
			} else {
				m_Header << ";" << std::endl;

				m_Impl << "void ObjectImpl<" << klass.Name << ">::Set" << it->GetFriendlyName() << "(" << it->Type.GetArgumentType() << " value, bool suppress_events, const Value& cookie)" << std::endl
				       << "{" << std::endl;

				if (it->Type.IsName || !it->TrackAccessor.empty())
					m_Impl << "\t" << "Value oldValue = Get" << it->GetFriendlyName() << "();" << std::endl;

					
				if (it->SetAccessor.empty() && !(it->Attributes & FANoStorage))
					m_Impl << "\t" << "m_" << it->GetFriendlyName() << " = value;" << std::endl;
				else
					m_Impl << it->SetAccessor << std::endl << std::endl;

				if (it->Type.IsName || !it->TrackAccessor.empty()) {
					if (it->Name != "active") {
						m_Impl << "\t" << "ConfigObject *dobj = dynamic_cast<ConfigObject *>(this);" << std::endl
						       << "\t" << "if (!dobj || dobj->IsActive())" << std::endl
						       << "\t";
					}

					m_Impl << "\t" << "Track" << it->GetFriendlyName() << "(oldValue, value);" << std::endl;
				}

				m_Impl << "\t" << "if (!suppress_events)" << std::endl
				       << "\t\t" << "Notify" << it->GetFriendlyName() << "(cookie);" << std::endl
				       << "}" << std::endl << std::endl;
			}
		}

		m_Header << "protected:" << std::endl;

		bool needs_tracking = false;

		/* tracking */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			if (!it->Type.IsName && it->TrackAccessor.empty())
				continue;

			needs_tracking = true;

			m_Header << "\t" << "virtual void Track" << it->GetFriendlyName() << "(" << it->Type.GetArgumentType() << " oldValue, " << it->Type.GetArgumentType() << " newValue);";

			m_Impl << "void ObjectImpl<" << klass.Name << ">::Track" << it->GetFriendlyName() << "(" << it->Type.GetArgumentType() << " oldValue, " << it->Type.GetArgumentType() << " newValue)" << std::endl
			       << "{" << std::endl;

			if (!it->TrackAccessor.empty())
				m_Impl << "\t" << it->TrackAccessor << std::endl;

			if (it->Type.ArrayRank > 0) {
				m_Impl << "\t" << "if (oldValue) {" << std::endl
				       << "\t\t" << "ObjectLock olock(oldValue);" << std::endl
				       << "\t\t" << "BOOST_FOREACH(const String& ref, oldValue) {" << std::endl
				       << "\t\t\t" << "DependencyGraph::RemoveDependency(this, ConfigObject::GetObject(\"" << it->Type.TypeName << "\", ref).get());" << std::endl
				       << "\t\t" << "}" << std::endl
				       << "\t" << "}" << std::endl
				       << "\t" << "if (newValue) {" << std::endl
				       << "\t\t" << "ObjectLock olock(newValue);" << std::endl
				       << "\t\t" << "BOOST_FOREACH(const String& ref, newValue) {" << std::endl
				       << "\t\t\t" << "DependencyGraph::AddDependency(this, ConfigObject::GetObject(\"" << it->Type.TypeName << "\", ref).get());" << std::endl
				       << "\t\t" << "}" << std::endl
				       << "\t" << "}" << std::endl;
			} else {
				m_Impl << "\t" << "if (!oldValue.IsEmpty())" << std::endl
				       << "\t\t" << "DependencyGraph::RemoveDependency(this, ConfigObject::GetObject(\"" << it->Type.TypeName << "\", oldValue).get());" << std::endl
				       << "\t" << "if (!newValue.IsEmpty())" << std::endl
				       << "\t\t" << "DependencyGraph::AddDependency(this, ConfigObject::GetObject(\"" << it->Type.TypeName << "\", newValue).get());" << std::endl;
			}

			m_Impl << "}" << std::endl << std::endl;
		}

		/* navigation */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			if ((it->Attributes & FANavigation) == 0)
				continue;

			m_Header << "public:" << std::endl
				 << "\t" << "virtual Object::Ptr Navigate" << it->GetFriendlyName() << "(void) const";

			if (it->PureNavigateAccessor) {
				m_Header << " = 0;" << std::endl;
			} else {
				m_Header << ";" << std::endl;

				m_Impl << "Object::Ptr ObjectImpl<" << klass.Name << ">::Navigate" << it->GetFriendlyName() << "(void) const" << std::endl
				       << "{" << std::endl;

				if (it->NavigateAccessor.empty())
					m_Impl << "\t" << "return Get" << it->GetFriendlyName() << "();" << std::endl;
				else
					m_Impl << "\t" << it->NavigateAccessor << std::endl;

				m_Impl << "}" << std::endl << std::endl;
			}
		}

		/* start/stop */
		if (needs_tracking) {
			m_Header << "virtual void Start(bool runtimeCreated = false) override;" << std::endl
				 << "virtual void Stop(bool runtimeRemoved = false) override;" << std::endl;

			m_Impl << "void ObjectImpl<" << klass.Name << ">::Start(bool runtimeCreated)" << std::endl
			       << "{" << std::endl
			       << "\t" << klass.Parent << "::Start(runtimeCreated);" << std::endl << std::endl;

			for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
				if (!(it->Type.IsName))
					continue;

				m_Impl << "\t" << "Track" << it->GetFriendlyName() << "(Empty, Get" << it->GetFriendlyName() << "());" << std::endl;
			}

			m_Impl << "}" << std::endl << std::endl
			       << "void ObjectImpl<" << klass.Name << ">::Stop(bool runtimeRemoved)" << std::endl
			       << "{" << std::endl
			       << "\t" << klass.Parent << "::Stop(runtimeRemoved);" << std::endl << std::endl;

			for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
				if (!(it->Type.IsName))
					continue;

				m_Impl << "\t" << "Track" << it->GetFriendlyName() << "(Get" << it->GetFriendlyName() << "(), Empty);" << std::endl;
			}

			m_Impl << "}" << std::endl << std::endl;
		}

		/* notify */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::string prot;

			if (it->Attributes & FASetProtected)
				prot = "protected";
			else
				prot = "public";

			m_Header << prot << ":" << std::endl
				 << "\t" << "virtual void Notify" << it->GetFriendlyName() << "(const Value& cookie = Empty);" << std::endl;

			m_Impl << "void ObjectImpl<" << klass.Name << ">::Notify" << it->GetFriendlyName() << "(const Value& cookie)" << std::endl
			       << "{" << std::endl
			       << "\t" << "ConfigObject *dobj = dynamic_cast<ConfigObject *>(this);" << std::endl;

			if (it->Name != "active") {
				m_Impl << "\t" << "if (!dobj || dobj->IsActive())" << std::endl
				       << "\t";
			}

			m_Impl << "\t" << "On" << it->GetFriendlyName() << "Changed(static_cast<" << klass.Name << " *>(this), cookie);" << std::endl
			       << "}" << std::endl << std::endl;
		}
		
		/* default */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::string realType = it->Type.GetRealType();

			m_Header << "private:" << std::endl
				 << "\t" << "inline " << realType << " GetDefault" << it->GetFriendlyName() << "(void) const;" << std::endl;

			m_Impl << realType << " ObjectImpl<" << klass.Name << ">::GetDefault" << it->GetFriendlyName() << "(void) const" << std::endl
			       << "{" << std::endl;

			if (it->DefaultAccessor.empty())
				m_Impl << "\t" << "return " << realType << "();" << std::endl;
			else
				m_Impl << "\t" << it->DefaultAccessor << std::endl;

			m_Impl << "}" << std::endl << std::endl;
		}

		/* validators */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			m_Header << "protected:" << std::endl
				 << "\t" << "virtual void Validate" << it->GetFriendlyName() << "(" << it->Type.GetArgumentType() << " value, const ValidationUtils& utils);" << std::endl;
		}

		/* instance variables */
		m_Header << "private:" << std::endl;

		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			if (it->Attributes & FANoStorage)
				continue;

			m_Header << "\t" << it->Type.GetRealType() << " m_" << it->GetFriendlyName() << ";" << std::endl;
		}
		
		/* signal */
		m_Header << "public:" << std::endl;
		
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			m_Header << "\t" << "static boost::signals2::signal<void (const intrusive_ptr<" << klass.Name << ">&, const Value&)> On" << it->GetFriendlyName() << "Changed;" << std::endl;
			m_Impl << std::endl << "boost::signals2::signal<void (const intrusive_ptr<" << klass.Name << ">&, const Value&)> ObjectImpl<" << klass.Name << ">::On" << it->GetFriendlyName() << "Changed;" << std::endl << std::endl;
		}
	}

	if (klass.Name == "ConfigObject")
		m_Header << "\t" << "friend class ConfigItem;" << std::endl;

	if (!klass.TypeBase.empty())
		m_Header << "\t" << "friend class " << klass.TypeBase << ";" << std::endl;

	m_Header << "};" << std::endl << std::endl;

	for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
		m_MissingValidators[std::make_pair(klass.Name, it->GetFriendlyName())] = *it;
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

		for (std::vector<Rule>::size_type i = 0; i < rules.size(); i++) {
			const Rule& rule = rules[i];

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

	for (std::vector<Rule>::size_type i = 0; i < rules.size(); i++) {
		const Rule& rule = rules[i];

		if (rule.Attributes & RARequired)
			continue;

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
					       << (type_check ? "\t" : "") << "\t\t\t" << "BOOST_FOREACH(const Dictionary::Pair& kv, dict) {" << std::endl
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
					       << (type_check ? "\t" : "") << "\t\t\t" << "BOOST_FOREACH(const Value& avalue, arr) {" << std::endl
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

				for (std::vector<Rule>::size_type i = 0; i < rule.Rules.size(); i++) {
					const Rule& srule = rule.Rules[i];

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
	for (std::vector<Rule>::size_type i = 0; i < rules.size(); i++) {
		const Rule& rule = rules[i];

		if (rule.Attributes & RARequired)
			continue;

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

	for (std::map<std::pair<std::string, std::string>, Field>::const_iterator it = m_MissingValidators.begin(); it != m_MissingValidators.end(); it++)
		CodeGenValidator(it->first.first + it->first.second, it->first.first, validator.Rules, it->second.Name, it->second.Type, ValidatorField);

	for (std::map<std::pair<std::string, std::string>, Field>::const_iterator it = m_MissingValidators.begin(); it != m_MissingValidators.end(); it++) {
		m_Impl << "void ObjectImpl<" << it->first.first << ">::Validate" << it->first.second << "(" << it->second.Type.GetArgumentType() << " value, const ValidationUtils& utils)" << std::endl
		       << "{" << std::endl
		       << "\t" << "SimpleValidate" << it->first.second << "(value, utils);" << std::endl
		       << "\t" << "std::vector<String> location;" << std::endl
		       << "\t" << "location.push_back(\"" << it->second.Name << "\");" << std::endl
		       << "\t" << "TIValidate" << it->first.first << it->first.second << "(this, value, location, utils);" << std::endl
		       << "\t" << "location.pop_back();" << std::endl
		       << "}" << std::endl << std::endl;
	}

	m_MissingValidators.clear();
}

void ClassCompiler::HandleMissingValidators(void)
{
	for (std::map<std::pair<std::string, std::string>, Field>::const_iterator it = m_MissingValidators.begin(); it != m_MissingValidators.end(); it++) {
		m_Impl << "void ObjectImpl<" << it->first.first << ">::Validate" << it->first.second << "(" << it->second.Type.GetArgumentType() << " value, const ValidationUtils& utils)" << std::endl
		       << "{" << std::endl
		       << "\t" << "SimpleValidate" << it->first.second << "(value, utils);" << std::endl
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

	for (std::string::size_type i = 0; i < result.size(); i++) {
		result[i] = toupper(result[i]);

		if (result[i] == '.')
			result[i] = '_';
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
	      << "#include <boost/foreach.hpp>" << std::endl
	      << "#include <boost/assign/list_of.hpp>" << std::endl
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
