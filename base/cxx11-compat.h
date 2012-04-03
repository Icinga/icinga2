#ifndef CXX11COMPAT_H
#define CXX11COMPAT_H

template <typename T>
shared_ptr<T> make_shared(void)
{
	return shared_ptr<T>(new T());
}

template <typename T, typename TArg1>
shared_ptr<T> make_shared(const TArg1& arg1)
{
	return shared_ptr<T>(new T());
}

template <typename T, typename TArg1, typename TArg2>
shared_ptr<T> make_shared(const TArg1& arg1, const TArg2& arg2)
{
	return shared_ptr<T>(new T());
}

#endif /* CXX11COMPAT_H */
