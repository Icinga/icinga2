/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef CLUSTERLISTENER_H
#define CLUSTERLISTENER_H

#include "base/dynamicobject.h"
#include "base/timer.h"
#include "base/array.h"
#include "base/tcpsocket.h"
#include "base/tlsstream.h"
#include "base/utility.h"
#include "base/tlsutility.h"
#include "base/stdiostream.h"
#include "base/workqueue.h"
#include "icinga/service.h"
#include "cluster/endpoint.h"

namespace icinga
{

/**
 * @ingroup cluster
 */
class ClusterListener : public DynamicObject
{
public:
	DECLARE_PTR_TYPEDEFS(ClusterListener);
	DECLARE_TYPENAME(ClusterListener);

	virtual void Start(void);
	virtual void Stop(void);

	String GetCertificateFile(void) const;
	String GetKeyFile(void) const;
	String GetCAFile(void) const;
	String GetBindHost(void) const;
	String GetBindPort(void) const;
	Array::Ptr GetPeers(void) const;
	String GetClusterDir(void) const;

	shared_ptr<SSL_CTX> GetSSLContext(void) const;
	String GetIdentity(void) const;

protected:
	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

private:
	String m_CertPath;
	String m_KeyPath;
	String m_CAPath;
	String m_BindHost;
	String m_BindPort;
	Array::Ptr m_Peers;

	shared_ptr<SSL_CTX> m_SSLContext;
	String m_Identity;

	WorkQueue m_RelayQueue;
	WorkQueue m_MessageQueue;
	WorkQueue m_LogQueue;

	Timer::Ptr m_ClusterTimer;
	void ClusterTimerHandler(void);

	std::set<TcpSocket::Ptr> m_Servers;

	void AddListener(const String& service);
	void AddConnection(const String& node, const String& service);

	static void ConfigGlobHandler(const Dictionary::Ptr& config, const String& file, bool basename);

	void NewClientHandler(const Socket::Ptr& client, TlsRole role);
	void ListenerThreadProc(const Socket::Ptr& server);

	void AsyncRelayMessage(const Endpoint::Ptr& source, const Dictionary::Ptr& message, bool persistent);
	void RelayMessage(const Endpoint::Ptr& source, const Dictionary::Ptr& message, bool persistent);

	void OpenLogFile(void);
	void RotateLogFile(void);
	void CloseLogFile(void);
	static void LogGlobHandler(std::vector<int>& files, const String& file);
	void ReplayLog(const Endpoint::Ptr& endpoint, const Stream::Ptr& stream);

	Stream::Ptr m_LogFile;
	double m_LogMessageTimestamp;
	size_t m_LogMessageCount;

	void CheckResultHandler(const Service::Ptr& service, const Dictionary::Ptr& cr, const String& authority);
	void NextCheckChangedHandler(const Service::Ptr& service, double nextCheck, const String& authority);
	void NextNotificationChangedHandler(const Notification::Ptr& notification, double nextCheck, const String& authority);
	void ForceNextCheckChangedHandler(const Service::Ptr& service, bool forced, const String& authority);
	void ForceNextNotificationChangedHandler(const Service::Ptr& service, bool forced, const String& authority);
	void EnableActiveChecksChangedHandler(const Service::Ptr& service, bool enabled, const String& authority);
	void EnablePassiveChecksChangedHandler(const Service::Ptr& service, bool enabled, const String& authority);
	void EnableNotificationsChangedHandler(const Service::Ptr& service, bool enabled, const String& authority);
	void EnableFlappingChangedHandler(const Service::Ptr& service, bool enabled, const String& authority);
	void CommentAddedHandler(const Service::Ptr& service, const Dictionary::Ptr& comment, const String& authority);
	void CommentRemovedHandler(const Service::Ptr& service, const Dictionary::Ptr& comment, const String& authority);
	void DowntimeAddedHandler(const Service::Ptr& service, const Dictionary::Ptr& downtime, const String& authority);
	void DowntimeRemovedHandler(const Service::Ptr& service, const Dictionary::Ptr& downtime, const String& authority);
	void AcknowledgementSetHandler(const Service::Ptr& service, const String& author, const String& comment, AcknowledgementType type, double expiry, const String& authority);
	void AcknowledgementClearedHandler(const Service::Ptr& service, const String& authority);

	void AsyncMessageHandler(const Endpoint::Ptr& sender, const Dictionary::Ptr& message);
	void MessageHandler(const Endpoint::Ptr& sender, const Dictionary::Ptr& message);

	bool IsAuthority(const DynamicObject::Ptr& object, const String& type);
	void UpdateAuthority(void);

	static bool SupportsChecks(void);
	static bool SupportsNotifications(void);

	void SetSecurityInfo(const Dictionary::Ptr& message, const DynamicObject::Ptr& object, int privs);

	void PersistMessage(const Endpoint::Ptr& source, const Dictionary::Ptr& message);
};

}

#endif /* CLUSTERLISTENER_H */
