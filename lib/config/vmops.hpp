/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef VMOPS_H
#define VMOPS_H

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "config/configitembuilder.hpp"
#include "config/applyrule.hpp"
#include "config/objectrule.hpp"
#include "base/debuginfo.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/scriptglobal.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include <map>
#include <vector>

namespace icinga
{

class VMOps
{
public:
	static inline bool FindVarImportRef(ScriptFrame& frame, const String& name, Value *result, const DebugInfo& debugInfo = DebugInfo())
	{
		Array::Ptr imports = ScriptFrame::GetImports();

		ObjectLock olock(imports);
		for (const Value& import : imports) {
			Object::Ptr obj = import;
			if (obj->HasOwnField(name)) {
				*result = import;
				return true;
			}
		}

		return false;
	}

	static inline bool FindVarImport(ScriptFrame& frame, const String& name, Value *result, const DebugInfo& debugInfo = DebugInfo())
	{
		Value parent;

		if (FindVarImportRef(frame, name, &parent, debugInfo)) {
			*result = GetField(parent, name, frame.Sandboxed, debugInfo);
			return true;
		}

		return false;
	}

	static inline Value ConstructorCall(const Type::Ptr& type, const std::vector<Value>& args, const DebugInfo& debugInfo = DebugInfo())
	{
		if (type->GetName() == "String") {
			if (args.empty())
				return "";
			else if (args.size() == 1)
				return Convert::ToString(args[0]);
			else
				BOOST_THROW_EXCEPTION(ScriptError("Too many arguments for constructor."));
		} else if (type->GetName() == "Number") {
			if (args.empty())
				return 0;
			else if (args.size() == 1)
				return Convert::ToDouble(args[0]);
			else
				BOOST_THROW_EXCEPTION(ScriptError("Too many arguments for constructor."));
		} else if (type->GetName() == "Boolean") {
			if (args.empty())
				return 0;
			else if (args.size() == 1)
				return Convert::ToBool(args[0]);
			else
				BOOST_THROW_EXCEPTION(ScriptError("Too many arguments for constructor."));
		} else if (args.size() == 1 && type->IsAssignableFrom(args[0].GetReflectionType()))
			return args[0];
		else
			return type->Instantiate(args);
	}

	static inline Value FunctionCall(ScriptFrame& frame, const Value& self, const Function::Ptr& func, const std::vector<Value>& arguments)
	{
		if (!self.IsEmpty() || self.IsString())
			return func->InvokeThis(self, arguments);
		else
			return func->Invoke(arguments);

	}

	static inline Value NewFunction(ScriptFrame& frame, const String& name, const std::vector<String>& argNames,
		const std::map<String, std::unique_ptr<Expression> >& closedVars, const std::shared_ptr<Expression>& expression)
	{
		auto evaluatedClosedVars = EvaluateClosedVars(frame, closedVars);

		auto wrapper = [argNames, evaluatedClosedVars, expression](const std::vector<Value>& arguments) -> Value {
			if (arguments.size() < argNames.size())
				BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function"));

			ScriptFrame *frame = ScriptFrame::GetCurrentFrame();

			frame->Locals = new Dictionary();

			if (evaluatedClosedVars)
				evaluatedClosedVars->CopyTo(frame->Locals);

			for (std::vector<Value>::size_type i = 0; i < std::min(arguments.size(), argNames.size()); i++)
				frame->Locals->Set(argNames[i], arguments[i]);

			return expression->Evaluate(*frame);
		};

		return new Function(name, wrapper, argNames);
	}

	static inline Value NewApply(ScriptFrame& frame, const String& type, const String& target, const String& name, const std::shared_ptr<Expression>& filter,
		const String& package, const String& fkvar, const String& fvvar, const std::shared_ptr<Expression>& fterm, const std::map<String, std::unique_ptr<Expression> >& closedVars,
		bool ignoreOnError, const std::shared_ptr<Expression>& expression, const DebugInfo& debugInfo = DebugInfo())
	{
		ApplyRule::AddRule(type, target, name, expression, filter, package, fkvar,
			fvvar, fterm, ignoreOnError, debugInfo, EvaluateClosedVars(frame, closedVars));

		return Empty;
	}

