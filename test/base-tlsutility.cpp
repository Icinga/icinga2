/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "base/configuration.hpp"
#include "base/tlsutility.hpp"
#include "base/utility.hpp"
#include "remote/apilistener.hpp"
#include "remote/pkiutility.hpp"
#include <BoostTestTargetConfig.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <functional>
#include <memory>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <utility>
#include <vector>

using namespace icinga;

struct ConfigurationConstantsFixture
{
	String TmpDir;
	String PreviousDataDir;

	ConfigurationConstantsFixture()
	{
		TmpDir = boost::filesystem::detail::temp_directory_path().string() + "/icinga2";
		PreviousDataDir = Configuration::DataDir;
		Configuration::DataDir = TmpDir;
	}

	~ConfigurationConstantsFixture()
	{
		Configuration::DataDir = PreviousDataDir;
		Utility::RemoveDirRecursive(TmpDir);
	}
};

static const long l_2016 = 1480000000; // Thu Nov 24 15:06:40 UTC 2016
static const long l_2017 = 1490000000; // Mon Mar 20 08:53:20 UTC 2017

BOOST_AUTO_TEST_SUITE(base_tlsutility)

BOOST_AUTO_TEST_CASE(sha1)
{
	std::string allchars;
	for (size_t i = 0; i < 256; i++) {
		allchars.push_back(i);
	}

	std::vector<std::pair<std::string,std::string>> testdata = {
		{"",                        "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
		{"icinga",                  "f172c5e9e4d840a55356882a2b644846b302b216"},
		{"Icinga",                  "b3bdae77f60d9065f6152c7e3bbd351fa65e6fab"},
		{"ICINGA",                  "335da1d814abeef09b4623e2ce5169140c267a39"},
		{"#rX|wlcM:.8)uVmxz",       "99dc4d34caf36c6d6b08404135f1a7286211be1e"},
		{"AgbM;Z8Tz1!Im,kecZWs",    "aa793bef1ca307012980ae5ae046b7e929f6ed99"},
		{"yLUA4vKQ~24W}ahI;i?NLLS", "5e1a5ee3bd9fae5150681ef656ad43d9cb8e7005"},
		{allchars,                  "4916d6bdb7f78e6803698cab32d1586ea457dfc8"},
	};

	for (const auto& p : testdata) {
		const auto& input = p.first;
		const auto& expected = p.second;
		auto output = SHA1(input);
		BOOST_CHECK_MESSAGE(output == expected, "SHA1('" << input << "') should be " << expected << ", got " << output);
	}
}

static std::shared_ptr<EVP_PKEY> GetEVP_PKEY(const String& keyfile)
{
	BIO *cakeybio = BIO_new_file(keyfile.CStr(), "r");
	BOOST_REQUIRE_MESSAGE(cakeybio, "BIO_new_file() for private key from'" << keyfile << "': " << ERR_error_string(ERR_get_error(), nullptr));

	RSA* rsa = PEM_read_bio_RSAPrivateKey(cakeybio, nullptr, nullptr, nullptr);
	BOOST_REQUIRE_MESSAGE(rsa, "PEM read bio RSA key from private key file '" << keyfile << "': " << ERR_error_string(ERR_get_error(), nullptr));

	BIO_free(cakeybio);
	BOOST_CHECK_EQUAL(1, RSA_check_key(rsa)); // 1 == valid, 0 == invalid

	EVP_PKEY *pkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(pkey, rsa);
	BOOST_CHECK_EQUAL(EVP_PKEY_RSA, EVP_PKEY_id(pkey));

	return std::shared_ptr<EVP_PKEY>(pkey, EVP_PKEY_free);
}

BOOST_AUTO_TEST_CASE(create_verify_ca)
{
	ConfigurationConstantsFixture f;

	BOOST_CHECK_EQUAL(0, PkiUtility::NewCa());

	auto cacert(GetX509Certificate(ApiListener::GetCaDir()+"/ca.crt"));
	if (OPENSSL_VERSION_NUMBER >= 0x10100000L) {
		// OpenSSL 1.1.x provides https://www.openssl.org/docs/man1.1.0/man3/X509_check_ca.html
		BOOST_CHECK_EQUAL(true, IsCa(cacert));
	} else {
		BOOST_CHECK_THROW(IsCa(cacert), std::invalid_argument);
	}
	BOOST_CHECK_EQUAL(true, VerifyCertificate(cacert, cacert, String()));
	BOOST_CHECK_EQUAL(true, IsCaUptodate(cacert.get()));

	time_t caValidUntil(time(nullptr) + ROOT_VALID_FOR);
	BOOST_CHECK_EQUAL(-1, X509_cmp_time(X509_get_notAfter(cacert.get()), &caValidUntil));

	// Set the CA certificate to expire in 100 days, i.e. less than the LEAF_VALID_FOR threshold of 397 days.
	BOOST_CHECK(X509_gmtime_adj(X509_get_notAfter(cacert.get()), 60*60*24*100));
	BOOST_CHECK_EQUAL(false, IsCaUptodate(cacert.get()));

	// Even if the CA is going to expire at exactly the same time as the LEAF_VALID_FOR threshold,
	// it is still considered to be outdated.
	BOOST_CHECK(X509_gmtime_adj(X509_get_notAfter(cacert.get()), 60*60*24*397));
	BOOST_CHECK_EQUAL(false, IsCaUptodate(cacert.get()));

	// Reset the CA expiration date to the original value, i.e. 15 years.
	BOOST_CHECK(X509_gmtime_adj(X509_get_notAfter(cacert.get()), ROOT_VALID_FOR));
	BOOST_CHECK_EQUAL(true, IsCaUptodate(cacert.get()));
}

BOOST_AUTO_TEST_CASE(create_verify_leaf_certs)
{
	ConfigurationConstantsFixture f;

	String caDir = ApiListener::GetCaDir();
	String certsDir = ApiListener::GetCertsDir();
	Utility::MkDirP(certsDir, 0700);

	BOOST_CHECK_EQUAL(0, PkiUtility::NewCa());

	auto caprivatekey(GetEVP_PKEY(caDir+"/ca.key"));
	auto cacert(GetX509Certificate(caDir+"/ca.crt"));
	BOOST_CHECK_EQUAL(true, IsCaUptodate(cacert.get()));
	BOOST_CHECK_EQUAL(1, X509_verify(cacert.get(), caprivatekey.get())); // 1 == equal, 0 == unequal, -1 == error

	BOOST_CHECK_EQUAL(0, PkiUtility::NewCert("example.com", certsDir+"example.key", certsDir+"example.csr", certsDir+"example.crt"));
	BOOST_CHECK_EQUAL(0, PkiUtility::SignCsr(certsDir+"example.csr", certsDir+"example.crt"));

	auto cert(GetX509Certificate(certsDir+"/example.crt"));
	if (OPENSSL_VERSION_NUMBER >= 0x10100000L) {
		BOOST_CHECK_EQUAL(false, IsCa(cert));
	} else {
		BOOST_CHECK_THROW(IsCa(cert), std::invalid_argument);
	}
	BOOST_CHECK_EQUAL(true, IsCertUptodate(cert));
	BOOST_CHECK_EQUAL(true, VerifyCertificate(cacert, cert, String()));

	time_t certValidUntil(time(nullptr) + LEAF_VALID_FOR);
	BOOST_CHECK_EQUAL(-1, X509_cmp_time(X509_get_notAfter(cert.get()), &certValidUntil));

	// Set the certificate to expire in 20 days, i.e. less than the RENEW_THRESHOLD of 30 days.
	BOOST_CHECK(X509_gmtime_adj(X509_get_notAfter(cert.get()), 60*60*24*20));
	BOOST_CHECK_EQUAL(false, IsCertUptodate(cert));

	// Check whether expired certificates are correctly detected and verification fails.
	BOOST_CHECK(X509_gmtime_adj(X509_get_notAfter(cert.get()), -10));
	BOOST_CHECK_EQUAL(false, IsCertUptodate(cert));
	BOOST_CHECK_THROW(VerifyCertificate(cacert, cert, String()), openssl_error);

	// Reset the certificate expiration date to the original value, i.e. 397 days.
	BOOST_CHECK(X509_gmtime_adj(X509_get_notAfter(cert.get()), LEAF_VALID_FOR));
	BOOST_CHECK_EQUAL(true, IsCertUptodate(cert));
	BOOST_CHECK_EQUAL(true, VerifyCertificate(cacert, cert, String()));

	// Set the certificate validity start date to 2016, all certificates created before 2017 are considered outdated.
	BOOST_CHECK(X509_gmtime_adj(X509_get_notBefore(cert.get()), -(time(nullptr)-l_2016)));
	BOOST_CHECK_EQUAL(false, IsCertUptodate(cert));
	BOOST_CHECK_EQUAL(true, VerifyCertificate(cacert, cert, String()));

	// Reset the certificate validity start date to the least acceptable value, i.e. 2017.
	BOOST_CHECK(X509_gmtime_adj(X509_get_notBefore(cert.get()), -(time(nullptr)-l_2017)));
	BOOST_CHECK_EQUAL(true, IsCertUptodate(cert));
	BOOST_CHECK_EQUAL(true, VerifyCertificate(cacert, cert, String()));

	// Even if the leaf is up-to-date, the root CA has expired 10 days ago, so verification should fail.
	BOOST_CHECK(X509_gmtime_adj(X509_get_notAfter(cacert.get()), -10));
	BOOST_CHECK_EQUAL(false, IsCaUptodate(cacert.get()));
	BOOST_CHECK_THROW(VerifyCertificate(cacert, cert, String()), openssl_error);

	// Generate a new CA certificate to simulate a renewal and check whether verification still works.
	std::shared_ptr<EVP_PKEY> caPubKey(X509_get_pubkey(cacert.get()), EVP_PKEY_free);
	auto subject(X509_get_subject_name(cacert.get()));
	auto newCACert(CreateCertIcingaCA(caPubKey.get(), subject, true));
	BOOST_REQUIRE(newCACert);
	BOOST_CHECK_EQUAL(1, X509_verify(newCACert.get(), caprivatekey.get())); // 1 == equal, 0 == unequal, -1 == error
	BOOST_CHECK_EQUAL(true, IsCaUptodate(newCACert.get()));
	BOOST_CHECK_EQUAL(true, VerifyCertificate(newCACert, newCACert, String()));
	BOOST_CHECK_EQUAL(true, VerifyCertificate(newCACert, cert, String()));
	BOOST_CHECK_THROW(VerifyCertificate(cacert, newCACert, String()), openssl_error);

	// Remove the previously generated CA before regenerating a new one, PkiUtility::NewCa() would fail otherwise.
	Utility::RemoveDirRecursive(caDir);
	BOOST_CHECK_EQUAL(0, PkiUtility::NewCa());

	newCACert = GetX509Certificate(caDir+"/ca.crt");
	auto newCAPrivateKey = GetEVP_PKEY(caDir+"/ca.key");
	BOOST_REQUIRE(newCACert);
	BOOST_CHECK_NE(0, ASN1_INTEGER_cmp(X509_get_serialNumber(cacert.get()), X509_get_serialNumber(newCACert.get())));
	BOOST_CHECK_NE(1, EVP_PKEY_cmp(X509_get_pubkey(cacert.get()), X509_get_pubkey(newCACert.get())));

	BOOST_CHECK_MESSAGE(1 == X509_verify(newCACert.get(), newCAPrivateKey.get()), "Failed to verify new CA certificate: " << ERR_error_string(ERR_get_error(), nullptr));
	BOOST_CHECK_MESSAGE(1 > X509_verify(newCACert.get(), caprivatekey.get()), "New CA certificate should not be verifiable with the old private key");
	BOOST_CHECK_MESSAGE(1 > X509_verify(cacert.get(), newCAPrivateKey.get()), "Old CA certificate should not be verifiable with the new private key");

	BOOST_CHECK_EQUAL(true, IsCaUptodate(newCACert.get()));
	BOOST_CHECK_EQUAL(true, VerifyCertificate(newCACert, newCACert, String())); // Self-signed CA!
	// Verification should fail because the leaf certificate was signed by the old CA.
	BOOST_CHECK_THROW(VerifyCertificate(newCACert, StringToCertificate(CertificateToString(cert)), String()), openssl_error);

	// Renew the leaf certificate and check whether verification works with the new CA.
	std::shared_ptr<EVP_PKEY> pubkey(X509_get_pubkey(cert.get()), EVP_PKEY_free);
	auto leafSubject(X509_get_subject_name(cert.get()));
	auto newCert(CreateCertIcingaCA(pubkey.get(), leafSubject, false));
	BOOST_REQUIRE(newCert);
	BOOST_CHECK_EQUAL(true, IsCertUptodate(newCert));
	BOOST_CHECK_EQUAL(true, VerifyCertificate(newCACert, newCert, String()));
	// Verification should fail because the new leaf certificate was signed by the newly generated CA.
	BOOST_CHECK_THROW(VerifyCertificate(cacert, newCert, String()), openssl_error);
}

BOOST_AUTO_TEST_SUITE_END()
