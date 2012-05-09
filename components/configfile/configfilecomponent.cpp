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
