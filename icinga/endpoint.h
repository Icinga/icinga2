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

#ifndef ENDPOINT_H
#define ENDPOINT_H

namespace icinga
{

class EndpointManager;

/**
 * An endpoint that can be used to send and receive messages.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Endpoint : public Object
{
public:
	typedef shared_ptr<Endpoint> Ptr;
	typedef weak_ptr<Endpoint> WeakPtr;

	typedef set<String>::const_iterator ConstTopicIterator;

	Endpoint(void)
		: m_ReceivedWelcome(false), m_SentWelcome(false)
	{ }

	virtual String GetIdentity(void) const = 0;
	virtual String GetAddress(void) const = 0;

	void SetReceivedWelcome(bool value);
	bool HasReceivedWelcome(void) const;

	void SetSentWelcome(bool value);
	bool HasSentWelcome(void) const;

	shared_ptr<EndpointManager> GetEndpointManager(void) const;
	void SetEndpointManager(weak_ptr<EndpointManager> manager);

	void RegisterSubscription(String topic);
	void UnregisterSubscription(String topic);
	bool HasSubscription(String topic) const;

	void RegisterPublication(String topic);
	void UnregisterPublication(String topic);
	bool HasPublication(String topic) const;

	virtual bool IsLocal(void) const = 0;
	virtual bool IsConnected(void) const = 0;

	virtual void ProcessRequest(Endpoint::Ptr sender, const RequestMessage& message) = 0;
	virtual void ProcessResponse(Endpoint::Ptr sender, const ResponseMessage& message) = 0;

	virtual void Stop(void) = 0;

	void ClearSubscriptions(void);
	void ClearPublications(void);

	ConstTopicIterator BeginSubscriptions(void) const;
	ConstTopicIterator EndSubscriptions(void) const;

	ConstTopicIterator BeginPublications(void) const;
	ConstTopicIterator EndPublications(void) const;

	boost::signal<void (const Endpoint::Ptr&)> OnSessionEstablished;

private:
	set<String> m_Subscriptions; /**< The topics this endpoint is
					  subscribed to. */
	set<String> m_Publications; /**< The topics this endpoint is
				         publishing. */
	bool m_ReceivedWelcome; /**< Have we received a welcome message
				     from this endpoint? */
	bool m_SentWelcome; /**< Have we sent a welcome message to this
			         endpoint? */

	weak_ptr<EndpointManager> m_EndpointManager; /**< The endpoint manager
						          this endpoint is
							  registered with. */
};

}

#endif /* ENDPOINT_H */
