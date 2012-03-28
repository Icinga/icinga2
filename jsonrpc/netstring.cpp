#include <cstdio>
#include "i2-jsonrpc.h"

using namespace icinga;
using std::sprintf;

Netstring::Netstring(void)
{
	m_Length = 0;
	m_Data = NULL;
}

Netstring::~Netstring(void)
{
	Memory::Free(m_Data);
}

/* based on https://github.com/PeterScott/netstring-c/blob/master/netstring.c */
Netstring::RefType Netstring::ReadFromFIFO(FIFO::RefType fifo)
{
	size_t buffer_length = fifo->GetSize();
	const char *buffer = (const char *)fifo->Peek();

	/* minimum netstring length is 3 */
	if (buffer_length < 3)
		return Netstring::RefType();

	/* no leading zeros allowed */
	if (buffer[0] == '0' && isdigit(buffer[1]))
		throw exception(/*"Invalid netstring (leading zero)"*/);

	size_t len, i;

	len = 0;
	for (i = 0; i < buffer_length && isdigit(buffer[i]); i++) {
		/* length specifier must have at most 9 characters */
		if (i >= 9)
			return Netstring::RefType();

		len = len * 10 + (buffer[i] - '0');
	}

	/* make sure the buffer is large enough */
	if (i + len + 1 >= buffer_length)
		return Netstring::RefType();

	/* check for the colon delimiter */
	if (buffer[i++] != ':')
		throw exception(/*"Invalid Netstring (missing :)"*/);

	/* check for the comma delimiter after the string */
	if (buffer[i + len] != ',')
		throw exception(/*"Invalid Netstring (missing ,)"*/);

	Netstring::RefType ns = new_object<Netstring>();
	ns->m_Length = len;
	ns->m_Data = Memory::Allocate(len + 1);
	memcpy(ns->m_Data, &buffer[i], len);
	((char *)ns->m_Data)[len] = '\0';

	/* remove the data from the fifo */
	fifo->Read(NULL, i + len + 1);

	return ns;
}

bool Netstring::WriteToFIFO(FIFO::RefType fifo) const
{
	char strLength[50];
	sprintf(strLength, "%lu", (unsigned long)m_Length);

	fifo->Write(strLength, strlen(strLength));
	fifo->Write(m_Data, m_Length);
	fifo->Write(",", 1);

	return true;
}

size_t Netstring::GetSize(void) const {
	return m_Length;
}

const void *Netstring::GetData(void) const
{
	return m_Data;
}

const char *Netstring::ToString(void)
{
	/* our implementation already guarantees that there's a NUL char at
	   the end of the string - so we can just return m_Data here */
	return (const char *)m_Data;
}
