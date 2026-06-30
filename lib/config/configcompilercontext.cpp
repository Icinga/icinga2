// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config/configcompilercontext.hpp"
#include "base/singleton.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"

using namespace icinga;

ConfigCompilerContext *ConfigCompilerContext::GetInstance()
{
	return Singleton<ConfigCompilerContext>::GetInstance();
}

void ConfigCompilerContext::OpenObjectsFile(const String& filename)
{
	try {
		m_ObjectsFP = std::make_unique<AtomicFile>(filename, 0600);
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli", "Could not create temporary objects file: " + DiagnosticInformation(ex, false));
		Application::Exit(1);
	}
}

void ConfigCompilerContext::WriteObject(const Dictionary::Ptr& object)
{
	if (!m_ObjectsFP)
		return;

	String json = JsonEncode(object);

	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		NetString::WriteStringToStream(*m_ObjectsFP, json);
	}
}

void ConfigCompilerContext::CancelObjectsFile()
{
	if (!m_ObjectsFP)
		return;

	m_ObjectsFP.reset(nullptr);
}

void ConfigCompilerContext::FinishObjectsFile()
{
	if (!m_ObjectsFP)
		return;

	m_ObjectsFP->Commit();
	m_ObjectsFP.reset(nullptr);
}
