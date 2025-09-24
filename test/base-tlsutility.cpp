/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "base/tlsutility.hpp"
#include "base/utility.hpp"
#include "remote/pkiutility.hpp"
#include "test/remote-certificate-fixture.hpp"
#include <BoostTestTargetConfig.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <array>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

using namespace icinga;

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

static String GetOpenSSLError()
{
	std::array<char, 256> errBuf;
	ERR_error_string_n(ERR_get_error(), errBuf.data(), errBuf.size());
	return {errBuf.data()};
}

static std::shared_ptr<EVP_PKEY> GetEVP_PKEY(const String& keyfile)
{
	BIO *cakeybio = BIO_new_file(keyfile.CStr(), "r");
	BOOST_REQUIRE_MESSAGE(cakeybio, "BIO_new_file() for private key from'" << keyfile << "': " << GetOpenSSLError());

	RSA* rsa = PEM_read_bio_RSAPrivateKey(cakeybio, nullptr, nullptr, nullptr);
	BOOST_REQUIRE_MESSAGE(rsa, "PEM read bio RSA key from private key file '" << keyfile << "': " << GetOpenSSLError());

	BIO_free(cakeybio);
	BOOST_CHECK_EQUAL(1, RSA_check_key(rsa)); // 1 == valid, 0 == invalid

	EVP_PKEY *pkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(pkey, rsa);
	BOOST_CHECK_EQUAL(EVP_PKEY_RSA, EVP_PKEY_id(pkey));

	return {pkey, EVP_PKEY_free};
}

/**
 * Creates a new X509 certificate signed by the Icinga CA based on an existing CA certificate.
 *
 * It uses the public key and subject name from the existing CA certificate to create a new certificate.
 *
 * @param ca The existing CA certificate.
 * @param validFor The validity period in seconds.
 * @param validFrom The start time offset in seconds.
 * @param caCert Whether to create a CA certificate (true) or a leaf certificate (false).
 * @returns The new X509 certificate or an empty shared_ptr on error.
 */
auto NewCertFromExisting(const std::shared_ptr<X509>& ca, long validFrom, long validFor, bool caCert = false)
{
	std::shared_ptr<EVP_PKEY> caPubKey(X509_get_pubkey(ca.get()), EVP_PKEY_free);
	return CreateCertIcingaCA(caPubKey.get(), X509_get_subject_name(ca.get()), validFrom, validFor, caCert);
}

/**
 * Creates an ASN1_TIME object representing the current time plus the specified number of seconds.
 *
 * @param seconds The number of seconds to add to the current time.
 * @returns A shared pointer to the ASN1_TIME object.
 */
static std::shared_ptr<ASN1_TIME> MakeASN1TimeFrom(long seconds)
{
	auto now = time(nullptr);
	return {X509_time_adj_ex(nullptr, 0, seconds, &now), ASN1_TIME_free};
}

/**
 * Formats an ASN1_TIME object as a human-readable string.
 *
 * @param t The ASN1_TIME object to format.
 *
 * @returns A string representation of the ASN1_TIME object.
 */
static std::string FormatAsn1Time(const ASN1_TIME* t)
{
	BIO *bio = BIO_new(BIO_s_mem());
	ASN1_TIME_print(bio, t);
	char *data;
	long len = BIO_get_mem_data(bio, &data);
	std::string result(data, data + len);
	BIO_free(bio);
	return result;
}

BOOST_FIXTURE_TEST_CASE(create_verify_ca, CertificateFixture)
{
	auto cacert(GetX509Certificate(m_CaDir.string()+"/ca.crt"));
	if constexpr (OPENSSL_VERSION_NUMBER >= 0x10100000L) {
		// OpenSSL 1.1.x provides https://www.openssl.org/docs/man1.1.0/man3/X509_check_ca.html
		BOOST_CHECK(IsCa(cacert));
	} else {
		BOOST_CHECK_THROW(IsCa(cacert), std::invalid_argument);
	}
	BOOST_CHECK(VerifyCertificate(cacert, cacert, String())); // Self-signed CA!
	BOOST_CHECK(IsCaUptodate(cacert.get())); // Is CA up-to-date after its creation?
	auto validUntil = MakeASN1TimeFrom(ROOT_VALID_FOR);
	BOOST_CHECK_MESSAGE(0 >= Asn1TimeCompare(X509_get_notAfter(cacert.get()), validUntil.get()),
		"CA should expire within " << std::quoted(FormatAsn1Time(validUntil.get()))
		<< ", notAfter: " << std::quoted(FormatAsn1Time(X509_get_notAfter(cacert.get()))));

	// Set the CA certificate to expire in 100 days, i.e. less than the LEAF_VALID_FOR threshold of 397 days.
	cacert = NewCertFromExisting(cacert, 0, 100, true);
	BOOST_CHECK(!IsCaUptodate(cacert.get())); // Is CA outdated now?

	cacert = NewCertFromExisting(cacert, 0, LEAF_VALID_FOR-1, true);
	BOOST_CHECK(!IsCaUptodate(cacert.get())); // Still outdated, as it's less than LEAF_VALID_FOR.

	// Reset the CA expiration date to the original value, i.e. 15 years.
	cacert = NewCertFromExisting(cacert, 0, ROOT_VALID_FOR, true);
	BOOST_CHECK(IsCaUptodate(cacert.get()));
}

