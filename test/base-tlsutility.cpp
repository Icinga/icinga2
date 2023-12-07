/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "base/tlsutility.hpp"
#include <BoostTestTargetConfig.h>
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
	auto rsa (RSA_new());
	auto key (EVP_PKEY_new());

	BN_set_word(e, RSA_3);
	BOOST_REQUIRE(RSA_generate_key_ex(rsa, 512, e, nullptr));
	EVP_PKEY_assign_RSA(key, rsa);

	return key;
}

static X509* MakeCert(char* issuer, EVP_PKEY* signer, char* subject, EVP_PKEY* pubkey, void(*setTimes)(ASN1_TIME*, ASN1_TIME*))
{
	auto cert (X509_new());

	X509_set_version(cert, 0x2);
	BN_to_ASN1_INTEGER(BN_new(), X509_get_serialNumber(cert));
	X509_NAME_add_entry_by_NID(X509_get_issuer_name(cert), NID_commonName, MBSTRING_ASC, (unsigned char*)issuer, -1, -1, 0);
	setTimes(X509_get_notBefore(cert), X509_get_notAfter(cert));
	X509_NAME_add_entry_by_NID(X509_get_subject_name(cert), NID_commonName, MBSTRING_ASC, (unsigned char*)subject, -1, -1, 0);
	X509_set_pubkey(cert, pubkey);
	BOOST_REQUIRE(X509_sign(cert, signer, EVP_sha1()));

	return cert;
}

static const long l_2016 = 1480000000; // Thu Nov 24 16:06:40 CET 2016
static const long l_2017 = 1490000000; // Mon Mar 20 09:53:20 CET 2017

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

BOOST_AUTO_TEST_CASE(iscertuptodate_leaf_ok)
{
	BOOST_CHECK(IsCertUptodate(MakeCert("Icinga CA", GenKeypair(), "example.com", GenKeypair(), [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		time_t epoch = 0;
		X509_time_adj(notBefore, l_2017, &epoch);
		X509_gmtime_adj(notAfter, RENEW_THRESHOLD + 30);
	})));
}

BOOST_AUTO_TEST_CASE(iscertuptodate_leaf_expiring)
{
	BOOST_CHECK(!IsCertUptodate(MakeCert("Icinga CA", GenKeypair(), "example.com", GenKeypair(), [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		time_t epoch = 0;
		X509_time_adj(notBefore, l_2017, &epoch);
		X509_gmtime_adj(notAfter, RENEW_THRESHOLD - 30);
	})));
}

BOOST_AUTO_TEST_CASE(iscertuptodate_leaf_old)
{
	BOOST_CHECK(!IsCertUptodate(MakeCert("Icinga CA", GenKeypair(), "example.com", GenKeypair(), [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		time_t epoch = 0;
		X509_time_adj(notBefore, l_2016, &epoch);
		X509_gmtime_adj(notAfter, RENEW_THRESHOLD + 30);
	})));
}

BOOST_AUTO_TEST_CASE(iscertuptodate_root_ok)
{
	auto key (GenKeypair());

	BOOST_CHECK(IsCertUptodate(MakeCert("Icinga CA", key, "Icinga CA", key, [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		time_t epoch = 0;
		X509_time_adj(notBefore, l_2017, &epoch);
		X509_gmtime_adj(notAfter, LEAF_VALID_FOR + 30);
	})));
}

BOOST_AUTO_TEST_CASE(iscertuptodate_root_expiring)
{
	auto key (GenKeypair());

	BOOST_CHECK(!IsCertUptodate(MakeCert("Icinga CA", key, "Icinga CA", key, [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		time_t epoch = 0;
		X509_time_adj(notBefore, l_2017, &epoch);
		X509_gmtime_adj(notAfter, LEAF_VALID_FOR - 30);
	})));
}

BOOST_AUTO_TEST_CASE(iscertuptodate_root_old)
{
	auto key (GenKeypair());

	BOOST_CHECK(IsCertUptodate(MakeCert("Icinga CA", key, "Icinga CA", key, [](ASN1_TIME* notBefore, ASN1_TIME* notAfter) {
		time_t epoch = 0;
		X509_time_adj(notBefore, l_2016, &epoch);
		X509_gmtime_adj(notAfter, LEAF_VALID_FOR + 30);
	})));
}

BOOST_AUTO_TEST_SUITE_END()
