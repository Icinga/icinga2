#ifndef NETSTRING_H
#define NETSTRING_H

namespace icinga
{

class I2_JSONRPC_API Netstring : public Object
{
private:
	size_t m_Length;
	void *m_Data;

public:
	typedef shared_ptr<Netstring> Ptr;
	typedef weak_ptr<Netstring> WeakPtr;

	static cJSON *ReadJSONFromFIFO(FIFO::Ptr fifo);
	static void WriteJSONToFIFO(FIFO::Ptr fifo, cJSON *object);
};

}

#endif /* NETSTRING_H */
