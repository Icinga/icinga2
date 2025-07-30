/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include <BoostTestTargetConfig.h>
#include "remote/apilistener.hpp"
#include "remote/pkiutility.hpp"
#include "test/base-configuration-fixture.hpp"

namespace icinga {

struct CertificateFixture : ConfigurationDataDirFixture
{
	CertificateFixture()
	{
		namespace fs = boost::filesystem;

		m_CaDir = ApiListener::GetCaDir();
		m_CertsDir = ApiListener::GetCertsDir();
		m_CaCrtFile = m_CertsDir / "ca.crt";

		fs::create_directories(m_PersistentCertsDir / "ca");
		fs::create_directories(m_PersistentCertsDir / "certs");

		if (fs::exists(m_DataDir / "ca")) {
			fs::remove(m_DataDir / "ca");
		}
		if (fs::exists(m_DataDir / "certs")) {
			fs::remove(m_DataDir / "certs");
		}

		fs::create_directory_symlink(m_PersistentCertsDir / "certs", m_DataDir / "certs");
		fs::create_directory_symlink(m_PersistentCertsDir / "ca", m_DataDir / "ca");

		if (!fs::exists(m_CaCrtFile)) {
			PkiUtility::NewCa();
			fs::copy_file(m_CaDir / "ca.crt", m_CaCrtFile);
		}
	}

	auto EnsureCertFor(const std::string& name)
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

		if (!Utility::PathExists(cert.crtFile)) {
			PkiUtility::NewCert(name, cert.keyFile, cert.csrFile, cert.crtFile);
			PkiUtility::SignCsr(cert.csrFile, cert.crtFile);
		}

		return cert;
	}

	boost::filesystem::path m_CaDir;
	boost::filesystem::path m_CertsDir;
	boost::filesystem::path m_CaCrtFile;
	static const boost::filesystem::path m_PersistentCertsDir;
};

} // namespace icinga
