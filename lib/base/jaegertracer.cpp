/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#include "base/jaegertracer.hpp"
#include <iostream>

using namespace icinga;

void JaegerTracer::SetupTracer(const String& service)
{
	auto configYAML = YAML::LoadFile("/usr/local/icinga/icinga2/etc/icinga2/jaegertracing.yml"); // TODO
	auto config = jaegertracing::Config::parse(configYAML);
	auto tracer = jaegertracing::Tracer::make(service, config, jaegertracing::logging::consoleLogger());

	opentracing::Tracer::InitGlobal(std::static_pointer_cast<opentracing::Tracer>(tracer));
}

void JaegerTracer::TracedSubRoutine(jspan& parentSpan, const String& subRoutineContext)
{
	auto span = opentracing::Tracer::Global()->StartSpan(subRoutineContext.CStr(), { opentracing::ChildOf(&parentSpan->context()) });
	span->Finish();
}

jspan JaegerTracer::TracedFunction(const String& functionContext)
{
	auto span = opentracing::Tracer::Global()->StartSpan(functionContext.CStr());
	span->Finish();

	return span;
}

String JaegerTracer::Inject(jspan& span, const String& name)
{
	std::stringstream ss;

	if (!span)
			auto span = opentracing::Tracer::Global()->StartSpan(name.CStr());

	auto err = opentracing::Tracer::Global()->Inject(span->context(), ss);

	ASSERT(err);

	return ss.str();
}

void JaegerTracer::Extract(jspan& span, const String& name, const String& meta)
{
	std::stringstream ss(meta.CStr());

	auto spanContext = opentracing::Tracer::Global()->Extract(ss);

	auto propagationSpan = opentracing::Tracer::Global()->StartSpan("propagationSpan", {ChildOf(spanContext->get())});

	propagationSpan->Finish();
}

void JaegerTracer::Close()
{
	opentracing::Tracer::Global()->Close();
}

