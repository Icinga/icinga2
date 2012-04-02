#include "i2-configfilecomponent.h"
#include <cJSON.h>
#include <iostream>
#include <fstream>

using namespace icinga;
using std::ifstream;

string ConfigFileComponent::GetName(void)
{
	return "configfilecomponent";
}

void ConfigFileComponent::Start(void)
{
	ifstream fp;
	FIFO::RefType fifo = new_object<FIFO>();

	string filename;
	if (!GetConfig()->GetProperty("configFilename", &filename))
		throw exception(/*"Missing configFilename property"*/);

	fp.open(filename.c_str(), ifstream::in);
	if (fp.fail())
		throw exception(/*"Could not open config file"*/);
	
	while (!fp.eof()) {
		size_t bufferSize = 1024;
		char *buffer = (char *)fifo->GetWriteBuffer(&bufferSize);
		fp.read(buffer, bufferSize);
		if (fp.bad())
			throw exception(/*"Could not read from config file"*/);
		fifo->Write(NULL, fp.gcount());
	}

	fp.close();

	fifo->Write("\0", 1);

	/* TODO: implement config parsing, for now we just use JSON */
	cJSON *jsonobj = cJSON_Parse((const char *)fifo->GetReadBuffer());
	fifo->Read(NULL, fifo->GetSize());

	if (jsonobj == NULL)
		throw exception(/*"Could not parse config file."*/);

	for (cJSON *typeobj = jsonobj->child; typeobj != NULL; typeobj = typeobj->next) {
		string type = typeobj->string;

		for (cJSON *object = typeobj->child; object != NULL; object = object->next) {
			string name = object->string;

			ConfigObject::RefType cfgobj = new_object<ConfigObject>();
			cfgobj->SetName(name);
			cfgobj->SetType(type);

			for (cJSON *property = object->child; property != NULL; property = property->next) {
				string key = property->string;

				if (property->type != cJSON_String)
					continue;

				string value = property->valuestring;

				cfgobj->SetProperty(key, value);
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
