/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "base/tlsutility.hpp"
#include <BoostTestTargetConfig.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

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

BOOST_AUTO_TEST_SUITE(base_tlsutility)

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
	BOOST_CHECK(VerifyCertificate(signingCaCert, leafCert, ""));

	// Create a second CA with a different key, the leaf certificate is supposed to fail validation against that CA.
	auto otherCaKey = GenKeypair();
	auto otherCaCert = CreateCert(otherCaKey, caSubject, caSubject, otherCaKey, true);
	BOOST_CHECK_THROW(VerifyCertificate(otherCaCert, leafCert, ""), openssl_error);
}

BOOST_AUTO_TEST_SUITE_END()
