#include "i2-jsonrpc.h"

using namespace icinga;

Message::Message(void)
{
	m_Dictionary = make_shared<Dictionary>();
}

Message::Message(const Dictionary::Ptr& dictionary)
{
	m_Dictionary = dictionary;
}

Message::Message(const Message& message)
{
	m_Dictionary = message.GetDictionary();
}

Dictionary::Ptr Message::GetDictionary(void) const
{
	return m_Dictionary;
}
