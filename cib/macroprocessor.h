#ifndef MACROPROCESSOR_H
#define MACROPROCESSOR_H

namespace icinga
{

class I2_CIB_API MacroProcessor
{
public:
	static string ResolveMacros(const string& str, const Dictionary::Ptr& macros);
};

}

#endif /* MACROPROCESSOR_H */