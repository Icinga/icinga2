/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/array.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"

using namespace icinga;

static double ArrayLen(void)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	return self->GetLength();
}

static void ArraySet(int index, const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->Set(index, value);
}

static Value ArrayGet(int index)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	return self->Get(index);
}

static void ArrayAdd(const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->Add(value);
}

static void ArrayRemove(int index)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->Remove(index);
}

static bool ArrayContains(const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	return self->Contains(value);
}

static void ArrayClear(void)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->Clear();
}

static bool ArraySortCmp(const Function::Ptr& cmp, const Value& a, const Value& b)
{
	std::vector<Value> args;
	args.push_back(a);
	args.push_back(b);
	return cmp->Invoke(args);
}

static Array::Ptr ArraySort(const std::vector<Value>& args)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);

	Array::Ptr arr = self->ShallowClone();

	if (args.empty()) {
		ObjectLock olock(arr);
		std::sort(arr->Begin(), arr->End());
	} else {
		Function::Ptr function = args[0];

		if (vframe->Sandboxed && !function->IsSideEffectFree())
			BOOST_THROW_EXCEPTION(ScriptError("Sort function must be side-effect free."));

		ObjectLock olock(arr);
		std::sort(arr->Begin(), arr->End(), std::bind(ArraySortCmp, args[0], _1, _2));
	}

	return arr;
}

static Array::Ptr ArrayShallowClone(void)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	return self->ShallowClone();
}

static Value ArrayJoin(const Value& separator)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);

	Value result;
	bool first = true;

	ObjectLock olock(self);
	for (const Value& item : self) {
		if (first) {
			first = false;
		} else {
			result = result + separator;
		}

		result = result + item;
	}

	return result;
}

static Array::Ptr ArrayReverse(void)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	return self->Reverse();
}

static Array::Ptr ArrayMap(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Map function must be side-effect free."));

	Array::Ptr result = new Array();

	ObjectLock olock(self);
	for (const Value& item : self) {
		std::vector<Value> args;
		args.push_back(item);
		result->Add(function->Invoke(args));
	}

	return result;
}

static Value ArrayReduce(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Reduce function must be side-effect free."));

	if (self->GetLength() == 0)
		return Empty;

	Value result = self->Get(0);

	ObjectLock olock(self);
	for (size_t i = 1; i < self->GetLength(); i++) {
		std::vector<Value> args;
		args.push_back(result);
		args.push_back(self->Get(i));
		result = function->Invoke(args);
	}

	return result;
}

static Array::Ptr ArrayFilter(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Filter function must be side-effect free."));

	Array::Ptr result = new Array();

	ObjectLock olock(self);
	for (const Value& item : self) {
		std::vector<Value> args;
		args.push_back(item);
		if (function->Invoke(args))
			result->Add(item);
	}

	return result;
}

static bool ArrayAny(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Filter function must be side-effect free."));

	ObjectLock olock(self);
	for (const Value& item : self) {
		std::vector<Value> args;
		args.push_back(item);
		if (function->Invoke(args))
			return true;
	}

	return false;
}

static bool ArrayAll(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Filter function must be side-effect free."));

	ObjectLock olock(self);
	for (const Value& item : self) {
		std::vector<Value> args;
		args.push_back(item);
		if (!function->Invoke(args))
			return false;
	}

	return true;
}
static Array::Ptr ArrayUnique(void)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);

	std::set<Value> result;

	ObjectLock olock(self);
	for (const Value& item : self) {
		result.insert(item);
	}

	return Array::FromSet(result);
}

Object::Ptr Array::GetPrototype(void)
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("len", new Function("Array#len", WrapFunction(ArrayLen), {}, true));
		prototype->Set("set", new Function("Array#set", WrapFunction(ArraySet), { "index", "value" }));
		prototype->Set("get", new Function("Array#get", WrapFunction(ArrayGet), { "index" }));
		prototype->Set("add", new Function("Array#add", WrapFunction(ArrayAdd), { "value" }));
		prototype->Set("remove", new Function("Array#remove", WrapFunction(ArrayRemove), { "index" }));
		prototype->Set("contains", new Function("Array#contains", WrapFunction(ArrayContains), { "value" }, true));
		prototype->Set("clear", new Function("Array#clear", WrapFunction(ArrayClear)));
		prototype->Set("sort", new Function("Array#sort", WrapFunction(ArraySort), { "less_cmp" }, true));
		prototype->Set("shallow_clone", new Function("Array#shallow_clone", WrapFunction(ArrayShallowClone), {}, true));
		prototype->Set("join", new Function("Array#join", WrapFunction(ArrayJoin), { "separator" }, true));
		prototype->Set("reverse", new Function("Array#reverse", WrapFunction(ArrayReverse), {}, true));
		prototype->Set("map", new Function("Array#map", WrapFunction(ArrayMap), { "func" }, true));
		prototype->Set("reduce", new Function("Array#reduce", WrapFunction(ArrayReduce), { "reduce" }, true));
		prototype->Set("filter", new Function("Array#filter", WrapFunction(ArrayFilter), { "func" }, true));
		prototype->Set("any", new Function("Array#any", WrapFunction(ArrayAny), { "func" }, true));
		prototype->Set("all", new Function("Array#all", WrapFunction(ArrayAll), { "func" }, true));
		prototype->Set("unique", new Function("Array#unique", WrapFunction(ArrayUnique), {}, true));
	}

	return prototype;
}
