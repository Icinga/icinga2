// SPDX-FileCopyrightText: 2020 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cli/pkiverifycommand.hpp"
#include "icinga/service.hpp"
#include "remote/pkiutility.hpp"
#include "base/tlsutility.hpp"
#include "base/logger.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/verify", PKIVerifyCommand);

String PKIVerifyCommand::GetDescription() const
{
	return "Verify TLS certificates: CN, signed by CA, is CA; Print certificate";
}

String PKIVerifyCommand::GetShortDescription() const
{
	return "verify TLS certificates: CN, signed by CA, is CA; Print certificate";
}

void PKIVerifyCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	[[maybe_unused]] boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("cn", po::value<std::string>(), "Common Name (optional). Use with '--cert' to check the CN in the certificate.")
		("cert", po::value<std::string>(), "Certificate file path (optional). Standalone: print certificate. With '--cacert': Verify against CA.")
		("cacert", po::value<std::string>(), "CA certificate file path (optional). If passed standalone, verifies whether this is a CA certificate")
		("crl", po::value<std::string>(), "CRL file path (optional). Check the certificate against this revocation list when verifying against CA.");
}

std::vector<String> PKIVerifyCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "cert" || argument == "cacert" || argument == "crl")
		return GetBashCompletionSuggestions("file", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

/**
 * The entry point for the "pki verify" CLI command.
 *
 * @returns An exit status.
 */
int PKIVerifyCommand::Run(const boost::program_options::variables_map& vm, [[maybe_unused]] const std::vector<std::string>& ap) const
{
	String cn, certFile, caCertFile, crlFile;

	if (vm.count("cn"))
		cn = vm["cn"].as<std::string>();

	if (vm.count("cert"))
		certFile = vm["cert"].as<std::string>();

	if (vm.count("cacert"))
		caCertFile = vm["cacert"].as<std::string>();

	if (vm.count("crl"))
		crlFile = vm["crl"].as<std::string>();

	/* Verify CN in certificate. */
	if (!cn.IsEmpty() && !certFile.IsEmpty()) {
		std::shared_ptr<X509> cert;
		try {
			cert = GetX509Certificate(certFile);
		} catch (const std::exception& ex) {
			Log(LogCritical, "cli")
				<< "Cannot read certificate file '" << certFile << "'. Please ensure that it exists and is readable.";

			return ServiceCritical;
		}

		Log(LogInformation, "cli")
			<< "Verifying common name (CN) '" << cn << " in certificate '" << certFile << "'.";

		std::cout << PkiUtility::GetCertificateInformation(cert) << "\n";

		String certCN = GetCertificateCN(cert);

		if (cn == certCN) {
			Log(LogInformation, "cli")
				<< "OK: CN '" << cn << "' matches certificate CN '" << certCN << "'.";

			return ServiceOK;
		} else {
			Log(LogCritical, "cli")
				<< "CRITICAL: CN '" << cn << "' does NOT match certificate CN '" << certCN << "'.";

			return ServiceCritical;
		}
	}

	/* Verify certificate. */
	if (!certFile.IsEmpty() && !caCertFile.IsEmpty()) {
		std::shared_ptr<X509> cert;
		try {
			cert = GetX509Certificate(certFile);
		} catch (const std::exception& ex) {
			Log(LogCritical, "cli")
				<< "Cannot read certificate file '" << certFile << "'. Please ensure that it exists and is readable.";

			return ServiceCritical;
		}

		std::shared_ptr<X509> cacert;
		try {
			cacert = GetX509Certificate(caCertFile);
		} catch (const std::exception& ex) {
			Log(LogCritical, "cli")
				<< "Cannot read CA certificate file '" << caCertFile << "'. Please ensure that it exists and is readable.";

			return ServiceCritical;
		}

		Log(LogInformation, "cli")
			<< "Verifying certificate '" << certFile << "'";

		std::cout << PkiUtility::GetCertificateInformation(cert) << "\n";

		Log(LogInformation, "cli")
			<< " with CA certificate '" << caCertFile << "'.";

		std::cout << PkiUtility::GetCertificateInformation(cacert) << "\n";

		String certCN = GetCertificateCN(cert);

		bool signedByCA;

		try {
			signedByCA = VerifyCertificate(cacert, cert, crlFile);
		} catch (const std::exception& ex) {
			Log logmsg (LogCritical, "cli");
			logmsg << "CRITICAL: Certificate with CN '" << certCN << "' is NOT signed by CA: ";
			if (const unsigned long *openssl_code = boost::get_error_info<errinfo_openssl_error>(ex)) {
				logmsg << X509_verify_cert_error_string(*openssl_code) << " (code " << *openssl_code << ")";
			} else {
				logmsg << DiagnosticInformation(ex, false);
			}

			return ServiceCritical;
		}

		if (signedByCA) {
			Log(LogInformation, "cli")
				<< "OK: Certificate with CN '" << certCN << "' is signed by CA.";

			return ServiceOK;
		} else {
			Log(LogCritical, "cli")
				<< "CRITICAL: Certificate with CN '" << certCN << "' is NOT signed by CA.";

			return ServiceCritical;
		}
	}


	/* Standalone CA checks. */
	if (certFile.IsEmpty() && !caCertFile.IsEmpty()) {
		std::shared_ptr<X509> cacert;
		try {
			cacert = GetX509Certificate(caCertFile);
		} catch (const std::exception& ex) {
			Log(LogCritical, "cli")
				<< "Cannot read CA certificate file '" << caCertFile << "'. Please ensure that it exists and is readable.";

			return ServiceCritical;
		}

		Log(LogInformation, "cli")
			<< "Checking whether certificate '" << caCertFile << "' is a valid CA certificate.";

		std::cout << PkiUtility::GetCertificateInformation(cacert) << "\n";

		if (IsCa(cacert)) {
			Log(LogInformation, "cli")
				<< "OK: CA certificate file '" << caCertFile << "' was verified successfully.\n";

			return ServiceOK;
		} else {
			Log(LogCritical, "cli")
				<< "CRITICAL: The file '" << caCertFile << "' does not seem to be a CA certificate file.\n";

			return ServiceCritical;
		}
	}

	/* Print certificate */
	if (!certFile.IsEmpty()) {
		std::shared_ptr<X509> cert;
		try {
			cert = GetX509Certificate(certFile);
		} catch (const std::exception& ex) {
			Log(LogCritical, "cli")
				<< "Cannot read certificate file '" << certFile << "'. Please ensure that it exists and is readable.";

			return ServiceCritical;
		}

		Log(LogInformation, "cli")
			<< "Printing certificate '" << certFile << "'";

		std::cout << PkiUtility::GetCertificateInformation(cert) << "\n";

		return ServiceOK;
	}

	/* Error handling. */
	if (!cn.IsEmpty() && certFile.IsEmpty()) {
		Log(LogCritical, "cli")
			<< "The '--cn' parameter requires the '--cert' parameter.";

		return ServiceCritical;
	}

	if (cn.IsEmpty() && certFile.IsEmpty() && caCertFile.IsEmpty()) {
		Log(LogInformation, "cli")
			<< "Please add the '--help' parameter to see all available options.";

		return ServiceOK;
	}

	return ServiceOK;
}
