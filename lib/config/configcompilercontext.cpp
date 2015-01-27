/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "config/configcompilercontext.hpp"
#include "base/singleton.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/exception.hpp"
#include <boost/foreach.hpp>
#include <fstream>

using namespace icinga;

ConfigCompilerContext *ConfigCompilerContext::GetInstance(void)
{
	return Singleton<ConfigCompilerContext>::GetInstance();
}

void ConfigCompilerContext::OpenObjectsFile(const String& filename)
{
	m_ObjectsPath = filename;

	String tempFilename = m_ObjectsPath + ".tmp";

	std::fstream *fp = new std::fstream();
	fp->open(tempFilename.CStr(), std::ios_base::out);

	if (!*fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + tempFilename + "' file"));

	m_ObjectsFP = new StdioStream(fp, true);
}

void ConfigCompilerContext::WriteObject(const Dictionary::Ptr& object)
{
	if (!m_ObjectsFP)
		return;

	String json = JsonEncode(object);

	{
		boost::mutex::scoped_lock lock(m_Mutex);
		NetString::WriteStringToStream(m_ObjectsFP, json);
	}
}

void ConfigCompilerContext::FinishObjectsFile(void)
{
	m_ObjectsFP->Close();
	m_ObjectsFP.reset();

	String tempFilename = m_ObjectsPath + ".tmp";

#ifdef _WIN32
	_unlink(m_ObjectsPath.CStr());
#endif /* _WIN32 */

	if (rename(tempFilename.CStr(), m_ObjectsPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempFilename));
	}
}

