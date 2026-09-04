/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/array.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"
#include <boost/config.hpp>
#include <limits>
#include <type_traits>

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

/**
 * Sorts input using heapsort, safe against a misbehaving comparator.
 *
 * A script-provided comparator is arbitrary script code and isn't guaranteed to implement
 * a strict weak ordering (e.g. a comparator that always returns true, or one that's
 * non-deterministic like `random() % 2`). Every std::-family algorithm (sort,
 * stable_sort, partial_sort, nth_element) relies on that guarantee for memory safety and
 * reads/writes out of bounds if it's violated.
 *
 * This is the textbook heapsort from https://en.wikipedia.org/w/index.php?title=Heapsort&oldid=1367370775
 * (functions and variables named to match that article's heapify/siftDown/heapSort pseudocode exactly),
 * which never makes that assumption to avoid out-of-bounds access.
 *
 * @param input The array to sort.
 * @param less Comparator, returns true if a < b.
 * @returns The sorted array.
 */
template<class Compare>
static Array::Ptr SafeSort(const Array::Ptr& input, Compare less)
{
	using Index = decltype(input->End() - input->Begin());
	static_assert(std::is_signed_v<Index>);

	constexpr Array::SizeType max = std::numeric_limits<Index>::max();

	// Dividing by 4, so that the sort algorithm may still multiply every index with 2
	constexpr auto limit = max / 4;

	// Keep sorting duration below 1 second
	constexpr decltype(limit) sandboxLimit = 1L << 17;
	static_assert(sandboxLimit <= limit);

	if (input->GetLength() > (ScriptFrame::GetCurrentFrame()->Sandboxed ? sandboxLimit : limit)) {
		BOOST_THROW_EXCEPTION(ScriptError("Array is too large."));
	}

	auto output = input->ShallowClone();
	ObjectLock aLock (output);
	auto begin = output->Begin();
	auto end = output->End();

	auto iParent = [](Index i) -> Index {
		if (BOOST_UNLIKELY(i <= 0)) {
			return -1; // no parent
		}

		return (i - 1) / 2;
	};

	auto iLeftChild = [](Index i) -> Index {
		return 2 * i + 1;
	};

	auto iRightChild = [](Index i) -> Index {
		return 2 * i + 2;
	};

	auto siftDown = [&](Index start, Index last) {
		Index root = start;

		// While the root has at least one child
		while (iLeftChild(root) <= last) {
			Index child = iLeftChild(root);
			Index swap = root; // Child to swap with

			if (less(begin[swap], begin[child])) {
				swap = child;
			}

			child = iRightChild(root);

			// If there is a right child and that child is greater
			if (child <= last && less(begin[swap], begin[child])) {
				swap = child;
			}

			if (swap == root) {
				// The root holds the largest element, children are already valid
				return;
			}

			std::iter_swap(begin + root, begin + swap);

			// Repeat to continue sifting down the child now
			root = swap;
		}
	};

	Index count = end - begin;

	// heapify(a, count): build the max-heap, from the last parent up to the root
	for (Index start = iParent(count - 1); start >= 0; --start) {
		siftDown(start, count - 1);
	}

	// heapSort(a, count): repeatedly move the root (the max) to the end of the
	// shrinking unsorted prefix a[0:last], then restore the heap property there
	for (Index last = count - 1; last > 0; --last) {
		std::iter_swap(begin, begin + last);
		siftDown(0, last - 1);
	}

	return output;
}

static Array::Ptr ArraySort(const std::vector<Value>& args)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	if (args.empty()) {
		return SafeSort(self, std::less<Value>{});
	} else {
		Function::Ptr function = args[0];
		REQUIRE_NOT_NULL(function);

		if (vframe->Sandboxed && !function->IsSideEffectFree())
			BOOST_THROW_EXCEPTION(ScriptError("Sort function must be side-effect free."));

		return SafeSort(self, [&args](const Value& a, const Value& b) -> bool {
			Function::Ptr cmp = args[0];
			return cmp->Invoke({ a, b });
		});
	}
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
	return self->Join(separator);
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
	REQUIRE_NOT_NULL(function);

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
	REQUIRE_NOT_NULL(function);

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
	REQUIRE_NOT_NULL(function);

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
	REQUIRE_NOT_NULL(function);

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
	REQUIRE_NOT_NULL(function);

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
