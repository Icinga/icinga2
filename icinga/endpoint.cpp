#include "i2-icinga.h"

using namespace icinga;

EndpointManager::Ptr Endpoint::GetEndpointManager(void) const
{
	return m_EndpointManager;
}

void Endpoint::SetEndpointManager(EndpointManager::Ptr manager)
{
	m_EndpointManager = manager;
}

void Endpoint::RegisterMethodSink(string method)
{
	m_MethodSinks.insert(method);
}

void Endpoint::UnregisterMethodSink(string method)
{
	m_MethodSinks.erase(method);
}

bool Endpoint::IsMethodSink(string method) const
{
	return (m_MethodSinks.find(method) != m_MethodSinks.end());
}

void Endpoint::ForeachMethodSink(function<int (const NewMethodEventArgs&)> callback)
{
	for (set<string>::iterator i = m_MethodSinks.begin(); i != m_MethodSinks.end(); i++) {
		NewMethodEventArgs nmea;
		nmea.Source = shared_from_this();
		nmea.Method = *i;
		callback(nmea);
	}
}

void Endpoint::RegisterMethodSource(string method)
{
	m_MethodSources.insert(method);
}

void Endpoint::UnregisterMethodSource(string method)
{
	m_MethodSources.erase(method);
}

bool Endpoint::IsMethodSource(string method) const
{
	return (m_MethodSources.find(method) != m_MethodSinks.end());
}

void Endpoint::ForeachMethodSource(function<int (const NewMethodEventArgs&)> callback)
{
	for (set<string>::iterator i = m_MethodSources.begin(); i != m_MethodSources.end(); i++) {
		NewMethodEventArgs nmea;
		nmea.Source = shared_from_this();
		nmea.Method = *i;
		callback(nmea);
	}
}
