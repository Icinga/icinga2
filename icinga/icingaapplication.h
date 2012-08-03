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

	IcingaApplication(void);

	int Main(const vector<String>& args);

	static IcingaApplication::Ptr GetInstance(void);

	String GetCertificateFile(void) const;
	String GetCAFile(void) const;
	String GetNode(void) const;
	String GetService(void) const;
	String GetPidPath(void) const;
	Dictionary::Ptr GetMacros(void) const;

	double GetStartTime(void) const;

	static const String DefaultPidPath;

private:
	String m_CertificateFile;
	String m_CAFile;
	String m_Node;
	String m_Service;
	String m_PidPath;
	Dictionary::Ptr m_Macros;

	double m_StartTime;

	Timer::Ptr m_RetentionTimer;

	void DumpProgramState(void);
};

}

#endif /* ICINGAAPPLICATION_H */
