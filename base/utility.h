#ifndef UTILITY_H
#define UTILITY_H

namespace icinga
{

/**
 * Utility
 *
 * Utility functions.
 */
class I2_BASE_API Utility
{
private:
	Utility(void);

public:
	/**
	 * GetTypeName
	 *
	 * Returns the type name of an object (using RTTI).
	 */
	template<class T>
	static string GetTypeName(const T& value)
	{
		string klass = typeid(value).name();

#ifdef HAVE_GCC_ABI_DEMANGLE
		int status;
		char *realname = abi::__cxa_demangle(klass.c_str(), 0, 0, &status);

		if (realname != NULL) {
			klass = string(realname);
			free(realname);
		}
#endif /* HAVE_GCC_ABI_DEMANGLE */

		return klass;
	}

	static void Daemonize(void);
};

}

#endif /* UTILITY_H */
