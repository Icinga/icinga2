/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "base/tlsutility.hpp"
#include <BoostTestTargetConfig.h>
#include <functional>
#include <memory>
#include <openssl/asn1.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <utility>
#include <vector>

using namespace icinga;

static EVP_PKEY* GenKeypair()
{
	InitializeOpenSSL();

	auto e (BN_new());
	BOOST_REQUIRE(e);

	auto rsa (RSA_new());
	BOOST_REQUIRE(rsa);

	auto key (EVP_PKEY_new());
	BOOST_REQUIRE(key);

	BOOST_REQUIRE(BN_set_word(e, RSA_F4));
	BOOST_REQUIRE(RSA_generate_key_ex(rsa, 4096, e, nullptr));
	BOOST_REQUIRE(EVP_PKEY_assign_RSA(key, rsa));

	return key;
}

static std::shared_ptr<X509> MakeCert(const char* issuer, EVP_PKEY* signer, const char* subject, EVP_PKEY* pubkey, std::function<void(ASN1_TIME*, ASN1_TIME*)> setTimes)
{
	auto cert (X509_new());
	BOOST_REQUIRE(cert);

	auto serial (BN_new());
	BOOST_REQUIRE(serial);

	BOOST_REQUIRE(X509_set_version(cert, 0x2));
	BOOST_REQUIRE(BN_to_ASN1_INTEGER(serial, X509_get_serialNumber(cert)));
	BOOST_REQUIRE(X509_NAME_add_entry_by_NID(X509_get_issuer_name(cert), NID_commonName, MBSTRING_ASC, (unsigned char*)issuer, -1, -1, 0));
	setTimes(X509_get_notBefore(cert), X509_get_notAfter(cert));
	BOOST_REQUIRE(X509_NAME_add_entry_by_NID(X509_get_subject_name(cert), NID_commonName, MBSTRING_ASC, (unsigned char*)subject, -1, -1, 0));
	BOOST_REQUIRE(X509_set_pubkey(cert, pubkey));
	BOOST_REQUIRE(X509_sign(cert, signer, EVP_sha256()));

	return std::shared_ptr<X509>(cert, X509_free);
}

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

BOOST_AUTO_TEST_CASE(iscauptodate_ok)
{
	auto key (GenKeypair());

	BOOST_CHECK(IsCaUptodate(MakeCert("Icinga CA", key, "Icinga CA", key, [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		BOOST_REQUIRE(X509_gmtime_adj(notBefore, 0));
		BOOST_REQUIRE(X509_gmtime_adj(notAfter, LEAF_VALID_FOR + 60 * 60));
	}).get()));
}

BOOST_AUTO_TEST_CASE(iscauptodate_expiring)
{
	auto key (GenKeypair());

	BOOST_CHECK(!IsCaUptodate(MakeCert("Icinga CA", key, "Icinga CA", key, [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		BOOST_REQUIRE(X509_gmtime_adj(notBefore, 0));
		BOOST_REQUIRE(X509_gmtime_adj(notAfter, LEAF_VALID_FOR - 60 * 60));
	}).get()));
}

BOOST_AUTO_TEST_CASE(iscertuptodate_ok)
{
	BOOST_CHECK(IsCertUptodate(MakeCert("Icinga CA", GenKeypair(), "example.com", GenKeypair(), [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		time_t epoch = 0;
		BOOST_REQUIRE(X509_time_adj(notBefore, l_2017, &epoch));
		BOOST_REQUIRE(X509_gmtime_adj(notAfter, RENEW_THRESHOLD + 60 * 60));
	})));
}

BOOST_AUTO_TEST_CASE(iscertuptodate_expiring)
{
	BOOST_CHECK(!IsCertUptodate(MakeCert("Icinga CA", GenKeypair(), "example.com", GenKeypair(), [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		time_t epoch = 0;
		BOOST_REQUIRE(X509_time_adj(notBefore, l_2017, &epoch));
		BOOST_REQUIRE(X509_gmtime_adj(notAfter, RENEW_THRESHOLD - 60 * 60));
	})));
}

BOOST_AUTO_TEST_CASE(iscertuptodate_old)
{
	BOOST_CHECK(!IsCertUptodate(MakeCert("Icinga CA", GenKeypair(), "example.com", GenKeypair(), [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		time_t epoch = 0;
		BOOST_REQUIRE(X509_time_adj(notBefore, l_2016, &epoch));
		BOOST_REQUIRE(X509_gmtime_adj(notAfter, RENEW_THRESHOLD + 60 * 60));
	})));
}

BOOST_AUTO_TEST_CASE(VerifyCertificate_revalidate)
{
	X509_NAME *caSubject = X509_NAME_new();
	X509_NAME_add_entry_by_txt(caSubject, "CN", MBSTRING_ASC, (const unsigned char*)"Icinga CA", -1, -1, 0);

	auto signingCaKey = GenKeypair();
	auto signingCaCert = CreateCert(signingCaKey, caSubject, caSubject, signingCaKey, true);

	X509_NAME *leafSubject = X509_NAME_new();
	X509_NAME_add_entry_by_txt(leafSubject, "CN", MBSTRING_ASC, (const unsigned char*)"Leaf Certificate", -1, -1, 0);
	auto leafKey = GenKeypair();
	auto leafCert = CreateCert(leafKey, leafSubject, caSubject, signingCaKey, false);
	BOOST_CHECK(VerifyCertificate(signingCaCert, leafCert, "", ""));

	// Create a second CA with a different key, the leaf certificate is supposed to fail validation against that CA.
	auto otherCaKey = GenKeypair();
	auto otherCaCert = CreateCert(otherCaKey, caSubject, caSubject, otherCaKey, true);
	BOOST_CHECK_THROW(VerifyCertificate(otherCaCert, leafCert, "", ""), openssl_error);
}

BOOST_AUTO_TEST_SUITE_END()
