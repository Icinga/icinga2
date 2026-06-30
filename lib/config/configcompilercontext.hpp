// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONFIGCOMPILERCONTEXT_H
#define CONFIGCOMPILERCONTEXT_H

#include "config/i2-config.hpp"
#include "base/atomic-file.hpp"
#include "base/dictionary.hpp"
#include <fstream>
#include <memory>
#include <mutex>

namespace icinga
{

/*
 * @ingroup config
 */
class ConfigCompilerContext
{
public:
	void OpenObjectsFile(const String& filename);
	void WriteObject(const Dictionary::Ptr& object);
	void CancelObjectsFile();
	void FinishObjectsFile();

	inline bool IsOpen() const noexcept
	{
		return (bool)m_ObjectsFP;
	}

	static ConfigCompilerContext *GetInstance();

private:
	std::unique_ptr<AtomicFile> m_ObjectsFP;

	mutable std::mutex m_Mutex;
};

}

#endif /* CONFIGCOMPILERCONTEXT_H */
