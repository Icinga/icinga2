#include "test-ctest.hpp"
#include <boost/regex.hpp>
#include <boost/make_shared.hpp>
#include <iostream>

using namespace icinga;

BOOST_TEST_GLOBAL_FIXTURE(CTestFileGeneratorFixture);

boost::unit_test::decorator::base_ptr CTestProperties::clone() const
{
	return boost::make_shared<CTestProperties>(m_Props);
}

CTestFileGenerator::CTestFileGenerator(const boost::filesystem::path& testexe, const boost::filesystem::path& outfile)
	: m_Fp(outfile.string(), std::iostream::trunc), m_WorkDir(outfile.parent_path().string()),
	  m_TestExe(testexe.string())
{
	m_Fp.exceptions(std::iostream::badbit | std::iostream::failbit);
}

void CTestFileGenerator::visit(const boost::unit_test::test_case& test)
{
	m_Fp << "add_test(" << test.full_name() << " " << m_TestExe << " --run_test=" << test.full_name() << ")\n";
	m_Fp << "set_tests_properties(" << test.full_name() << "\n  PROPERTIES"
		 << "  WORKING_DIRECTORY " << m_WorkDir << "\n";

	auto labels = test.p_labels.get();
	if (!labels.empty()) {
		m_Fp << "  " << LabelsToProp(labels) << "\n";
	}

	for (auto& props : m_PropsStack) {
		for (auto& prop : props) {
			m_Fp << "  " << prop << "\n";
		}
	}

	if (!test.is_enabled()) {
		m_Fp << "  DISABLED\n";
	}

	auto decs = test.p_decorators.get();
	for (auto& dec : decs) {
		auto ctp = boost::dynamic_pointer_cast<CTestProperties>(dec);
		if (ctp) {
			m_Fp << "  " << ctp->m_Props << "\n";
		}
	}

	m_Fp << ")\n";
	m_Fp.flush();
}

bool CTestFileGenerator::test_suite_start(const boost::unit_test::test_suite& suite)
{
	m_PropsStack.emplace_back();

	auto decs = suite.p_decorators.get();
	for (auto& dec : decs) {
		auto ctp = boost::dynamic_pointer_cast<CTestProperties>(dec);
		if (ctp) {
			m_PropsStack.back().push_back(ctp->m_Props);
		}
	}

	auto labels = suite.p_labels.get();
	if (!labels.empty()) {
		m_PropsStack.back().push_back(LabelsToProp(labels));
	}

	return true;
}

void CTestFileGenerator::test_suite_finish(const boost::unit_test::test_suite&)
{
	m_PropsStack.pop_back();
}

std::string CTestFileGenerator::LabelsToProp(const std::vector<std::string>& labels)
{
	std::string labelsProp{"LABELS \""};
	for (auto it = labels.begin(); it != labels.end(); ++it) {
		if (it != labels.begin()) {
			labelsProp.push_back(';');
		}

		for (auto c : *it) {
			if (c == ';') {
				labelsProp.push_back('\\');
			}
			labelsProp.push_back(c);
		}
	}
	labelsProp.push_back('"');
	return labelsProp;
}

CTestFileGeneratorFixture::CTestFileGeneratorFixture()
{
	auto& mts = boost::unit_test::framework::master_test_suite();
	int argc = mts.argc;
	for (int i = 1; i < argc; i++) {
		std::string argument(mts.argv[i]);

		if (argument == "--generate_ctest_config") {
			if (argc >= i + 1) {
				boost::filesystem::path testbin(mts.argv[0]);
				boost::filesystem::path file(mts.argv[i + 1]);

				try {
					CTestFileGenerator visitor{testbin, file};
					traverse_test_tree(mts, visitor);
				} catch (const std::exception& ex) {
					std::cerr << "Error: --generate_ctest_config failed with: " << ex.what() << '\n';
					std::_Exit(EXIT_FAILURE);
				}
			} else {
				std::cerr << "Error: --generate_ctest_config specified with no argument.\n";
				std::_Exit(EXIT_FAILURE);
			}

			std::_Exit(EXIT_SUCCESS);
		}
	}
}
