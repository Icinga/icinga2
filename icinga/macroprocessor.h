#ifndef MACROPROCESSOR_H
#define MACROPROCESSOR_H

namespace icinga
{

class MacroProcessor
{
public:
	static string ResolveMacros(string str, Dictionary::Ptr macros);
};

}

#endif /* MACROPROCESSOR_H */