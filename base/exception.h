#ifndef EXCEPTION_H
#define EXCEPTION_H

namespace icinga
{

class I2_BASE_API Exception
{
private:
	string m_Message;

public:
	typedef shared_ptr<Exception> Ptr;
	typedef weak_ptr<Exception> WeakPtr;

	Exception(void);
	Exception(const string& message);

	virtual ~Exception(void);

	string GetMessage(void) const;
};

#define DEFINE_EXCEPTION_CLASS(klass)								\
	class klass : public Exception									\
	{																\
	public:															\
		typedef shared_ptr<klass> Ptr;								\
		typedef weak_ptr<klass> WeakPtr;							\
																	\
		inline klass(void) : Exception()							\
		{															\
		}															\
																	\
		inline klass(const string& message) : Exception(message)	\
		{															\
		}															\
	};

DEFINE_EXCEPTION_CLASS(NotImplementedException);
DEFINE_EXCEPTION_CLASS(InvalidArgumentException);

}

#endif /* EXCEPTION_H */
