/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef CONFIGWRITER_H
#define CONFIGWRITER_H

#include "base/object.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include <fstream>

namespace icinga
{

/**
 * A config identifier.
 *
 * @ingroup base
 */
class I2_BASE_API ConfigIdentifier : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigIdentifier);

	ConfigIdentifier(const String& name);

	String GetName(void) const;

private:
	String m_Name;
};

/**
 * A configuration writer.
 *
 * @ingroup base
 */
class I2_BASE_API ConfigWriter
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

	static const std::vector<String>& GetKeywords(void);
private:
	static String EscapeIcingaString(const String& str);
	ConfigWriter(void);
};

}

#endif /* CONFIGWRITER_H */
