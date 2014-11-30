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
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <map>
#include <set>
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
	HandleMissingValidators();

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

			std::cout << "\t\t\t" << "case " << num << ":" << std::endl
				<< "\t\t\t\t" << "return Field(" << num << ", \"" << ftype << "\", \"" << it->Name << "\", " << nameref << ", " << it->Attributes << ");" << std::endl;
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

	/* GetLoadDependencies */
	std::cout << "\t" << "virtual std::vector<String> GetLoadDependencies(void) const" << std::endl
		  << "\t" << "{" << std::endl
		  << "\t\t" << "std::vector<String> deps;" << std::endl;

	for (std::vector<std::string>::const_iterator itd = klass.LoadDependencies.begin(); itd != klass.LoadDependencies.end(); itd++)
		std::cout << "\t\t" << "deps.push_back(\"" << *itd << "\");" << std::endl;

	std::cout << "\t\t" << "return deps;" << std::endl
		  << "\t" << "}" << std::endl;

	std::cout << "};" << std::endl << std::endl;

	std::cout << std::endl;

	/* ObjectImpl */
	std::cout << "template<>" << std::endl
		  << "class ObjectImpl<" << klass.Name << ">"
		  << " : public " << (klass.Parent.empty() ? "Object" : klass.Parent) << std::endl
		  << "{" << std::endl
		  << "public:" << std::endl
		  << "\t" << "DECLARE_PTR_TYPEDEFS(ObjectImpl<" << klass.Name << ">);" << std::endl << std::endl;

	/* Validate */
	std::cout << "\t" << "virtual void Validate(int types, const ValidationUtils& utils)" << std::endl
		<< "\t" << "{" << std::endl;

	if (!klass.Parent.empty())
		std::cout << "\t\t" << klass.Parent << "::Validate(types, utils);" << std::endl << std::endl;

	for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
		std::cout << "\t\t" << "if (" << (it->Attributes & (FAEphemeral|FAConfig|FAState)) << " & types)" << std::endl
			  << "\t\t\t" << "Validate" << it->GetFriendlyName() << "(Get" << it->GetFriendlyName() << "(), utils);" << std::endl;
	}

	std::cout << "\t" << "}" << std::endl << std::endl;

	for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
		std::cout << "\t" << "inline void SimpleValidate" << it->GetFriendlyName() << "(" << it->Type.GetArgumentType() << " value, const ValidationUtils& utils)" << std::endl
			  << "\t" << "{" << std::endl;

		const Field& field = *it;

		if ((field.Attributes & (FARequired)) || field.Type.IsName) {
			if (field.Attributes & FARequired) {
				if (field.Type.GetRealType().find("::Ptr") != std::string::npos)
					std::cout << "\t\t" << "if (!value)" << std::endl;
				else
					std::cout << "\t\t" << "if (value.IsEmpty())" << std::endl;

				std::cout << "\t\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of(\"" << field.Name << "\"), \"Attribute must not be empty.\"));" << std::endl << std::endl;
			}

			if (field.Type.IsName) {
				std::cout << "\t\t" << "String ref = value;" << std::endl
					  << "\t\t" << "if (!ref.IsEmpty() && !utils.ValidateName(\"" << field.Type.TypeName << "\", ref))" << std::endl
					  << "\t\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of(\"" << field.Name << "\"), \"Object '\" + ref + \"' of type '" << field.Type.TypeName
					  << "' does not exist.\"));" << std::endl;
			}
		}

		std::cout << "\t" << "}" << std::endl << std::endl;
	}

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
				std::cout << "static_cast<" << it->Type.GetRealType() << ">(static_cast<int>(";

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
			    << "\t" << "virtual " << it->Type.GetRealType() << " Get" << it->GetFriendlyName() << "(void) const";

			if (it->PureGetAccessor) {
				std::cout << " = 0;" << std::endl;
			} else {
				std::cout << std::endl
					  << "\t" << "{" << std::endl;

				if (it->GetAccessor.empty() && !(it->Attributes & FANoStorage))
					std::cout << "\t\t" << "return m_" << it->GetFriendlyName() << ";" << std::endl;
				else
					std::cout << it->GetAccessor << std::endl;

				std::cout << "\t" << "}" << std::endl << std::endl;
			}
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
				  << "\t" << "virtual void Set" << it->GetFriendlyName() << "(" << it->Type.GetArgumentType() << " value)" << std::endl;

			if (it->PureSetAccessor) {
				std::cout << " = 0;" << std::endl;
			} else {
				std::cout << "\t" << "{" << std::endl;

				if (it->SetAccessor.empty() && !(it->Attributes & FANoStorage))
					std::cout << "\t\t" << "m_" << it->GetFriendlyName() << " = value;" << std::endl;
				else
					std::cout << it->SetAccessor << std::endl;

				std::cout << "\t" << "}" << std::endl << std::endl;
			}
		}

		/* default */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::string realType = it->Type.GetRealType();

			std::cout << "private:" << std::endl
					  << "\t" << realType << " GetDefault" << it->GetFriendlyName() << "(void) const" << std::endl
					  << "\t" << "{" << std::endl;

			if (it->DefaultAccessor.empty())
				std::cout << "\t\t" << "return " << realType << "();" << std::endl;
			else
				std::cout << it->DefaultAccessor << std::endl;

			std::cout << "\t" << "}" << std::endl;
		}

		/* validators */
		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			std::cout << "protected:" << std::endl
				  << "\t" << "virtual void Validate" << it->GetFriendlyName() << "(" << it->Type.GetArgumentType() << " value, const ValidationUtils& utils);" << std::endl;
		}

		/* instance variables */
		std::cout << "private:" << std::endl;

		for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
			if (!(it->Attributes & FANoStorage))
				std::cout << "\t" << it->Type.GetRealType() << " m_" << it->GetFriendlyName() << ";" << std::endl;
		}
	}

	if (klass.Name == "DynamicObject")
		std::cout << "\t" << "friend class ConfigItem;" << std::endl;

	if (!klass.TypeBase.empty())
		std::cout << "\t" << "friend class " << klass.TypeBase << ";" << std::endl;

	std::cout << "};" << std::endl << std::endl;

	for (it = klass.Fields.begin(); it != klass.Fields.end(); it++) {
		m_MissingValidators[std::make_pair(klass.Name, it->GetFriendlyName())] = *it;
	}
}

