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

	int Main(const vector<string>& args);

	static IcingaApplication::Ptr GetInstance(void);

	string GetCertificateFile(void) const;
	string GetCAFile(void) const;
	string GetNode(void) const;
	string GetService(void) const;
	string GetPidPath(void) const;

	time_t GetStartTime(void) const;

	static const string DefaultPidPath;

private:
	string m_CertificateFile;
	string m_CAFile;
	string m_Node;
	string m_Service;
	string m_PidPath;

	time_t m_StartTime;

	void NewComponentHandler(const ConfigObject::Ptr& object);
	void DeletedComponentHandler(const ConfigObject::Ptr& object);
};

}

#endif /* ICINGAAPPLICATION_H */
