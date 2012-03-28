#include <cstdio>
#include "i2-jsonrpc.h"

using namespace icinga;
using std::sprintf;

/* based on https://github.com/PeterScott/netstring-c/blob/master/netstring.c */
cJSON *Netstring::ReadJSONFromFIFO(FIFO::RefType fifo)
{
	size_t buffer_length = fifo->GetSize();
	char *buffer = (char *)fifo->GetReadBuffer();

	/* minimum netstring length is 3 */
	if (buffer_length < 3)
		return NULL;

	/* no leading zeros allowed */
	if (buffer[0] == '0' && isdigit(buffer[1]))
		throw exception(/*"Invalid netstring (leading zero)"*/);

	size_t len, i;

	len = 0;
	for (i = 0; i < buffer_length && isdigit(buffer[i]); i++) {
		/* length specifier must have at most 9 characters */
		if (i >= 9)
			return NULL;

		len = len * 10 + (buffer[i] - '0');
	}

	/* make sure the buffer is large enough */
	if (i + len + 1 >= buffer_length)
		return NULL;

	/* check for the colon delimiter */
	if (buffer[i++] != ':')
		throw exception(/*"Invalid Netstring (missing :)"*/);

	/* check for the comma delimiter after the string */
	if (buffer[i + len] != ',')
		throw exception(/*"Invalid Netstring (missing ,)"*/);

	/* nuke the comma delimiter */
	buffer[i + len] = '\0';
	cJSON *object = cJSON_Parse(&buffer[i]);

	if (object == NULL) {
		/* restore the comma */
		buffer[i + len] = ',';
		throw exception(/*"Invalid JSON string"*/);
	}

	/* remove the data from the fifo */
	fifo->Read(NULL, i + len + 1);

	return object;
}

void Netstring::WriteJSONToFIFO(FIFO::RefType fifo, cJSON *object)
{
	char *json = cJSON_Print(object);
	size_t len = strlen(json);
	char strLength[50];
	sprintf(strLength, "%lu", (unsigned long)len);

	fifo->Write(strLength, strlen(strLength));
	fifo->Write(json, len);
	fifo->Write(",", 1);
}
