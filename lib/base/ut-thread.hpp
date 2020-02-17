/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef UT_THREAD_H
#define UT_THREAD_H

#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/object.hpp"
#include "base/shared-object.hpp"
#include "base/ut-current.hpp"
#include "base/ut-mgmt.hpp"
#include <boost/context/continuation.hpp>
#include <unordered_map>
#include <utility>

namespace icinga
{
namespace UT
{

/**
 * A lightweight thread.
 *
 * @ingroup base
 */
class Thread : public SharedObject
{
public:
	DECLARE_PTR_TYPEDEFS(Thread);

	template<class F>
	Thread(F&& f)
		: m_Parent(nullptr), m_Context(FunctionToContext(std::move(f)))
	{
		Queue::Default.Push(this);
	}

	Thread(const Thread&) = delete;
	Thread(Thread&&) = delete;
	Thread& operator=(const Thread&) = delete;
	Thread& operator=(Thread&&) = delete;

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
			l_UserspaceThreads.fetch_add(1);
			m_Parent = &parent;
			Current::m_Thread = this;
			Current::Yield_();

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

			Current::m_Thread = nullptr;
			l_UserspaceThreads.fetch_sub(1);

			return std::move(parent);
		});
	}
};

}
}

#endif /* UT_THREAD_H */
