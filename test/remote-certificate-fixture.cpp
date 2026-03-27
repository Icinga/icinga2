// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remote-certificate-fixture.hpp"
#include "remote/pkiutility.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/replace.hpp>

using namespace icinga;

RequiresCertificate::RequiresCertificate(const std::vector<String>& certs, const String& prefix)
{
	auto caFixtureName = "remote_" + (prefix.IsEmpty() ? "" : prefix + "_") + "ca";

	/* Add a fixture for setup and cleanup of the CA, if it doesn't exist yet.
	 */
	if (boost::range::find(m_CaFixtures, caFixtureName) == m_CaFixtures.end()) {
		AddCaFixture(caFixtureName);
	}

	/* In case the test doesn't specify any cerficates it needs to depend on the CA fixture
	 * directly.
	 */
	if (certs.empty()) {
		m_RequiredFixtures.emplace_back(caFixtureName);
	}

	for (const std::string& cert : certs) {
		auto fixtureName = "remote_" + (prefix.IsEmpty() ? "" : prefix + "_") + "cert_" + cert;
		boost::range::replace(fixtureName, '.', '_');

		m_RequiredFixtures.emplace_back(fixtureName);

		/* Add a fixture for the generation and cleanup of the certificate if it doesn't exist yet.
		 */
		if (boost::range::find(m_CertFixtures, fixtureName) == m_CertFixtures.end()) {
			AddCertFixture(cert, caFixtureName, fixtureName);
		}
	}
}

void RequiresCertificate::AddCaFixture(const String& caFixtureName)
{
	boost::unit_test::decorator::base_ptr certLabel{new boost::unit_test::label{"cert"}};
	auto& mts = boost::unit_test::framework::master_test_suite();

	auto* setup = boost::unit_test::make_test_case(
		[]() {
			CertificateFixture certFixture;
			PkiUtility::NewCa();
			auto persistentCaPath = CertificateFixture::m_PersistentCertsDir / "ca";
			auto persistentCertsPath = CertificateFixture::m_PersistentCertsDir / "certs";
			Utility::CopyFile((certFixture.m_CaDir / "ca.crt").string(), (persistentCaPath / "ca.crt").string());
			Utility::CopyFile((certFixture.m_CaDir / "ca.crt").string(), (persistentCertsPath / "ca.crt").string());
			Utility::CopyFile((certFixture.m_CaDir / "ca.key").string(), (persistentCaPath / "ca.key").string());

			BOOST_REQUIRE(Utility::PathExists((persistentCaPath / "ca.crt").string()));
			BOOST_REQUIRE(Utility::PathExists((persistentCertsPath / "ca.crt").string()));
			BOOST_REQUIRE(Utility::PathExists((persistentCaPath / "ca.key").string()));
		},
		caFixtureName.GetData() + "_setup",
		__FILE__,
		__LINE__
	);

	setup->p_decorators.value.emplace_back(new CTestProperties{"FIXTURES_SETUP " + caFixtureName});
	setup->p_decorators.value.emplace_back(certLabel);
	mts.add(setup);

	/* This ensures that tests operating under a different prefix will wait for the
	 * cleanup of another prefix to complete. For example if a group of tests operate
	 * destructively on certs or the CA, they would request their own prefix and
	 * this would ensure that the tests using that prefix run either before or after
	 * other tests.
	 */
	if (!m_CaFixtures.empty()) {
		setup->p_decorators.value.emplace_back(new CTestProperties{"DEPENDS " + m_CaFixtures.back() + "_cleanup"});
	}

	auto* cleanup = boost::unit_test::make_test_case(
		[]() {
			BOOST_REQUIRE(Utility::PathExists(CertificateFixture::m_PersistentCertsDir.string()));
			Utility::RemoveDirRecursive(CertificateFixture::m_PersistentCertsDir.string());
		},
		caFixtureName.GetData() + "_cleanup",
		__FILE__,
		__LINE__
	);

	cleanup->p_decorators.value.emplace_back(new CTestProperties{"FIXTURES_CLEANUP " + caFixtureName});
	cleanup->p_decorators.value.emplace_back(certLabel);
	mts.add(cleanup);

	m_CaFixtures.emplace_back(caFixtureName);
}

void RequiresCertificate::AddCertFixture(const String& cn, const String& caFixture, const String& certFixture)
{
	auto& mts = boost::unit_test::framework::master_test_suite();
	boost::unit_test::decorator::base_ptr certLabel{new boost::unit_test::label{"cert"}};

	auto* setup = boost::unit_test::make_test_case(
		[cn]() {
			CertificateFixture certFixture;
			auto persistentCertsPath = CertificateFixture::m_PersistentCertsDir / "certs";
			auto keyFile = persistentCertsPath / (cn.GetData() + ".key");
			auto csrFile = persistentCertsPath / (cn.GetData() + ".csr");
			auto crtFile = persistentCertsPath / (cn.GetData() + ".crt");
			PkiUtility::NewCert(cn, keyFile.string(), csrFile.string(), "");
			PkiUtility::SignCsr(csrFile.string(), crtFile.string());
		},
		certFixture.GetData() + "_setup",
		__FILE__,
		__LINE__
	);

	/* Declare this unit as the setup routine of the ctest fixture.
	 */
	setup->p_decorators.value.emplace_back(new CTestProperties{"FIXTURES_SETUP " + certFixture});
	setup->p_decorators.value.emplace_back(certLabel);
	mts.add(setup);

	/* This "cleanup" fixture doesn't do anything on its own, but it is needed to enforce
	 * the ordering of all tests that require the certificate before the cleanup of the
	 * CA (defined above).
	 */
	auto* cleanup = boost::unit_test::make_test_case([]() {}, certFixture.GetData() + "_cleanup", __FILE__, __LINE__);
	cleanup->p_decorators.value.emplace_back(new CTestProperties{"FIXTURES_CLEANUP " + certFixture});
	cleanup->p_decorators.value.emplace_back(certLabel);
	mts.add(cleanup);

	for (auto* unit : {setup, cleanup}) {
		unit->p_decorators.value.emplace_back(new CTestProperties{"FIXTURES_REQUIRED " + caFixture});
	}

	m_CertFixtures.push_back(certFixture);
}

std::string RequiresCertificate::Get()
{
	std::string props = "FIXTURES_REQUIRED \"";
	props.append(boost::algorithm::join(m_RequiredFixtures, ";"));
	props.append("\"");
	return props;
}
