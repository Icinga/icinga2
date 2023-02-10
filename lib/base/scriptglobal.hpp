/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SCRIPTGLOBAL_H
#define SCRIPTGLOBAL_H

#include "base/i2-base.hpp"
#include "base/namespace.hpp"

namespace icinga
{

/**
 * Global script variables.
 *
 * @ingroup base
 */
class ScriptGlobal
{
public:
	static Value Get(const String& name, const Value *defaultValue = nullptr);
	static void Set(const String& name, const Value& value);
	static void SetConst(const String& name, const Value& value);
	static bool Exists(const String& name);

	static void WriteToFile(const String& filename);

	static Namespace::Ptr GetGlobals();

private:
	static Namespace::Ptr m_Globals;
};

}

#endif /* SCRIPTGLOBAL_H */
