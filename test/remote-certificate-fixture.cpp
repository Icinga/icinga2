/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "remote-certificate-fixture.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

const boost::filesystem::path CertificateFixture::m_PersistentCertsDir =
	boost::filesystem::current_path() / "persistent" / "certs";

BOOST_AUTO_TEST_SUITE(remote_certs_fixture)

/**
 * Recursively removes the directory that contains the test certificates.
 *
 * This needs to be done once initially to prepare the directory, in case there are any
 * left-overs from previous test runs, and once after all tests using the certificates
 * have been completed.
 *
 * This dependency is expressed as a CTest fixture and not a boost-test one, because that
 * is the only way to have persistency between individual test-cases with CTest.
 */
static void CleanupPersistentCertificateDir()
{
	if (boost::filesystem::exists(CertificateFixture::m_PersistentCertsDir)) {
		boost::filesystem::remove_all(CertificateFixture::m_PersistentCertsDir);
	}
}

BOOST_FIXTURE_TEST_CASE(prepare_directory, ConfigurationDataDirFixture)
{
	// Remove any existing left-overs of the persistent certificate directory from a previous
	// test run.
	CleanupPersistentCertificateDir();
}

BOOST_FIXTURE_TEST_CASE(cleanup_certs, ConfigurationDataDirFixture)
{
	CleanupPersistentCertificateDir();
}

BOOST_AUTO_TEST_SUITE_END()
