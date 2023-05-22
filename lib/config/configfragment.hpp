/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "config/configcompiler.hpp"
#include "base/initialize.hpp"
#include "base/debug.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"

/* Ensure that the priority is lower than the basic namespace initialization in scriptframe.cpp. */
#define REGISTER_CONFIG_FRAGMENT(name, fragment) \
	INITIALIZE_ONCE_WITH_PRIORITY([]() { \
		std::unique_ptr<icinga::Expression> expression = icinga::ConfigCompiler::CompileText(name, fragment); \
		VERIFY(expression); \
		try { \
			icinga::ScriptFrame frame(true); \
			expression->Evaluate(frame); \
		} catch (const std::exception& ex) { \
			std::cerr << icinga::DiagnosticInformation(ex) << std::endl; \
			icinga::Application::Exit(1); \
		} \
	}, icinga::InitializePriority::EvaluateConfigFragments)
