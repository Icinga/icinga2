/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/object.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include <iosfwd>

namespace icinga
{

/**
 * A config identifier.
 *
 * @ingroup base
 */
class ConfigIdentifier final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigIdentifier);

	ConfigIdentifier(String name);

	String GetName() const;

private:
	String m_Name;
};

/**
 * A configuration writer.
 *
 * @ingroup base
 */
class ConfigWriter
{
public:
	static void EmitBoolean(std::ostream& fp, bool val);
	static void EmitNumber(std::ostream& fp, double val);
	static void EmitString(std::ostream& fp, const String& val);
	static void EmitEmpty(std::ostream& fp);
	static void EmitArray(std::ostream& fp, int indentLevel, const Array::Ptr& val);
	static void EmitArrayItems(std::ostream& fp, int indentLevel, const Array::Ptr& val);
	static void EmitScope(std::ostream& fp, int indentLevel, const Dictionary::Ptr& val,
		const Array::Ptr& imports = nullptr, bool splitDot = false);
	static void EmitValue(std::ostream& fp, int indentLevel, const Value& val);
	static void EmitRaw(std::ostream& fp, const String& val);
	static void EmitIndent(std::ostream& fp, int indentLevel);

	static void EmitIdentifier(std::ostream& fp, const String& identifier, bool inAssignment);
	static void EmitConfigItem(std::ostream& fp, const String& type, const String& name, bool isTemplate,
		bool ignoreOnError, const Array::Ptr& imports, const Dictionary::Ptr& attrs);

	static void EmitComment(std::ostream& fp, const String& text);
	static void EmitFunctionCall(std::ostream& fp, const String& name, const Array::Ptr& arguments);

	static const std::vector<String>& GetKeywords();
private:
	static String EscapeIcingaString(const String& str);
	ConfigWriter();
};

}
