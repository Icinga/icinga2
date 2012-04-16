#include "i2-icinga.h"

using namespace icinga;

void Endpoint::RegisterMethodSink(string method)
{
	m_MethodSinks.insert(method);
}

void Endpoint::UnregisterMethodSink(string method)
{
	m_MethodSinks.erase(method);
}

bool Endpoint::IsMethodSink(string method)
{
	return (m_MethodSinks.find(method) != m_MethodSinks.end());
}

void Endpoint::RegisterMethodSource(string method)
{
	m_MethodSources.insert(method);
}

void Endpoint::UnregisterMethodSource(string method)
{
	m_MethodSources.erase(method);
}

bool Endpoint::IsMethodSource(string method)
{
	return (m_MethodSources.find(method) != m_MethodSinks.end());
}
