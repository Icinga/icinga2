#ifndef NETSTRING_H
#define NETSTRING_H

namespace icinga
{

class I2_JSONRPC_API Netstring : public Object
{
private:
	size_t m_Length;
	void *m_Data;

	static Dictionary::Ptr GetDictionaryFromJson(cJSON *json);
	static cJSON *GetJsonFromDictionary(const Dictionary::Ptr& dictionary);

public:
	typedef shared_ptr<Netstring> Ptr;
	typedef weak_ptr<Netstring> WeakPtr;

	static bool ReadMessageFromFIFO(FIFO::Ptr fifo, Message *message);
	static void WriteMessageToFIFO(FIFO::Ptr fifo, const Message& message);
};

}

#endif /* NETSTRING_H */
