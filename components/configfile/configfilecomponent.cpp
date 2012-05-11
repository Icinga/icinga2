/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include <iostream>
#include <fstream>
#include "i2-configfile.h"
#include "cJSON.h"

using namespace icinga;

string ConfigFileComponent::GetName(void) const
{
	return "configfilecomponent";
}

void ConfigFileComponent::Start(void)
{
	ifstream fp;
	FIFO::Ptr fifo = make_shared<FIFO>();

	string filename;
	if (!GetConfig()->GetPropertyString("configFilename", &filename))
		throw InvalidArgumentException("Missing 'configFilename' property");

	fp.open(filename.c_str(), ifstream::in);
	if (fp.fail())
		throw ConfigParserException("Could not open config file");
	
	GetApplication()->Log("Reading config file: " + filename);

	while (!fp.eof()) {
		size_t bufferSize = 1024;
		char *buffer = (char *)fifo->GetWriteBuffer(&bufferSize);
		fp.read(buffer, bufferSize);
		if (fp.bad())
			throw ConfigParserException("Could not read from config file");
		fifo->Write(NULL, fp.gcount());
	}

	fp.close();

	fifo->Write("\0", 1);

	/* TODO: implement config parsing, for now we just use JSON */
	cJSON *jsonobj = cJSON_Parse((const char *)fifo->GetReadBuffer());
	fifo->Read(NULL, fifo->GetSize());

	if (jsonobj == NULL)
		throw ConfigParserException("Could not parse config file.");

	for (cJSON *typeobj = jsonobj->child; typeobj != NULL; typeobj = typeobj->next) {
		string type = typeobj->string;

		for (cJSON *object = typeobj->child; object != NULL; object = object->next) {
			string name = object->string;

			ConfigObject::Ptr cfgobj = make_shared<ConfigObject>(type, name);

			for (cJSON *property = object->child; property != NULL; property = property->next) {
				string key = property->string;
				
				if (property->type == cJSON_String) {
					string value = property->valuestring;

					cfgobj->SetPropertyString(key, value);
				} else if (property->type == cJSON_Array) {
					Dictionary::Ptr items = make_shared<Dictionary>();

					for (cJSON *item = property->child; item != NULL; item = item->next) {
						if (item->type != cJSON_String)
							continue;

						items->AddUnnamedPropertyString(item->valuestring);
					}

					cfgobj->SetPropertyDictionary(key, items);
				}
			}

			GetApplication()->GetConfigHive()->AddObject(cfgobj);
		}
	}

	cJSON_Delete(jsonobj);
}

void ConfigFileComponent::Stop(void)
{
}

EXPORT_COMPONENT(ConfigFileComponent);