BOOST_FIXTURE_TEST_CASE(create_verify_leaf_certs, CertificateFixture)
{
	String caDir = m_CaDir.string();
	String certsDir = m_CertsDir.string();

	auto caprivatekey(GetEVP_PKEY(caDir+"/ca.key"));
	auto cacert(GetX509Certificate(caDir+"/ca.crt"));
	BOOST_CHECK(IsCaUptodate(cacert.get()));
	BOOST_CHECK_EQUAL(1, X509_verify(cacert.get(), caprivatekey.get())); // 1 == equal, 0 == unequal, -1 == error

	auto certInfo = EnsureCertFor("example.com", true); // Generates example.com.{key,csr,crt} files.

	auto cert(GetX509Certificate(certInfo.crtFile));
	if constexpr (OPENSSL_VERSION_NUMBER >= 0x10100000L) {
		BOOST_CHECK(!IsCa(cert));
	} else {
		BOOST_CHECK_THROW(IsCa(cert), std::invalid_argument);
	}
	BOOST_CHECK(IsCertUptodate(cert)); // Is leaf up-to-date after its creation?
	BOOST_CHECK(VerifyCertificate(cacert, cert, String())); // Signed by our CA?
	auto validUntil = MakeASN1TimeFrom(LEAF_VALID_FOR);
	BOOST_CHECK_MESSAGE(0 >= Asn1TimeCompare(X509_get_notAfter(cert.get()), validUntil.get()),
		"Leaf certificate should expire within " << std::quoted(FormatAsn1Time(validUntil.get()))
		<< ", notAfter: " << std::quoted(FormatAsn1Time(X509_get_notAfter(cert.get()))));

	// Set the certificate to expire in 20 days, i.e. less than the RENEW_THRESHOLD of 30 days.
	cert = NewCertFromExisting(cert, 0, 20);
	BOOST_CHECK(!IsCertUptodate(cert));
	BOOST_CHECK(VerifyCertificate(cacert, cert, String())); // Verification should still work.

	// Check whether expired certificates are correctly detected and verification fails.
	cert = NewCertFromExisting(cert, -LEAF_VALID_FOR, -10*24*60*60); // Expire 10 days ago!
	validUntil = MakeASN1TimeFrom(-10*24*60*60);
	BOOST_CHECK_MESSAGE(0 >= Asn1TimeCompare(X509_get_notAfter(cert.get()), validUntil.get()), // Is certificate indeed expired?
		"Leaf certificate should have expired on " << std::quoted(FormatAsn1Time(validUntil.get()))
		<< ", notAfter: " << std::quoted(FormatAsn1Time(X509_get_notAfter(cert.get()))));
	BOOST_CHECK(!IsCertUptodate(cert)); // It's already expired, so definitely not up-to-date.
	auto assertForCertHasExpired = [](const openssl_error& e) {
		const unsigned long* opensslCode = boost::get_error_info<errinfo_openssl_error>(e);
		BOOST_REQUIRE(opensslCode);
		BOOST_REQUIRE_EQUAL(*opensslCode, X509_V_ERR_CERT_HAS_EXPIRED);
		return *opensslCode == X509_V_ERR_CERT_HAS_EXPIRED;
	};
	BOOST_CHECK_EXCEPTION(VerifyCertificate(cacert, cert, String()), openssl_error, assertForCertHasExpired);

	// Set the certificate validity start date to 2016, all certificates created before 2017 are considered outdated.
	cert = NewCertFromExisting(cert, -(time(nullptr)-l_2016), LEAF_VALID_FOR);
	BOOST_CHECK(!IsCertUptodate(cert));
	// ... but verification should still work, as the certificate is still valid.
	BOOST_CHECK(VerifyCertificate(cacert, cert, String()));

	// Reset the certificate validity start date to the least acceptable value, i.e. 2017.
	cert = NewCertFromExisting(cert, -(time(nullptr)-l_2017), LEAF_VALID_FOR);
	BOOST_CHECK(IsCertUptodate(cert));
	BOOST_CHECK(VerifyCertificate(cacert, cert, String()));

	cacert = NewCertFromExisting(cacert, -LEAF_VALID_FOR, -10*24*60*60, true); // Expire the CA 10 days ago.
	BOOST_CHECK_EQUAL(1, X509_verify(cacert.get(), caprivatekey.get())); // 1 == equal, 0 == unequal, -1 == error
	BOOST_CHECK(!IsCaUptodate(cacert.get()));
	BOOST_CHECK_EXCEPTION(VerifyCertificate(cacert, cert, String()), openssl_error, assertForCertHasExpired);

	// Generate a new CA certificate to simulate a renewal and check whether verification still works.
	auto newCACert = NewCertFromExisting(cacert, 0, ROOT_VALID_FOR, true);
	BOOST_REQUIRE(newCACert);
	BOOST_CHECK_EQUAL(1, X509_verify(newCACert.get(), caprivatekey.get())); // 1 == equal, 0 == unequal, -1 == error
	BOOST_CHECK(IsCaUptodate(newCACert.get()));
	BOOST_CHECK(VerifyCertificate(newCACert, newCACert, String()));
	BOOST_CHECK(VerifyCertificate(newCACert, cert, String()));
	auto assertIMMorDZSSC = [](const openssl_error& e) { // IMM = Issuer Mismatch, DZSSC = Depth Zero Self-Signed Cert
		const unsigned long* opensslCode = boost::get_error_info<errinfo_openssl_error>(e);
		BOOST_REQUIRE(opensslCode);
#ifdef LIBRESSL_VERSION_NUMBER
		BOOST_REQUIRE_EQUAL(*opensslCode, X509_V_ERR_SUBJECT_ISSUER_MISMATCH);
		return *opensslCode == X509_V_ERR_SUBJECT_ISSUER_MISMATCH;
#else
		BOOST_REQUIRE_EQUAL(*opensslCode, X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT);
		return *opensslCode == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT;
#endif
	};
	//... but verifying the new CA with the old CA or vice versa should fail.
	BOOST_CHECK_EXCEPTION(VerifyCertificate(cacert, newCACert, String()), openssl_error, assertIMMorDZSSC);
	BOOST_CHECK_EXCEPTION(VerifyCertificate(newCACert, cacert, String()), openssl_error, assertIMMorDZSSC);

	cacert = newCACert;
	cert = NewCertFromExisting(cert, 0, LEAF_VALID_FOR);
	BOOST_CHECK(IsCertUptodate(cert));
	BOOST_CHECK(VerifyCertificate(cacert, cert, String()));

	// Remove the previously generated CA before regenerating a new one, PkiUtility::NewCa() would fail otherwise.
	Utility::Remove(caDir+"/ca.crt");
	Utility::Remove(caDir+"/ca.key");
	BOOST_CHECK_EQUAL(0, PkiUtility::NewCa());

	newCACert = GetX509Certificate(caDir+"/ca.crt");
	auto newCAPrivateKey = GetEVP_PKEY(caDir+"/ca.key");
	BOOST_REQUIRE(newCACert);
	BOOST_CHECK_NE(0, ASN1_INTEGER_cmp(X509_get_serialNumber(cacert.get()), X509_get_serialNumber(newCACert.get())));
	BOOST_CHECK_NE(1, EVP_PKEY_cmp(X509_get_pubkey(cacert.get()), X509_get_pubkey(newCACert.get())));

	BOOST_CHECK_MESSAGE(1 == X509_verify(newCACert.get(), newCAPrivateKey.get()), "Failed to verify new CA certificate: " << GetOpenSSLError());
	BOOST_CHECK_MESSAGE(1 > X509_verify(newCACert.get(), caprivatekey.get()), "New CA certificate should not be verifiable with the old private key");
	BOOST_CHECK_MESSAGE(1 > X509_verify(cacert.get(), newCAPrivateKey.get()), "Old CA certificate should not be verifiable with the new private key");

	BOOST_CHECK(IsCaUptodate(newCACert.get()));
	BOOST_CHECK(VerifyCertificate(newCACert, newCACert, String())); // Self-signed CA!
	// Verification should fail because the leaf certificate was signed by the old CA.
	BOOST_CHECK_EXCEPTION(VerifyCertificate(newCACert, cert, String()), openssl_error, [](const openssl_error& e) {
		const unsigned long* opensslCode = boost::get_error_info<errinfo_openssl_error>(e);
		BOOST_REQUIRE(opensslCode);
		BOOST_REQUIRE_EQUAL(*opensslCode, X509_V_ERR_CERT_SIGNATURE_FAILURE);
		return *opensslCode == X509_V_ERR_CERT_SIGNATURE_FAILURE;
	});

	// Renew the leaf certificate and check whether verification works with the new CA.
	auto newCert = NewCertFromExisting(cert, 0, LEAF_VALID_FOR, false);
	BOOST_CHECK(IsCertUptodate(newCert));
	BOOST_CHECK(VerifyCertificate(newCACert, newCert, String()));
	// Verification should fail because the new leaf certificate was signed by the newly generated CA.
	BOOST_CHECK_EXCEPTION(VerifyCertificate(cacert, newCert, String()), openssl_error, [](const openssl_error& e) {
		const unsigned long* opensslCode = boost::get_error_info<errinfo_openssl_error>(e);
		BOOST_REQUIRE(opensslCode);
		BOOST_REQUIRE_EQUAL(*opensslCode, X509_V_ERR_CERT_SIGNATURE_FAILURE);
		return *opensslCode == X509_V_ERR_CERT_SIGNATURE_FAILURE;
	});
}

BOOST_AUTO_TEST_SUITE_END()
