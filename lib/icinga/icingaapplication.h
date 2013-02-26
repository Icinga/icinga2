/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef ICINGAAPPLICATION_H
#define ICINGAAPPLICATION_H

namespace icinga
{

/**
 * The Icinga application.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API IcingaApplication : public Application
{
public:
	typedef shared_ptr<IcingaApplication> Ptr;
	typedef weak_ptr<IcingaApplication> WeakPtr;

	IcingaApplication(const Dictionary::Ptr& serializedUpdate);

	int Main(void);

	static IcingaApplication::Ptr GetInstance(void);

	String GetCertificateFile(void) const;
	String GetCAFile(void) const;
	String GetNode(void) const;
	String GetService(void) const;
	String GetPidPath(void) const;
	String GetStatePath(void) const;
	Dictionary::Ptr GetMacros(void) const;
	shared_ptr<SSL_CTX> GetSSLContext(void) const;

	double GetStartTime(void) const;

	static Dictionary::Ptr CalculateDynamicMacros(const IcingaApplication::Ptr& self);

private:
	Attribute<String> m_CertPath;
	Attribute<String> m_CAPath;
	Attribute<String> m_Node;
	Attribute<String> m_Service;
	Attribute<String> m_PidPath;
	Attribute<String> m_StatePath;
	Attribute<Dictionary::Ptr> m_Macros;

	shared_ptr<SSL_CTX> m_SSLContext;

	double m_StartTime;

	Timer::Ptr m_RetentionTimer;

	void DumpProgramState(void);

	virtual void OnShutdown(void);
};

}

#endif /* ICINGAAPPLICATION_H */
