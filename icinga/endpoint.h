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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#ifndef ENDPOINT_H
#define ENDPOINT_H

namespace icinga
{

class EndpointManager;

struct I2_ICINGA_API NewMethodEventArgs : public EventArgs
{
	string Method;
};

class I2_ICINGA_API Endpoint : public Object
{
private:
	string m_Identity;
	set<string> m_MethodSinks;
	set<string> m_MethodSources;
	bool m_ReceivedWelcome;
	bool m_SentWelcome;

	weak_ptr<EndpointManager> m_EndpointManager;

public:
	typedef shared_ptr<Endpoint> Ptr;
	typedef weak_ptr<Endpoint> WeakPtr;

	Endpoint(void);

	virtual string GetAddress(void) const = 0;

	string GetIdentity(void) const;
	void SetIdentity(string identity);
	bool HasIdentity(void) const;

	void SetReceivedWelcome(bool value);
	bool GetReceivedWelcome(void) const;

	void SetSentWelcome(bool value);
	bool GetSentWelcome(void) const;

	shared_ptr<EndpointManager> GetEndpointManager(void) const;
	void SetEndpointManager(weak_ptr<EndpointManager> manager);

	void RegisterMethodSink(string method);
	void UnregisterMethodSink(string method);
	bool IsMethodSink(string method) const;

	void RegisterMethodSource(string method);
	void UnregisterMethodSource(string method);
	bool IsMethodSource(string method) const;

	virtual bool IsLocal(void) const = 0;
	virtual bool IsConnected(void) const = 0;

	virtual void ProcessRequest(Endpoint::Ptr sender, const JsonRpcRequest& message) = 0;
	virtual void ProcessResponse(Endpoint::Ptr sender, const JsonRpcResponse& message) = 0;

	virtual void Stop(void) = 0;

	Event<NewMethodEventArgs> OnNewMethodSink;
	Event<NewMethodEventArgs> OnNewMethodSource;

	void ForEachMethodSink(function<int (const NewMethodEventArgs&)> callback);
	void ForEachMethodSource(function<int (const NewMethodEventArgs&)> callback);

	void ClearMethodSinks(void);
	void ClearMethodSources(void);

	int CountMethodSinks(void) const;
	int CountMethodSources(void) const;

	set<string>::const_iterator BeginSinks(void) const;
	set<string>::const_iterator EndSinks(void) const;

	set<string>::const_iterator BeginSources(void) const;
	set<string>::const_iterator EndSources(void) const;

	Event<EventArgs> OnIdentityChanged;
	Event<EventArgs> OnSessionEstablished;
};

}

#endif /* ENDPOINT_H */
