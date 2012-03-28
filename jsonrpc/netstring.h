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

	Netstring(void);
	~Netstring(void);

	static Netstring::RefType ReadFromFIFO(FIFO::RefType fifo);
	bool WriteToFIFO(FIFO::RefType fifo) const;

	size_t GetSize(void) const;
	const void *GetData(void) const;

	void SetString(char *str);
	const char *ToString(void);
};

}

#endif /* I2_NETSTRING_H */
