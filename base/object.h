#ifndef OBJECT_H
#define OBJECT_H

namespace icinga
{

class Object : public enable_shared_from_this<Object>
{
private:
	Object(const Object &other);

protected:
	Object(void);

public:
	typedef shared_ptr<Object> Ptr;
	typedef weak_ptr<Object> WeakPtr;

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

typedef function<Object::Ptr ()> factory_function;

template<class T>
Object::Ptr factory(void)
{
	return new_object<T>();
}

}

#endif /* OBJECT_H */
