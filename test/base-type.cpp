/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/perfdatavalue.hpp"
#include "base/dictionary.hpp"
#include "base/objectlock.hpp"
#include "base/application.hpp"
#include "base/type.hpp"
#include <BoostTestTargetConfig.h>
#include <algorithm>
#include <stdexcept>

using namespace icinga;

class TestType : public Type
{
public:
	TestType(std::unordered_set<Type*> loadDependencies = {}) : m_LoadDependencies(std::move(loadDependencies))
	{
	}

	const std::unordered_set<Type*>& GetLoadDependencies() const override
	{
		return m_LoadDependencies;
	}

	String GetName() const override
	{
		throw std::runtime_error("not implemented");
	}

	Type::Ptr GetBaseType() const override
	{
		throw std::runtime_error("not implemented");
	}

	int GetAttributes() const override
	{
		throw std::runtime_error("not implemented");
	}

	int GetFieldId(const String&) const override
	{
		throw std::runtime_error("not implemented");
	}

	Field GetFieldInfo(int) const override
	{
		throw std::runtime_error("not implemented");
	}

	int GetFieldCount() const override
	{
		throw std::runtime_error("not implemented");
	}

protected:
	ObjectFactory GetFactory() const override
	{
		throw std::runtime_error("not implemented");
	}

private:
	std::unordered_set<Type*> m_LoadDependencies;
};

static void TestSortByLoadDependenciesUnrelated(int amount, bool withDepsToForeign = false)
{
	std::vector<Type::Ptr> expected;

	for (int i = 0; i < amount; ++i) {
		expected.emplace_back(withDepsToForeign ? new TestType({new TestType()}) : new TestType());
	}

	std::sort(expected.begin(), expected.end());

	auto input (expected);

	do {
		auto actual (Type::SortByLoadDependencies(input));

		std::sort(actual.begin(), actual.end());
		BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
	} while (std::next_permutation(input.begin(), input.end()));
}

static void TestSortByLoadDependenciesRelated(int amount, bool withDepsToForeign = false)
{
	std::vector<Type::Ptr> expected;

	for (int i = 0; i < amount; ++i) {
		if (i) {
			expected.emplace_back(
				withDepsToForeign
					? new TestType({expected.at(expected.size() - 1u).get(), new TestType()})
					: new TestType({expected.at(expected.size() - 1u).get()})
			);
		} else {
			expected.emplace_back(withDepsToForeign ? new TestType({new TestType()}) : new TestType());
		}
	}

	auto input (expected);

	std::sort(input.begin(), input.end());

	do {
		auto actual (Type::SortByLoadDependencies(input));

		BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
	} while (std::next_permutation(input.begin(), input.end()));
}

BOOST_AUTO_TEST_SUITE(base_type)

BOOST_AUTO_TEST_CASE(gettype)
{
	Type::Ptr t = Type::GetByName("Application");

	BOOST_CHECK(t);
}

BOOST_AUTO_TEST_CASE(assign)
{
	Type::Ptr t1 = Type::GetByName("Application");
	Type::Ptr t2 = Type::GetByName("ConfigObject");

	BOOST_CHECK(t1->IsAssignableFrom(t1));
	BOOST_CHECK(t2->IsAssignableFrom(t1));
	BOOST_CHECK(!t1->IsAssignableFrom(t2));
}

BOOST_AUTO_TEST_CASE(byname)
{
	Type::Ptr t = Type::GetByName("Application");

	BOOST_CHECK(t);
}

BOOST_AUTO_TEST_CASE(instantiate)
{
	Type::Ptr t = Type::GetByName("PerfdataValue");

	Object::Ptr p = t->Instantiate(std::vector<Value>());

	BOOST_CHECK(p);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies0)
{
	std::vector<Type::Ptr> empty;
	auto actual (Type::SortByLoadDependencies(empty));

	BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), empty.begin(), empty.end());
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies1)
{
	std::vector<Type::Ptr> one ({new TestType()});
	auto actual (Type::SortByLoadDependencies(one));

	BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), one.begin(), one.end());
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies2unrelated)
{
	TestSortByLoadDependenciesUnrelated(2);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies2related)
{
	TestSortByLoadDependenciesRelated(2);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies3unrelated)
{
	TestSortByLoadDependenciesUnrelated(3);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies3related)
{
	TestSortByLoadDependenciesRelated(3);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies4unrelated)
{
	TestSortByLoadDependenciesUnrelated(4);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies4related)
{
	TestSortByLoadDependenciesRelated(4);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies5unrelated)
{
	TestSortByLoadDependenciesUnrelated(5);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies5related)
{
	TestSortByLoadDependenciesRelated(5);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies1_withforeign)
{
	std::vector<Type::Ptr> one ({new TestType({new TestType()})});
	auto actual (Type::SortByLoadDependencies(one));

	BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), one.begin(), one.end());
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies2unrelated_withforeign)
{
	TestSortByLoadDependenciesUnrelated(2, true);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies2related_withforeign)
{
	TestSortByLoadDependenciesRelated(2, true);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies3unrelated_withforeign)
{
	TestSortByLoadDependenciesUnrelated(3, true);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies3related_withforeign)
{
	TestSortByLoadDependenciesRelated(3, true);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies4unrelated_withforeign)
{
	TestSortByLoadDependenciesUnrelated(4, true);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies4related_withforeign)
{
	TestSortByLoadDependenciesRelated(4, true);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies5unrelated_withforeign)
{
	TestSortByLoadDependenciesUnrelated(5, true);
}

BOOST_AUTO_TEST_CASE(sortbyloaddependencies5related_withforeign)
{
	TestSortByLoadDependenciesRelated(5, true);
}

BOOST_AUTO_TEST_SUITE_END()
