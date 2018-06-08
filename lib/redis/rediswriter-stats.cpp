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

#include "redis/rediswriter.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/statsfunction.hpp"
#include "base/convert.hpp"

using namespace icinga;

Dictionary::Ptr RedisWriter::GetStats()
{
	Dictionary::Ptr stats = new Dictionary();

	//TODO: Figure out if more stats can be useful here.
	Dictionary::Ptr statsFunctions = ScriptGlobal::Get("StatsFunctions", &Empty);

	if (!statsFunctions)
		Dictionary::Ptr();

	ObjectLock olock(statsFunctions);

	for (const Dictionary::Pair& kv : statsFunctions)
	{
		Function::Ptr func = kv.second;

		if (!func)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid status function name."));

		Dictionary::Ptr status = new Dictionary();
		Array::Ptr perfdata = new Array();
		func->Invoke({ status, perfdata });

		stats->Set(kv.first, new Dictionary({
			{ "status", status },
			{ "perfdata", Serialize(perfdata, FAState) }
		}));
	}

	return stats;
}

