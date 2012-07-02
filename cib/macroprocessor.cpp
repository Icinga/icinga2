#include "i2-cib.h"

using namespace icinga;

string MacroProcessor::ResolveMacros(const string& str, const Dictionary::Ptr& macros)
{
	string::size_type offset, pos_first, pos_second;

	offset = 0;

	string result = str;
	while ((pos_first = result.find_first_of('$', offset)) != string::npos) {
		pos_second = result.find_first_of('$', pos_first + 1);

		if (pos_second == string::npos)
			throw runtime_error("Closing $ not found in macro format string.");

		string name = result.substr(pos_first + 1, pos_second - pos_first - 1);
		string value;
		if (!macros || !macros->GetProperty(name, &value))
			throw runtime_error("Macro '" + name + "' is not defined.");

		result.replace(pos_first, pos_second - pos_first + 1, value);

		offset = pos_first + value.size();
	}

	return result;
}
