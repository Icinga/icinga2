/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef CONFIGCOMPILERCONTEXT_H
#define CONFIGCOMPILERCONTEXT_H

#include "config/i2-config.hpp"
#include "base/dictionary.hpp"
#include <boost/thread/mutex.hpp>
#include <fstream>

namespace icinga
{

/*
 * @ingroup config
 */
class ConfigCompilerContext
{
public:
	ConfigCompilerContext();

	void OpenObjectsFile(const String& filename);
	void WriteObject(const Dictionary::Ptr& object);
	void CancelObjectsFile();
	void FinishObjectsFile();

	static ConfigCompilerContext *GetInstance();

private:
	String m_ObjectsPath;
	String m_ObjectsTempFile;
	std::fstream *m_ObjectsFP;

	mutable boost::mutex m_Mutex;
};

}

#endif /* CONFIGCOMPILERCONTEXT_H */
