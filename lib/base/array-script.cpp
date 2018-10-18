/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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
	REQUIRE_NOT_NULL(self);
	return self->GetLength();
}

static void ArraySet(int index, const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	self->Set(index, value);
}

static Value ArrayGet(int index)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	return self->Get(index);
}

static void ArrayAdd(const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	self->Add(value);
}

static void ArrayRemove(int index)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	self->Remove(index);
}

static bool ArrayContains(const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	return self->Contains(value);
}

static void ArrayClear()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
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
	REQUIRE_NOT_NULL(self);

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
	REQUIRE_NOT_NULL(self);
	return self->ShallowClone();
}

static Value ArrayJoin(const Value& separator)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

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
	REQUIRE_NOT_NULL(self);
	return self->Reverse();
}

static Array::Ptr ArrayMap(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Map function must be side-effect free."));

	ArrayData result;

	ObjectLock olock(self);
	for (const Value& item : self) {
		result.push_back(function->Invoke({ item }));
	}

	return new Array(std::move(result));
}

static Value ArrayReduce(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

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
	REQUIRE_NOT_NULL(self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Filter function must be side-effect free."));

	ArrayData result;

	ObjectLock olock(self);
	for (const Value& item : self) {
		if (function->Invoke({ item }))
			result.push_back(item);
	}

	return new Array(std::move(result));
}

static bool ArrayAny(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

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
	REQUIRE_NOT_NULL(self);

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
	REQUIRE_NOT_NULL(self);
	return self->Unique();
}

static void ArrayFreeze()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->Freeze();
}

Object::Ptr Array::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "len", new Function("Array#len", ArrayLen, {}, true) },
		{ "set", new Function("Array#set", ArraySet, { "index", "value" }) },
		{ "get", new Function("Array#get", ArrayGet, { "index" }) },
		{ "add", new Function("Array#add", ArrayAdd, { "value" }) },
		{ "remove", new Function("Array#remove", ArrayRemove, { "index" }) },
		{ "contains", new Function("Array#contains", ArrayContains, { "value" }, true) },
		{ "clear", new Function("Array#clear", ArrayClear) },
		{ "sort", new Function("Array#sort", ArraySort, { "less_cmp" }, true) },
		{ "shallow_clone", new Function("Array#shallow_clone", ArrayShallowClone, {}, true) },
		{ "join", new Function("Array#join", ArrayJoin, { "separator" }, true) },
		{ "reverse", new Function("Array#reverse", ArrayReverse, {}, true) },
		{ "map", new Function("Array#map", ArrayMap, { "func" }, true) },
		{ "reduce", new Function("Array#reduce", ArrayReduce, { "reduce" }, true) },
		{ "filter", new Function("Array#filter", ArrayFilter, { "func" }, true) },
		{ "any", new Function("Array#any", ArrayAny, { "func" }, true) },
		{ "all", new Function("Array#all", ArrayAll, { "func" }, true) },
		{ "unique", new Function("Array#unique", ArrayUnique, {}, true) },
		{ "freeze", new Function("Array#freeze", ArrayFreeze, {}) }
	});

	return prototype;
}
