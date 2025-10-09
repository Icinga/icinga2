/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef MACROPROCESSOR_H
#define MACROPROCESSOR_H

#include "icinga/i2-icinga.hpp"
#include "icinga/checkable.hpp"
#include "base/value.hpp"
#include <vector>
#include <utility>

namespace icinga
{

/**
 * Resolves macros.
 *
 * @ingroup icinga
 */
class MacroProcessor
{
public:
	struct ResolverSpec
	{
		String Name;
		Object::Ptr Obj;

		// Whether to resolve not only e.g. $host.address$, but also just $address$
		bool ResolveShortMacros;

		inline ResolverSpec(String name, Object::Ptr obj, bool resolveShortMacros = true)
			: Name(std::move(name)), Obj(std::move(obj)), ResolveShortMacros(resolveShortMacros)
		{
		}
	};

	typedef std::function<Value (const Value&)> EscapeCallback;
	typedef std::vector<ResolverSpec> ResolverList;

	static Value ResolveMacros(const Value& str, const ResolverList& resolvers,
		const CheckResult::Ptr& cr = nullptr, String *missingMacro = nullptr,
		const EscapeCallback& escapeFn = EscapeCallback(),
		const Dictionary::Ptr& resolvedMacros = nullptr,
		bool useResolvedMacros = false, int recursionLevel = 0);

	static Value ResolveArguments(const Value& command, const Dictionary::Ptr& arguments,
		const MacroProcessor::ResolverList& resolvers, const CheckResult::Ptr& cr,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros, int recursionLevel = 0);

	static bool ValidateMacroString(const String& macro);
	static void ValidateCustomVars(const ConfigObject::Ptr& object, const Dictionary::Ptr& value);

private:
	MacroProcessor();

	static bool ResolveMacro(const String& macro, const ResolverList& resolvers,
		const CheckResult::Ptr& cr, Value *result, bool *recursive_macro);
	static Value InternalResolveMacros(const String& str,
		const ResolverList& resolvers, const CheckResult::Ptr& cr,
		String *missingMacro, const EscapeCallback& escapeFn,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros,
		int recursionLevel = 0);
	static Value EvaluateFunction(const Function::Ptr& func, const ResolverList& resolvers,
		const CheckResult::Ptr& cr,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros, int recursionLevel);

	static void AddArgumentHelper(const Array::Ptr& args, const String& key, const String& value,
		bool add_key, bool add_value, const Value& separator);
	static Value EscapeMacroShellArg(const Value& value);

};

}

#endif /* MACROPROCESSOR_H */