enum ValidatorType
{
	ValidatorField,
	ValidatorArray,
	ValidatorDictionary
};

static void CodeGenValidator(const std::string& name, const std::string& klass, const std::vector<Rule>& rules, const std::string& field, const FieldType& fieldType, ValidatorType validatorType)
{
	std::cout << "inline void TIValidate" << name << "(const intrusive_ptr<ObjectImpl<" << klass << "> >& object, ";

	if (validatorType != ValidatorField)
		std::cout << "const String& key, ";

	bool static_known_attribute = false;

	std::cout << fieldType.GetArgumentType() << " value, std::vector<String>& location, const ValidationUtils& utils)" << std::endl
		  << "{" << std::endl;

	if (validatorType == ValidatorField) {
		static_known_attribute = true;

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
				std::cout << "\t" << "if (value.IsEmpty())" << std::endl;
			else
				std::cout << "\t" << "if (!value)" << std::endl;

			if (required)
				std::cout << "BOOST_THROW_EXCEPTION(ValidationError(this, location, \"This attribute must not be empty.\"));" << std::endl;
			else
				std::cout << "\t\t" << "return;" << std::endl;

			std::cout << std::endl;
		}
	}

	if (!static_known_attribute)
		std::cout << "\t" << "bool known_attribute = false;" << std::endl;

	bool type_check = false;

	for (std::vector<Rule>::size_type i = 0; i < rules.size(); i++) {
		const Rule& rule = rules[i];

		if (rule.Attributes & RARequired)
			continue;

		if (validatorType == ValidatorField && rule.Pattern != field)
			continue;

		std::cout << "\t" << "do {" << std::endl;

		if (validatorType != ValidatorField) {
			if (rule.Pattern != "*") {
				if (rule.Pattern.find_first_of("*?") != std::string::npos)
					std::cout << "\t\t" << "if (!Utility::Match(\"" << rule.Pattern << "\", key))" << std::endl;
				else
					std::cout << "\t\t" << "if (key != \"" << rule.Pattern << "\")" << std::endl;

				std::cout << "\t\t\t" << "break;" << std::endl;
			} else
				static_known_attribute = true;

			if (!static_known_attribute)
				std::cout << "\t\t" << "known_attribute = true;" << std::endl;
		}

		if (rule.IsName) {
			std::cout << "\t\t" << "if (value.IsScalar()) {" << std::endl
				  << "\t\t\t" << "if (utils.ValidateName(\"" << rule.Type << "\", value))" << std::endl
				  << "\t\t\t\t" << "return;" << std::endl
				  << "\t\t\t" << "else" << std::endl
				  << "\t\t\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(object, location, \"Object '\" + value + \"' of type '" << rule.Type << "' does not exist.\"));" << std::endl
				  << "\t\t" << "}" << std::endl;
		}

		if (fieldType.GetRealType() == "Value") {
			if (rule.Type == "String")
				std::cout << "\t\t" << "if (value.IsScalar())" << std::endl
					  << "\t\t\t" << "return;" << std::endl;
			else if (rule.Type == "Number") {
				std::cout << "\t\t" << "try {" << std::endl
					  << "\t\t\t" << "Convert::ToDouble(value);" << std::endl
					  << "\t\t\t" << "return;" << std::endl
					  << "\t\t" << "} catch (...) { }" << std::endl;
			}
		}

		if (rule.Type == "Dictionary" || rule.Type == "Array" || rule.Type == "Function") {
			if (fieldType.GetRealType() == "Value") {
				std::cout << "\t\t" << "if (value.IsObjectType<" << rule.Type << ">()) {" << std::endl;
				type_check = true;
			} else if (fieldType.GetRealType() != rule.Type + "::Ptr") {
				std::cout << "\t\t" << "if (dynamic_pointer_cast<" << rule.Type << ">(value)) {" << std::endl;
				type_check = true;
			}

			if (!rule.Rules.empty()) {
				bool indent = false;

				if (rule.Type == "Dictionary") {
					if (type_check)
						std::cout << "\t\t\t" << "Dictionary::Ptr dict = value;" << std::endl;
					else
						std::cout << "\t\t" << "const Dictionary::Ptr& dict = value;" << std::endl;

					std::cout << (type_check ? "\t" : "") << "\t\t" << "ObjectLock olock(dict);" << std::endl
						  << (type_check ? "\t" : "") << "\t\t" << "BOOST_FOREACH(const Dictionary::Pair& kv, dict) {" << std::endl
						  << (type_check ? "\t" : "") << "\t\t\t" << "const String& akey = kv.first;" << std::endl
						  << (type_check ? "\t" : "") << "\t\t\t" << "const Value& avalue = kv.second;" << std::endl;
					indent = true;
				} else if (rule.Type == "Array") {
					if (type_check)
						std::cout << "\t\t\t" << "Array::Ptr arr = value;" << std::endl;
					else
						std::cout << "\t\t" << "const Array::Ptr& arr = value;" << std::endl;

					std::cout << (type_check ? "\t" : "") << "\t\t" << "Array::SizeType anum = 0;" << std::endl
						  << (type_check ? "\t" : "") << "\t\t" << "ObjectLock olock(arr);" << std::endl
						  << (type_check ? "\t" : "") << "\t\t" << "BOOST_FOREACH(const Value& avalue, arr) {" << std::endl
						  << (type_check ? "\t" : "") << "\t\t\t" << "String akey = Convert::ToString(anum);" << std::endl;
					indent = true;
				} else {
					std::cout << (type_check ? "\t" : "") << "\t\t" << "String akey = \"\";" << std::endl
						  << (type_check ? "\t" : "") << "\t\t" << "const Value& avalue = value;" << std::endl;
				}

				std::string subvalidator_prefix;

				if (validatorType == ValidatorField)
					subvalidator_prefix = klass;
				else
					subvalidator_prefix = name;

				std::cout << (type_check ? "\t" : "") << (indent ? "\t" : "") << "\t\t" << "location.push_back(akey);" << std::endl
					  << (type_check ? "\t" : "") << (indent ? "\t" : "") << "\t\t" << "TIValidate" << subvalidator_prefix << "_" << i << "(object, akey, avalue, location, utils);" << std::endl;

				if (rule.Type == "Array")
					std::cout << (type_check ? "\t" : "") << "\t\t\t" << "anum++;" << std::endl;

				if (rule.Type == "Dictionary" || rule.Type == "Array")
					std::cout << (type_check ? "\t" : "") << "\t\t" << "}" << std::endl;

				for (std::vector<Rule>::size_type i = 0; i < rule.Rules.size(); i++) {
					const Rule& srule = rule.Rules[i];

					if ((srule.Attributes & RARequired) == 0)
						continue;

					if (rule.Type == "Dictionary") {
						std::cout << (type_check ? "\t" : "") << "\t\t" << "if (dict.Get(\"" << srule.Pattern << "\").IsEmpty())" << std::endl
							  << (type_check ? "\t" : "") << "\t\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(this, location, \"Required dictionary item '" << srule.Pattern << "' is not set.\"));" << std::endl;
					} else if (rule.Type == "Array") {
						int index = -1;
						std::stringstream idxbuf;
						idxbuf << srule.Pattern;
						idxbuf >> index;

						if (index == -1) {
							std::cerr << "Invalid index for 'required' keyword: " << srule.Pattern;
							std::exit(1);
						}

						std::cout << (type_check ? "\t" : "") << "\t\t" << "if (arr.GetLength() < " << (index + 1) << ")" << std::endl
							  << (type_check ? "\t" : "") << "\t\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(this, location, \"Required index '" << index << "' is not set.\"));" << std::endl;
					}
				}

				std::cout << (type_check ? "\t" : "") << (indent ? "\t" : "") << "\t\t" << "location.pop_back();" << std::endl;
			}

			std::cout << (type_check ? "\t" : "") << "\t\t" << "return;" << std::endl;

			if (fieldType.GetRealType() == "Value" || fieldType.GetRealType() != rule.Type + "::Ptr")
				std::cout << "\t\t" << "}" << std::endl;
		}

		std::cout << "\t" << "} while (0);" << std::endl << std::endl;
	}

	if (type_check || validatorType != ValidatorField) {
		if (!static_known_attribute)
			std::cout << "\t" << "if (!known_attribute)" << std::endl
				  << "\t\t" << "BOOST_THROW_EXCEPTION(ValidationError(object, location, \"Invalid attribute: \" + key));" << std::endl
				  << "\t" << "else" << std::endl;

		std::cout << (!static_known_attribute ? "\t" : "") << "\t" << "BOOST_THROW_EXCEPTION(ValidationError(object, boost::assign::list_of(";

		if (validatorType == ValidatorField)
			std::cout << "\"" << field << "\"";
		else
			std::cout << "key";

		std::cout << "), \"Invalid type.\"));" << std::endl;
	}

	std::cout << "}" << std::endl << std::endl;
}

