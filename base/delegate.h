#ifndef I2_DELEGATE_H
#define I2_DELEGATE_H

#include <list>

namespace icinga
{

using std::tr1::function;
using namespace std::tr1::placeholders;

template<class TObject, class TArgs>
int delegate_fwd(int (TObject::*function)(TArgs), weak_ptr<TObject> wref, const TArgs& args)
{
	shared_ptr<TObject> ref = wref.lock();

	if (ref == NULL)
		return -1;

	return (ref.get()->*function)(args);
}

template<class TObject, class TArgs>
function<int (TArgs)> bind_weak(int (TObject::*function)(TArgs), const weak_ptr<TObject>& wref)
{
	return std::tr1::bind<int>(delegate_fwd<TObject, TArgs>, function, wref, _1);
}

template<class TObject, class TArgs>
function<int (TArgs)> bind_weak(int (TObject::*function)(TArgs), shared_ptr<TObject> ref)
{
	weak_ptr<TObject> wref = weak_ptr<TObject>(ref);
	return bind_weak(function, wref);
}

template<class TObject, class TArgs>
function<int (TArgs)> bind_weak(int (TObject::*function)(TArgs), shared_ptr<Object> ref)
{
	weak_ptr<TObject> wref = weak_ptr<TObject>(static_pointer_cast<TObject>(ref));
	return bind_weak(function, wref);
}

}

#endif /* I2_DELEGATE_H */