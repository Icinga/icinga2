// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <BoostTestTargetConfig.h>
#include "remote/apilistener.hpp"
#include "test/base-configuration-fixture.hpp"
#include "test/test-ctest.hpp"

namespace icinga {

struct CertificateFixture : ConfigurationDataDirFixture
{
	CertificateFixture()
	{
		namespace fs = boost::filesystem;

		m_CaDir = ApiListener::GetCaDir();
		m_CertsDir = ApiListener::GetCertsDir();
		m_CaCrtFile = m_CertsDir / "ca.crt";

		Utility::MkDirP((m_PersistentCertsDir / "ca").string(), 0700);
		Utility::MkDirP((m_PersistentCertsDir / "certs").string(), 0700);

		Utility::MkDirP(m_CaDir.string(), 0700);
		for (const auto& entry : fs::directory_iterator{m_PersistentCertsDir / "ca"}) {
			Utility::CopyFile(entry.path().string(), (m_CaDir / entry.path().filename()).string());
		}

		Utility::MkDirP(m_CertsDir.string(), 0700);
		for (const auto& entry : fs::directory_iterator{m_PersistentCertsDir / "certs"}) {
			Utility::CopyFile(entry.path().string(), (m_CertsDir / entry.path().filename()).string());
		}
	}

	[[nodiscard]] auto EnsureCertFor(const std::string& name) const
	{
		struct Cert
		{
			String crtFile;
			String keyFile;
			String csrFile;
		};

		Cert cert;
		cert.crtFile = (m_CertsDir / (name + ".crt")).string();
		cert.keyFile = (m_CertsDir / (name + ".key")).string();
		cert.csrFile = (m_CertsDir / (name + ".csr")).string();

		BOOST_REQUIRE(Utility::PathExists(cert.crtFile));
		BOOST_REQUIRE(Utility::PathExists(cert.keyFile));
		BOOST_REQUIRE(Utility::PathExists(cert.csrFile));

		return cert;
	}

	boost::filesystem::path m_CaDir;
	boost::filesystem::path m_CertsDir;
	boost::filesystem::path m_CaCrtFile;
	static inline const boost::filesystem::path
		m_PersistentCertsDir = boost::filesystem::current_path() / "persistent" / "certs";
};

/**
 * A unit-test decorator that declares that the test requires a given set of certificates.
 *
 * If no list of certs is given to the constructor, the test will still require the CA to be
 * generated before it executes.
 *
 * When adding this decorator, a CTest fixture (in the form of new setup and cleanup units) is
 * generated for the CA and each certificate. These fixtures are shared by all other test-cases
 * that require them and will ensure that the CA/certificates exist until all those tests complete.
 *
 * If the prefix argument is given to the constructor, A separate CA is generated and the
 * test-cases will be ensured to not run in parallel to other tests that use a different or no
 * prefix.
 */
class RequiresCertificate : public CTestPropertiesBase{
public:
	explicit RequiresCertificate(const std::vector<String>& certs, const String& prefix = "");

	std::string Get() override;

private:
	std::vector<String> m_RequiredFixtures;
	static inline std::vector<String> m_CertFixtures;
	static inline std::vector<String> m_CaFixtures;

	static void AddCaFixture(const String& caFixtureName);
	static void AddCertFixture(const String& cn, const String& caFixture, const String& certFixture);

	[[nodiscard]] boost::unit_test::decorator::base_ptr clone() const override{
		return boost::unit_test::decorator::base_ptr{new RequiresCertificate{*this}};
	}

	void apply(boost::unit_test::test_unit&) override {}
};

} // namespace icinga
