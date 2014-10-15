/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "cli/pkisigncsrcommand.hpp"
#include "base/logger_fwd.hpp"
#include "base/clicommand.hpp"
#include "base/tlsutility.hpp"
#include "base/application.hpp"

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/sign-csr", PKISignCSRCommand);

String PKISignCSRCommand::GetDescription(void) const
{
	return "Reads a Certificate Signing Request from stdin and prints a signed certificate on stdout.";
}

String PKISignCSRCommand::GetShortDescription(void) const
{
	return "signs a CSR";
}

void PKISignCSRCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc,
    ArgumentCompletionDescription& argCompletionDesc) const
{
	/* Command doesn't support any parameters. */
}

/**
 * The entry point for the "pki sign-csr" CLI command.
 *
 * @returns An exit status.
 */
int PKISignCSRCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	BIO *csrbio = BIO_new_fp(stdin, BIO_NOCLOSE);
	X509_REQ *req;
	PEM_read_bio_X509_REQ(csrbio, &req, NULL, NULL);
	BIO_free(csrbio);

	String cadir = Application::GetLocalStateDir() + "/lib/icinga2/ca";

	String cakeyfile = cadir + "/ca.key";

	RSA *rsa;

	BIO *cakeybio = BIO_new_file(const_cast<char *>(cakeyfile.CStr()), "r");
	rsa = PEM_read_bio_RSAPrivateKey(cakeybio, NULL, NULL, NULL);
	BIO_free(cakeybio);

	String cacertfile = cadir + "/ca.crt";

	BIO *cacertbio = BIO_new_file(const_cast<char *>(cacertfile.CStr()), "r");
	X509 *cacert = PEM_read_bio_X509(cacertbio, NULL, NULL, NULL);
	BIO_free(cacertbio);

	EVP_PKEY *privkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(privkey, rsa);

	EVP_PKEY *pubkey = X509_REQ_get_pubkey(req);

	X509 *cert = CreateCert(pubkey, X509_REQ_get_subject_name(req), X509_get_subject_name(cacert), privkey, false);

	X509_free(cacert);

	BIO *certbio = BIO_new_fp(stdout, BIO_NOCLOSE);
	PEM_write_bio_X509(certbio, cert);
	BIO_free(certbio);

	return 0;
}
