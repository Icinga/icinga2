#ifndef MACROPROCESSOR_H
#define MACROPROCESSOR_H

namespace icinga
{

class I2_ICINGA_API MacroProcessor
{
public:
	static string ResolveMacros(string str, Dictionary::Ptr macros);
};

}

#endif /* MACROPROCESSOR_H */