/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef OBJECTLISTUTILITY_H
#define OBJECTLISTUTILITY_H

#include "base/array.hpp"
#include "base/dictionary.hpp"
#include "base/i2-base.hpp"
#include "base/string.hpp"
#include "base/value.hpp"
#include "cli/i2-cli.hpp"

namespace icinga
{

/**
 * @ingroup cli
 */
class ObjectListUtility
{
public:
	static bool PrintObject(std::ostream& fp, bool& first, const String& message, std::map<String, int>& type_count, const String& name_filter, const String& type_filter);

private:
	static void PrintProperties(std::ostream& fp, const Dictionary::Ptr& props, const Dictionary::Ptr& debug_hints, int indent);
	static void PrintHints(std::ostream& fp, const Dictionary::Ptr& debug_hints, int indent);
	static void PrintHint(std::ostream& fp, const Array::Ptr& msg, int indent);
	static void PrintValue(std::ostream& fp, const Value& val);
	static void PrintArray(std::ostream& fp, const Array::Ptr& arr);
};

}

#endif /* OBJECTLISTUTILITY_H */
