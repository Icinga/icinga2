// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONFIGFRAGMENT_H
#define CONFIGFRAGMENT_H

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

#endif /* CONFIGFRAGMENT_H */
