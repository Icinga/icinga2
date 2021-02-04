/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONFIGCOMPILERCONTEXT_H
#define CONFIGCOMPILERCONTEXT_H

#include "config/i2-config.hpp"
#include "base/dictionary.hpp"
#include <fstream>
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

	static ConfigCompilerContext *GetInstance();

private:
	String m_ObjectsPath;
	String m_ObjectsTempFile;
	std::fstream *m_ObjectsFP{nullptr};

	mutable std::mutex m_Mutex;
};

}

#endif /* CONFIGCOMPILERCONTEXT_H */
