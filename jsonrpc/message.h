#ifndef MESSAGE_H
#define MESSAGE_H

namespace icinga
{

class I2_JSONRPC_API Message
{
private:
	Dictionary::Ptr m_Dictionary;

public:
	Message(void);
	Message(const Dictionary::Ptr& dictionary);
	Message(const Message& message);

	Dictionary::Ptr GetDictionary(void) const;
};

}

#endif /* MESSAGE_H */
