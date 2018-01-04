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

#include "base/array.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"

using namespace icinga;

static double ArrayLen()
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

static void ArrayClear()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->Clear();
}

static bool ArraySortCmp(const Function::Ptr& cmp, const Value& a, const Value& b)
{
	return cmp->Invoke({ a, b });
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

static Array::Ptr ArrayShallowClone()
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

static Array::Ptr ArrayReverse()
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
		result->Add(function->Invoke({ item }));
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
		result = function->Invoke({ result, self->Get(i) });
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
		if (function->Invoke({ item }))
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
		if (function->Invoke({ item }))
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
		if (!function->Invoke({ item }))
			return false;
	}

	return true;
}
static Array::Ptr ArrayUnique()
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

Object::Ptr Array::GetPrototype()
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("len", new Function("Array#len", ArrayLen, {}, true));
		prototype->Set("set", new Function("Array#set", ArraySet, { "index", "value" }));
		prototype->Set("get", new Function("Array#get", ArrayGet, { "index" }));
		prototype->Set("add", new Function("Array#add", ArrayAdd, { "value" }));
		prototype->Set("remove", new Function("Array#remove", ArrayRemove, { "index" }));
		prototype->Set("contains", new Function("Array#contains", ArrayContains, { "value" }, true));
		prototype->Set("clear", new Function("Array#clear", ArrayClear));
		prototype->Set("sort", new Function("Array#sort", ArraySort, { "less_cmp" }, true));
		prototype->Set("shallow_clone", new Function("Array#shallow_clone", ArrayShallowClone, {}, true));
		prototype->Set("join", new Function("Array#join", ArrayJoin, { "separator" }, true));
		prototype->Set("reverse", new Function("Array#reverse", ArrayReverse, {}, true));
		prototype->Set("map", new Function("Array#map", ArrayMap, { "func" }, true));
		prototype->Set("reduce", new Function("Array#reduce", ArrayReduce, { "reduce" }, true));
		prototype->Set("filter", new Function("Array#filter", ArrayFilter, { "func" }, true));
		prototype->Set("any", new Function("Array#any", ArrayAny, { "func" }, true));
		prototype->Set("all", new Function("Array#all", ArrayAll, { "func" }, true));
		prototype->Set("unique", new Function("Array#unique", ArrayUnique, {}, true));
	}

	return prototype;
}
