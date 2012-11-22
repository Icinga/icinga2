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

#ifndef COMPATIDOCOMPONENT_H
#define COMPATIDOCOMPONENT_H

namespace icinga
{

/**
 * @ingroup compatido
 */
class CompatIdoComponent : public IComponent
{
public:
	virtual void Start(void);
	virtual void Stop(void);

private:
	Timer::Ptr m_StatusTimer;
	Timer::Ptr m_ConfigTimer;
	Timer::Ptr m_ProgramStatusTimer;
	Timer::Ptr m_ReconnectTimer;

	IdoConnection::Ptr m_IdoConnection;

	String GetSocketAddress(void) const;
	String GetSocketPort(void) const;
	String GetInstanceName(void) const;
	double GetReconnectInterval(void) const;

	void SocketDisconnectHandler(void);

	void ConfigTimerHandler(void);
	void StatusTimerHandler(void);
	void ProgramStatusTimerHandler(void);
	void ReconnectTimerHandler(void);

	void OpenIdoSocket(void);
	void CloseIdoSocket(void);

	void SendStartProcess(void);

	void EnableHostObject(const Host::Ptr& host);
	void EnableServiceObject(const Service::Ptr& service);
	void DisableHostObject(const Host::Ptr& host);
	void DisableServiceObject(const Service::Ptr& service);

	void DumpConfigObjects(void);
	void DumpHostObject(const Host::Ptr& host);
	void DumpServiceObject(const Service::Ptr& service);
	void DumpStatusData(void);
	void DumpHostStatus(const Host::Ptr& host);
	void DumpServiceStatus(const Service::Ptr& service);
	void DumpProgramStatusData(void);

	template<typename T>
	void SendMessageList(stringstream& msg, const T& list, int type)
	{
		typename T::const_iterator it;
		for (it = list.begin(); it != list.end(); it++) {
			msg << type << "=" << *it << "\n";
		}
	}

	static const String DefaultSocketAddress;
	static const String DefaultSocketPort;
	static const String DefaultInstanceName;
	static const int DefaultReconnectInterval;

};

}

#endif /* COMPATIDOCOMPONENT_H */
