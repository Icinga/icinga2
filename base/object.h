#ifndef OBJECT_H
#define OBJECT_H

namespace icinga
{

/**
 * Object
 *
 * Base class for all heap-allocated objects. At least one of its methods
 * has to be virtual for RTTI to work.
 */
class I2_BASE_API Object : public enable_shared_from_this<Object>
{
private:
	Object(const Object& other);

protected:
	inline Object(void)
	{
	}

	inline virtual ~Object(void)
	{
	}

public:
	typedef shared_ptr<Object> Ptr;
	typedef weak_ptr<Object> WeakPtr;
};

template<class T>
struct weak_ptr_eq_raw
{
private:
	const void *m_Ref;

public:
	weak_ptr_eq_raw(const void *ref) : m_Ref(ref) { }

	bool operator()(const weak_ptr<T>& wref) const
	{
		return (wref.lock().get() == (const T *)m_Ref);
	}
};

typedef function<Object::Ptr ()> factory_function;

/**
 * factory<T>
 *
 * Returns a new object of type T.
 */
template<class T>
Object::Ptr factory(void)
{
	return make_shared<T>();
}

}

#endif /* OBJECT_H */
