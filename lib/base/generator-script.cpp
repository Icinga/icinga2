/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/array.hpp"
#include "base/exception.hpp"
#include "base/function.hpp"
#include "base/generator.hpp"
#include "base/generator-filter.hpp"
#include "base/generator-map.hpp"
#include "base/objectlock.hpp"
#include "base/reference.hpp"
#include "base/scriptframe.hpp"
#include "base/value.hpp"
#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

using namespace icinga;

static bool GeneratorNext(const Reference::Ptr& out)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	Value buf;

	if (self->GetNext(buf)) {
		if (out) {
			out->Set(std::move(buf));
		}

		return true;
	}

	return false;
}

static double GeneratorLen()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	return self->GetLength();
}

static bool GeneratorContains(const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	return self->Contains(value);
}

static Array::Ptr GeneratorSort(const std::vector<Value>& args)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	auto arr (self->ToArray());

	if (args.empty()) {
		ObjectLock olock(arr);
		std::sort(arr->Begin(), arr->End());
	} else {
		Function::Ptr function = args[0];

		if (vframe->Sandboxed && !function->IsSideEffectFree())
			BOOST_THROW_EXCEPTION(ScriptError("Sort function must be side-effect free."));

		ObjectLock olock(arr);
		std::sort(arr->Begin(), arr->End(), [&args](const Value& a, const Value& b) -> bool {
			Function::Ptr cmp = args[0];
			return cmp->Invoke({ a, b });
		});
	}

	return arr;
}

static Value GeneratorJoin(const Value& separator)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	return self->Join(separator);
}

static Array::Ptr GeneratorReverse()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	return self->Reverse();
}

static Generator::Ptr GeneratorMap(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Map function must be side-effect free."));

	return new icinga::GeneratorMap(std::move(self), function);
}

static Value GeneratorReduce(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Reduce function must be side-effect free."));

	Value result;

	if (!self->GetNext(result)) {
		return Empty;
	}

	Value buf;

	while (self->GetNext(buf)) {
		result = function->Invoke({ std::move(result), std::move(buf) });
	}

	return result;
}

static Generator::Ptr GeneratorFilter(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Filter function must be side-effect free."));

	return new icinga::GeneratorFilter(std::move(self), function);
}

static bool GeneratorAny(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Filter function must be side-effect free."));

	Value buf;

	while (self->GetNext(buf)) {
		if (function->Invoke({ std::move(buf) })) {
			return true;
		}
	}

	return false;
}

static bool GeneratorAll(const Function::Ptr& function)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	if (vframe->Sandboxed && !function->IsSideEffectFree())
		BOOST_THROW_EXCEPTION(ScriptError("Filter function must be side-effect free."));

	Value buf;

	while (self->GetNext(buf)) {
		if (!function->Invoke({ std::move(buf) })) {
			return false;
		}
	}

	return true;
}

static Generator::Ptr GeneratorUnique()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	return self->Unique();
}

static Array::Ptr GeneratorToArray()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Generator::Ptr self = static_cast<Generator::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	return self->ToArray();
}

Object::Ptr Generator::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "next", new Function("Generator#next", GeneratorNext, { "out" }) },
		{ "len", new Function("Generator#len", GeneratorLen, {}, true) },
		{ "contains", new Function("Generator#contains", GeneratorContains, { "value" }, true) },
		{ "sort", new Function("Generator#sort", GeneratorSort, { "less_cmp" }, true) },
		{ "join", new Function("Generator#join", GeneratorJoin, { "separator" }, true) },
		{ "reverse", new Function("Generator#reverse", GeneratorReverse, {}, true) },
		{ "map", new Function("Generator#map", ::GeneratorMap, { "func" }, true) },
		{ "reduce", new Function("Generator#reduce", GeneratorReduce, { "reduce" }, true) },
		{ "filter", new Function("Generator#filter", ::GeneratorFilter, { "func" }, true) },
		{ "any", new Function("Generator#any", GeneratorAny, { "func" }, true) },
		{ "all", new Function("Generator#all", GeneratorAll, { "func" }, true) },
		{ "unique", new Function("Generator#unique", ::GeneratorUnique, {}, true) },
		{ "to_array", new Function("Generator#to_array", GeneratorToArray, {}, true) }
	});

	return prototype;
}
