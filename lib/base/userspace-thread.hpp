/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef USERSPACE_THREAD_H
#define USERSPACE_THREAD_H

#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/object.hpp"
#include "base/shared-object.hpp"
#include "base/ut-current.hpp"
#include "base/ut-mgmt.hpp"
#include <boost/context/continuation.hpp>
#include <unordered_map>
#include <utility>

#ifndef _WIN32
#	include <unistd.h>
#endif /* _WIN32 */

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

#ifndef _WIN32
	static decltype(fork()) Fork();
#endif /* _WIN32 */

	template<class F>
	UserspaceThread(F&& f)
		: m_Parent(nullptr), m_Context(FunctionToContext(std::move(f)))
	{
		UT::Queue::Default.Push(this);
	}

	UserspaceThread(const UserspaceThread&) = delete;
	UserspaceThread(UserspaceThread&&) = delete;
	UserspaceThread& operator=(const UserspaceThread&) = delete;
	UserspaceThread& operator=(UserspaceThread&&) = delete;

	bool Resume();

	boost::context::continuation* m_Parent;

private:
	boost::context::continuation m_Context;
	std::unordered_map<void*, SharedObject::Ptr> m_Locals;

	template<class F>
	inline boost::context::continuation FunctionToContext(F&& f)
	{
		Ptr keepAlive (this);

		return boost::context::callcc([this, keepAlive, f](boost::context::continuation&& parent) {
			UT::l_UserspaceThreads.fetch_add(1);
			m_Parent = &parent;
			UT::Current::m_Thread = this;
			UT::Current::Yield_();

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

			UT::Current::m_Thread = nullptr;
			UT::l_UserspaceThreads.fetch_sub(1);

			return std::move(parent);
		});
	}
};

}

#endif /* USERSPACE_THREAD_H */
