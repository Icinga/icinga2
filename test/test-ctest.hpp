/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include <BoostTestTargetConfig.h>
#include <boost/filesystem/path.hpp>
#include <boost/test/tree/traverse.hpp>
#include <fstream>

namespace icinga {

/**
 * A boost test decorator to set additional ctest properties for the test.
 */
class CTestProperties : public boost::unit_test::decorator::base
{
public:
	explicit CTestProperties(std::string props) : m_Props(std::move(props)) {}

	std::string m_Props;

private:
	/**
	 * Doesn't do anything to the case/suite it's applied to.
	 *
	 * However this gets added to the list so we can later find it by iterating the
	 * decorators and dynamic_casting them to this type and get the m_props value.
	 */
	void apply(boost::unit_test::test_unit&) override {}
	[[nodiscard]] boost::unit_test::decorator::base_ptr clone() const override;
};

class CTestFileGenerator : public boost::unit_test::test_tree_visitor
{
public:
	CTestFileGenerator(const boost::filesystem::path& testexe, const boost::filesystem::path& outfile);

	void visit(const boost::unit_test::test_case& test) override;

	bool test_suite_start(const boost::unit_test::test_suite& suite) override;
	void test_suite_finish(const boost::unit_test::test_suite& suite) override;

private:
	static std::string LabelsToProp(const std::vector<std::string>& labels);

	std::ofstream m_Fp;
	boost::filesystem::path m_WorkDir;
	boost::filesystem::path m_TestExe;
	std::vector<std::vector<std::string>> m_PropsStack;
};

struct CTestFileGeneratorFixture
{
	CTestFileGeneratorFixture();
};

} // namespace icinga
