#include "i2-base.h"

using namespace icinga;

void Component::SetApplication(Application::WeakRefType application)
{
	m_Application = application;
}

Application::RefType Component::GetApplication(void)
{
	return m_Application.lock();
}
