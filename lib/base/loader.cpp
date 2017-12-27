/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "base/loader.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"

using namespace icinga;

std::priority_queue<DeferredInitializer>& Loader::GetDeferredInitializers(void)
{
	thread_local std::priority_queue<DeferredInitializer> initializers;
	return initializers;
}

void Loader::ExecuteDeferredInitializers(void)
{
	auto& initializers = GetDeferredInitializers();

	while (!initializers.empty()) {
		DeferredInitializer initializer = initializers.top();
		initializers.pop();
		initializer();
	}
}

void Loader::AddDeferredInitializer(const std::function<void(void)>& callback, int priority)
{
	GetDeferredInitializers().emplace(callback, priority);
}

DeferredInitializer::DeferredInitializer(const std::function<void (void)>& callback, int priority)
	: m_Callback(callback), m_Priority(priority)
{ }

bool DeferredInitializer::operator<(const DeferredInitializer& other) const
{
	return m_Priority < other.m_Priority;
}

void DeferredInitializer::operator()(void)
{
	m_Callback();
}
