#ifndef NETSTRING_H
#define NETSTRING_H

struct cJSON;

namespace icinga
{

typedef ::cJSON json_t;

class I2_JSONRPC_API Netstring : public Object
{
private:
	size_t m_Length;
	void *m_Data;

	static Dictionary::Ptr GetDictionaryFromJson(json_t *json);
	static json_t *GetJsonFromDictionary(const Dictionary::Ptr& dictionary);

public:
	typedef shared_ptr<Netstring> Ptr;
	typedef weak_ptr<Netstring> WeakPtr;

	static bool ReadMessageFromFIFO(FIFO::Ptr fifo, Message *message);
	static void WriteMessageToFIFO(FIFO::Ptr fifo, const Message& message);
};

}

#endif /* NETSTRING_H */
