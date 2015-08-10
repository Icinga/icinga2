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

#ifndef CONFIGWRITER_H
#define CONFIGWRITER_H

#include "config/i2-config.hpp"
#include "base/object.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include <fstream>
#include <set>

namespace icinga
{

/**
 * A configuration writer.
 *
 * @ingroup config
 */
class I2_CONFIG_API ConfigWriter : public Object {
public:
	DECLARE_PTR_TYPEDEFS(ConfigWriter);

	ConfigWriter(const String& fileName);

	void EmitBoolean(bool val);
	void EmitNumber(double val);
	void EmitString(const String& val);
	void EmitEmpty(void);
	void EmitArray(const Array::Ptr& val);
	void EmitArrayItems(const Array::Ptr& val);
	void EmitDictionary(const Dictionary::Ptr& val);
	void EmitScope(int indentLevel, const Dictionary::Ptr& val, const Array::Ptr& imports = Array::Ptr());
	void EmitValue(int indentLevel, const Value& val);
	void EmitRaw(const String& val);
	void EmitIndent(int indentLevel);

	void EmitIdentifier(const String& identifier, bool inAssignment);
	void EmitConfigItem(const String& type, const String& name, bool isTemplate,
	    const Array::Ptr& imports, const Dictionary::Ptr& attrs);

	void EmitComment(const String& text);
	void EmitFunctionCall(const String& name, const Array::Ptr& arguments);

private:
	std::ofstream m_FP;

	static String EscapeIcingaString(const String& str);
};

}

#endif /* CONFIGWRITER_H */
