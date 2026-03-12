// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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

		m_CaDir = *ApiListener::GetCaDir();
		m_CertsDir = *ApiListener::GetCertsDir();
		m_CaCrtFile = m_CertsDir / "ca.crt";

		Utility::MkDirP((m_PersistentCertsDir / "ca").string(), 0700);
		Utility::MkDirP((m_PersistentCertsDir / "certs").string(), 0700);

		if (Utility::PathExists(m_CaDir.string())) {
			Utility::RemoveDirRecursive(m_CaDir.string());
		}
		if (Utility::PathExists(m_CertsDir.string())) {
			Utility::RemoveDirRecursive(m_CertsDir.string());
		}

		Utility::MkDirP(m_CaDir.string(), 0700);
		for(const auto& entry : fs::directory_iterator{m_PersistentCertsDir / "ca"}){
			Utility::CopyFile(entry.path().string(), (m_CaDir / entry.path().filename()).string());
		}

		Utility::MkDirP(m_CertsDir.string(), 0700);
		for(const auto& entry : fs::directory_iterator{m_PersistentCertsDir / "certs"}){
			Utility::CopyFile(entry.path().string(), (m_CertsDir / entry.path().filename()).string());
		}

		if (!Utility::PathExists(m_CaCrtFile.string())) {
			PkiUtility::NewCa();
			Utility::CopyFile((m_CaDir / "ca.crt").string(), m_CaCrtFile.string());
		}
	}

	~CertificateFixture()
	{
		namespace fs = boost::filesystem;

		for(const auto& entry : fs::directory_iterator{m_CaDir}){
			Utility::CopyFile(entry.path().string(), (m_PersistentCertsDir / "ca" / entry.path().filename()).string());
		}

		for(const auto& entry : fs::directory_iterator{m_CertsDir}){
			Utility::CopyFile(entry.path().string(), (m_PersistentCertsDir / "certs" / entry.path().filename()).string());
		}
	}

	[[nodiscard]] auto EnsureCertFor(const std::string& name, bool overrideExisting = false) const
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

		if (overrideExisting || !Utility::PathExists(cert.crtFile)) {
			PkiUtility::NewCert(name, cert.keyFile, cert.csrFile, "");
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
