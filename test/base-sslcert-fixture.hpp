/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef SSLCERT_FIXTURE_H
#define SSLCERT_FIXTURE_H

#include "remote/apilistener.hpp"
#include "remote/pkiutility.hpp"
#include "test/base-configuration-fixture.hpp"
#include <BoostTestTargetConfig.h>

namespace icinga{

struct SslCertificateFixture: ConfigurationDataDirFixture
{
	SslCertificateFixture()
	{
		caDir = ApiListener::GetCaDir();
		Utility::MkDir(caDir, 0700);
		certsDir = ApiListener::GetCertsDir();
		Utility::MkDir(certsDir, 0700);

		PkiUtility::NewCa();
		if (!Utility::PathExists(certsDir+"ca.crt")) {
			Utility::CopyFile(caDir + "ca.crt", certsDir + "ca.crt");
		}
	}

	~SslCertificateFixture() {}

	void EnsureCertFor(const String& name)
	{
		auto certKeyFileName = certsDir + name + ".crt";

		if (!Utility::PathExists(certKeyFileName)) {
			PkiUtility::NewCert(name, certsDir + name + ".key", certsDir + name + ".csr",
				certsDir + name + ".crt");
			PkiUtility::SignCsr(certsDir + name + ".csr", certsDir + name + ".crt");
		}
	}

	String caDir;
	String certsDir;
};

} // namespace icinga

#endif // SSLCERT_FIXTURE_H
