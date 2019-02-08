/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "base/exception.hpp"
#include "base/io-engine.hpp"
#include "base/lazy-init.hpp"
#include "base/logger.hpp"
#include <exception>
#include <memory>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

using namespace icinga;

CpuBoundWork::CpuBoundWork(boost::asio::yield_context yc)
{
	auto& ioEngine (IoEngine::Get());

	for (;;) {
		auto availableSlots (ioEngine.m_CpuBoundSemaphore.fetch_sub(1));

		if (availableSlots < 1) {
			ioEngine.m_CpuBoundSemaphore.fetch_add(1);
			ioEngine.m_AlreadyExpiredTimer.async_wait(yc);
			continue;
		}

		break;
	}
}

CpuBoundWork::~CpuBoundWork()
{
	IoEngine::Get().m_CpuBoundSemaphore.fetch_add(1);
}

LazyInit<std::unique_ptr<IoEngine>> IoEngine::m_Instance ([]() { return std::unique_ptr<IoEngine>(new IoEngine()); });

IoEngine& IoEngine::Get()
{
	return *m_Instance.Get();
}

boost::asio::io_service& IoEngine::GetIoService()
{
	return m_IoService;
}

IoEngine::IoEngine() : m_IoService(), m_KeepAlive(m_IoService), m_AlreadyExpiredTimer(m_IoService)
{
	m_AlreadyExpiredTimer.expires_at(boost::posix_time::neg_infin);

	auto concurrency (std::thread::hardware_concurrency());

	if (concurrency < 2) {
		m_CpuBoundSemaphore.store(1);
	} else {
		m_CpuBoundSemaphore.store(concurrency - 1u);
	}

	for (auto i (std::thread::hardware_concurrency()); i; --i) {
		std::thread(&IoEngine::RunEventLoop, this).detach();
	}
}

void IoEngine::RunEventLoop()
{
	for (;;) {
		try {
			m_IoService.run();

			break;
		} catch (const std::exception& e) {
			Log(LogCritical, "IoEngine", "Exception during I/O operation!");
			Log(LogDebug, "IoEngine") << "Exception during I/O operation: " << DiagnosticInformation(e);
		}
	}
}
