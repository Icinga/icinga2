/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef GENERATOR_FUNCTION_H
#define GENERATOR_FUNCTION_H

#include "base/generator.hpp"
#include "base/value.hpp"
#include "config/expression.hpp"
#include <condition_variable>
#include <exception>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

namespace icinga
{

/**
 * Supplier a generator function's items.
 *
 * @ingroup config
 */
class GeneratorFunction final : public Generator
{
public:
	class ForcedUnwind
	{
	};

	GeneratorFunction(const Expression::Ptr& expression);
	~GeneratorFunction() override;

	bool GetNext(Value& out) override;
	void YieldItem(Value item);

private:
	std::thread m_Thread;

	struct {
		std::queue<std::promise<std::pair<Value, bool>>> NextQueue;
		bool Shutdown = false;
		std::mutex Lock;
		std::condition_variable CV;
		std::exception_ptr Exception;
	} m_ITC;
};

extern thread_local GeneratorFunction* l_GeneratorFunction;

}

#endif /* GENERATOR_FUNCTION_H */
