/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef PKIUTILITY_H
#define PKIUTILITY_H

#include "base/i2-base.hpp"
#include "cli/i2-cli.hpp"
#include "base/dictionary.hpp"
#include "base/string.hpp"

namespace icinga
{

/**
 * @ingroup cli
 */
class I2_CLI_API PkiUtility
{
public:
	static String GetPkiPath(void);
	static String GetLocalCaPath(void);

	static int NewCa(void);
	static int NewCert(const String& cn, const String& keyfile, const String& csrfile, const String& certfile);
	static int SignCsr(const String& csrfile, const String& certfile);
	static int SaveCert(const String& host, const String& port, const String& keyfile, const String& certfile, const String& trustedfile);
	static int GenTicket(const String& cn, const String& salt, std::ostream& ticketfp);
	static int RequestCertificate(const String& host, const String& port, const String& keyfile,
	    const String& certfile, const String& cafile, const String& trustedfile, const String& ticket);

private:
	PkiUtility(void);


};

}

#endif /* PKIUTILITY_H */
