#ifndef I2_NETSTRING_H
#define I2_NETSTRING_H

namespace icinga
{

class Netstring : public Object
{
private:
	size_t m_Length;
	void *m_Data;

public:
	typedef shared_ptr<Netstring> RefType;
	typedef weak_ptr<Netstring> WeakRefType;

	static cJSON *ReadJSONFromFIFO(FIFO::RefType fifo);
	static void WriteJSONToFIFO(FIFO::RefType fifo, cJSON *object);
};

}

#endif /* I2_NETSTRING_H */
