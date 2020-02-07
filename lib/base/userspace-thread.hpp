/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef USERSPACE_THREAD_H
#define USERSPACE_THREAD_H

#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/object.hpp"
#include "base/shared-object.hpp"
#include <boost/context/continuation.hpp>
#include <utility>

namespace icinga
{

/**
 * A lightweight thread.
 *
 * @ingroup base
 */
class UserspaceThread : public SharedObject
{
public:
	DECLARE_PTR_TYPEDEFS(UserspaceThread);

	static void Yield_();

	template<class F>
	inline UserspaceThread(F&& f) : m_Context(FunctionToContext(std::move(f)))
	{
	}

	UserspaceThread(const UserspaceThread&) = delete;
	UserspaceThread(UserspaceThread&&) = delete;
	UserspaceThread& operator=(const UserspaceThread&) = delete;
	UserspaceThread& operator=(UserspaceThread&&) = delete;

private:
	static thread_local boost::context::continuation* m_Parent;

	boost::context::continuation m_Context;

	template<class F>
	inline boost::context::continuation FunctionToContext(F&& f)
	{
		Ptr keepAlive (this);

		return boost::context::callcc([keepAlive, f](boost::context::continuation&& parent) {
			m_Parent = &parent;
			Yield_();

			try {
				f();
			} catch (const std::exception& ex) {
				try {
					Log(LogCritical, "UserspaceThread")
						<< "Exception thrown in UserspaceThread:\n"
						<< DiagnosticInformation(ex);
				} catch (...) {
				}
			} catch (...) {
				try {
					Log(LogCritical, "UserspaceThread", "Exception of unknown type thrown in UserspaceThread.");
				} catch (...) {
				}
			}

			m_Parent = nullptr;

			return std::move(parent);
		});
	}
};

}

#endif /* USERSPACE_THREAD_H */