static void CodeGenValidatorSubrules(const std::string& name, const std::string& klass, const std::vector<Rule>& rules)
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
		std::cout << "inline void ObjectImpl<" << it->first.first << ">::Validate" << it->first.second << "(" << it->second.Type.GetArgumentType() << " value, const ValidationUtils& utils)" << std::endl
			  << "{" << std::endl
			  << "\t" << "SimpleValidate" << it->first.second << "(value, utils);" << std::endl
			  << "\t" << "std::vector<String> location;" << std::endl
			  << "\t" << "location.push_back(\"" << it->second.Name << "\");" << std::endl
			  << "\t" << "TIValidate" << it->first.first << it->first.second << "(this, value, location, utils);" << std::endl
			  << "}" << std::endl << std::endl;
	}

	m_MissingValidators.clear();
}

void ClassCompiler::HandleMissingValidators(void)
{
	for (std::map<std::pair<std::string, std::string>, Field>::const_iterator it = m_MissingValidators.begin(); it != m_MissingValidators.end(); it++) {
		std::cout << "inline void ObjectImpl<" << it->first.first << ">::Validate" << it->first.second << "(" << it->second.Type.GetArgumentType() << " value, const ValidationUtils& utils)" << std::endl
			  << "{" << std::endl
			  << "\t" << "SimpleValidate" << it->first.second << "(value, utils);" << std::endl
			  << "}" << std::endl << std::endl;
	}

	m_MissingValidators.clear();
}

void ClassCompiler::CompileFile(const std::string& path)
{
	std::ifstream stream;
	stream.open(path.c_str(), std::ifstream::in);

	if (!stream)
		throw std::invalid_argument("Could not open config file: " + path);

	return CompileStream(path, &stream);
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
			  << "#include \"base/convert.hpp\"" << std::endl
			  << "#include \"base/exception.hpp\"" << std::endl
			  << "#include \"base/objectlock.hpp\"" << std::endl
			  << "#include \"base/utility.hpp\"" << std::endl << std::endl
			  << "#include <boost/foreach.hpp>" << std::endl
			  << "#include <boost/assign/list_of.hpp>" << std::endl
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
