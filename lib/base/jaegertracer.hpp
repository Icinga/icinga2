/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef TRACER_HPP
#define TRACER_HPP

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <jaegertracing/Tracer.h>

namespace icinga
{

typedef std::unique_ptr<opentracing::Span> jspan;

class JaegerTracer
{
public:
	JaegerTracer() {};
	~JaegerTracer() {};

	void SetupTracer(const String& serviceToTrace);
	void TracedSubRoutine(jspan& parentSpan, const String& subRoutineContext);
	jspan TracedFunction(const String& functionContext);

	String Inject(jspan& span, const String& name);
	void Extract(jspan& span, const String& name, const String& meta);

	void Close();
};

}

#endif /* TRACER_HPP */
