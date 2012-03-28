#ifndef I2_OBJECT_H
#define I2_OBJECT_H

namespace icinga
{

using std::tr1::shared_ptr;
using std::tr1::weak_ptr;
using std::tr1::enable_shared_from_this;
using std::tr1::static_pointer_cast;
using std::tr1::function;

class Object : public enable_shared_from_this<Object>
{
private:
	Object(const Object &other);

protected:
	Object(void);

public:
	typedef shared_ptr<Object> RefType;
	typedef weak_ptr<Object> WeakRefType;

	static unsigned long ActiveObjects;

	virtual ~Object(void);
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

template<class T>
shared_ptr<T> new_object(void)
{
	T *instance = new T();

	return shared_ptr<T>(instance);
}

typedef function<Object::RefType ()> factory_function;

template<class T>
Object::RefType factory(void)
{
	return new_object<T>();
}

}

#endif /* I2_OBJECT_H */
