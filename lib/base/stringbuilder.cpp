/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "base/stringbuilder.hpp"
#include <cstring>

using namespace icinga;

void StringBuilder::Append(const String& str)
{
	m_Buffer.insert(m_Buffer.end(), str.Begin(), str.End());
}

void StringBuilder::Append(const std::string& str)
{
	m_Buffer.insert(m_Buffer.end(), str.begin(), str.end());
}

void StringBuilder::Append(const char *begin, const char *end)
{
	m_Buffer.insert(m_Buffer.end(), begin, end);
}

void StringBuilder::Append(const char *cstr)
{
	m_Buffer.insert(m_Buffer.end(), cstr, cstr + std::strlen(cstr));
}

void StringBuilder::Append(char chr)
{
	m_Buffer.emplace_back(chr);
}

String StringBuilder::ToString() const
{
	return String(m_Buffer.data(), m_Buffer.data() + m_Buffer.size());
}
