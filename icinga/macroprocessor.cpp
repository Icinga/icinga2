#include "i2-icinga.h"

using namespace icinga;

string MacroProcessor::ResolveMacros(string str, Dictionary::Ptr macros)
{
	string::size_type offset, pos_first, pos_second;

	offset = 0;

	while ((pos_first = str.find_first_of('$', offset)) != string::npos) {
		pos_second = str.find_first_of('$', pos_first + 1);

		if (pos_second == string::npos)
			throw runtime_error("Closing $ not found in macro format string.");

		string name = str.substr(pos_first + 1, pos_second - pos_first - 1);
		string value;
		if (!macros || !macros->GetProperty(name, &value))
			throw runtime_error("Macro '" + name + "' is not defined.");

		str.replace(pos_first, pos_second - pos_first + 1, value);

		offset = pos_first + value.size();
	}

	return str;
}
