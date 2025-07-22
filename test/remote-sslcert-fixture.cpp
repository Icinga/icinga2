/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "remote-sslcert-fixture.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

const boost::filesystem::path CertificateFixture::m_PersistentCertsDir = boost::filesystem::current_path() / "persistent" / "certs";

BOOST_AUTO_TEST_SUITE(remote_certs)

BOOST_FIXTURE_TEST_CASE(setup_certs, ConfigurationDataDirFixture)
{
	if (boost::filesystem::exists(CertificateFixture::m_PersistentCertsDir)) {
		boost::filesystem::remove_all(CertificateFixture::m_PersistentCertsDir);
	}
}

BOOST_FIXTURE_TEST_CASE(cleanup_certs, ConfigurationDataDirFixture)
{
	if (boost::filesystem::exists(CertificateFixture::m_PersistentCertsDir)) {
		boost::filesystem::remove_all(CertificateFixture::m_PersistentCertsDir);
	}
}

BOOST_AUTO_TEST_SUITE_END()