	static inline Value NewObject(ScriptFrame& frame, bool abstract, const Type::Ptr& type, const String& name, const std::shared_ptr<Expression>& filter,
		const String& zone, const String& package, bool defaultTmpl, bool ignoreOnError, const std::map<String, std::unique_ptr<Expression> >& closedVars, const std::shared_ptr<Expression>& expression, const DebugInfo& debugInfo = DebugInfo())
	{
		ConfigItemBuilder::Ptr item = new ConfigItemBuilder(debugInfo);

		String checkName = name;

		if (!abstract) {
			auto *nc = dynamic_cast<NameComposer *>(type.get());

			if (nc)
				checkName = nc->MakeName(name, nullptr);
		}

		if (!checkName.IsEmpty()) {
			ConfigItem::Ptr oldItem = ConfigItem::GetByTypeAndName(type, checkName);

			if (oldItem) {
				std::ostringstream msgbuf;
				msgbuf << "Object '" << name << "' of type '" << type->GetName() << "' re-defined: " << debugInfo << "; previous definition: " << oldItem->GetDebugInfo();
				BOOST_THROW_EXCEPTION(ScriptError(msgbuf.str(), debugInfo));
			}
		}

		if (filter && !ObjectRule::IsValidSourceType(type->GetName())) {
			std::ostringstream msgbuf;
			msgbuf << "Object '" << name << "' of type '" << type->GetName() << "' must not have 'assign where' and 'ignore where' rules: " << debugInfo;
			BOOST_THROW_EXCEPTION(ScriptError(msgbuf.str(), debugInfo));
		}

		item->SetType(type);
		item->SetName(name);

		if (!abstract)
			item->AddExpression(new ImportDefaultTemplatesExpression());

		item->AddExpression(new OwnedExpression(expression));
		item->SetAbstract(abstract);
		item->SetScope(EvaluateClosedVars(frame, closedVars));
		item->SetZone(zone);
		item->SetPackage(package);
		item->SetFilter(filter);
		item->SetDefaultTemplate(defaultTmpl);
		item->SetIgnoreOnError(ignoreOnError);
		item->Compile()->Register();

		return Empty;
	}

	static inline ExpressionResult For(ScriptFrame& frame, const String& fkvar, const String& fvvar, const Value& value, const std::unique_ptr<Expression>& expression, const DebugInfo& debugInfo = DebugInfo())
	{
		if (value.IsObjectType<Array>()) {
			if (!fvvar.IsEmpty())
				BOOST_THROW_EXCEPTION(ScriptError("Cannot use dictionary iterator for array.", debugInfo));

			Array::Ptr arr = value;

			for (Array::SizeType i = 0; i < arr->GetLength(); i++) {
				frame.Locals->Set(fkvar, arr->Get(i));
				ExpressionResult res = expression->Evaluate(frame);
				CHECK_RESULT_LOOP(res);
			}
		} else if (value.IsObjectType<Dictionary>()) {
			if (fvvar.IsEmpty())
				BOOST_THROW_EXCEPTION(ScriptError("Cannot use array iterator for dictionary.", debugInfo));

			Dictionary::Ptr dict = value;
			std::vector<String> keys;

			{
				ObjectLock olock(dict);
				for (const Dictionary::Pair& kv : dict) {
					keys.push_back(kv.first);
				}
			}

			for (const String& key : keys) {
				frame.Locals->Set(fkvar, key);
				frame.Locals->Set(fvvar, dict->Get(key));
				ExpressionResult res = expression->Evaluate(frame);
				CHECK_RESULT_LOOP(res);
			}
		} else
			BOOST_THROW_EXCEPTION(ScriptError("Invalid type in for expression: " + value.GetTypeName(), debugInfo));

		return Empty;
	}

	static inline Value GetField(const Value& context, const String& field, bool sandboxed = false, const DebugInfo& debugInfo = DebugInfo())
	{
		if (unlikely(context.IsEmpty() && !context.IsString()))
			return Empty;

		if (unlikely(!context.IsObject()))
			return GetPrototypeField(context, field, true, debugInfo);

		Object::Ptr object = context;

		return object->GetFieldByName(field, sandboxed, debugInfo);
	}

	static inline void SetField(const Object::Ptr& context, const String& field, const Value& value, const DebugInfo& debugInfo = DebugInfo())
	{
		if (!context)
			BOOST_THROW_EXCEPTION(ScriptError("Cannot set field '" + field + "' on a value that is not an object.", debugInfo));

		return context->SetFieldByName(field, value, debugInfo);
	}

private:
	static inline Dictionary::Ptr EvaluateClosedVars(ScriptFrame& frame, const std::map<String, std::unique_ptr<Expression> >& closedVars)
	{
		Dictionary::Ptr locals;

		if (!closedVars.empty()) {
			locals = new Dictionary();

			for (const auto& cvar : closedVars) {
				locals->Set(cvar.first, cvar.second->Evaluate(frame));
			}
		}

		return locals;
	}
};

}

#endif /* VMOPS_H */
