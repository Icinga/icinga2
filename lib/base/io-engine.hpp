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

#ifndef IO_ENGINE_H
#define IO_ENGINE_H

#include "base/lazy-init.hpp"
#include <atomic>
#include <exception>
#include <memory>
#include <thread>
#include <vector>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>

/**
 * Scope lock for CPU-bound work done in an I/O thread
 *
 * @ingroup base
 */
class CpuBoundWork
{
public:
	CpuBoundWork(boost::asio::yield_context yc);
	CpuBoundWork(const CpuBoundWork&) = delete;
	CpuBoundWork(CpuBoundWork&&) = delete;
	CpuBoundWork& operator=(const CpuBoundWork&) = delete;
	CpuBoundWork& operator=(CpuBoundWork&&) = delete;
	~CpuBoundWork();

	void Done();

private:
	bool m_Done;
};

/**
 * Scope break for CPU-bound work done in an I/O thread
 *
 * @ingroup base
 */
class IoBoundWorkSlot
{
public:
	IoBoundWorkSlot(boost::asio::yield_context yc);
	IoBoundWorkSlot(const IoBoundWorkSlot&) = delete;
	IoBoundWorkSlot(IoBoundWorkSlot&&) = delete;
	IoBoundWorkSlot& operator=(const IoBoundWorkSlot&) = delete;
	IoBoundWorkSlot& operator=(IoBoundWorkSlot&&) = delete;
	~IoBoundWorkSlot();

private:
	boost::asio::yield_context yc;
};

/**
 * Async I/O engine
 *
 * @ingroup base
 */
class IoEngine
{
	friend CpuBoundWork;
	friend IoBoundWorkSlot;

public:
	IoEngine(const IoEngine&) = delete;
	IoEngine(IoEngine&&) = delete;
	IoEngine& operator=(const IoEngine&) = delete;
	IoEngine& operator=(IoEngine&&) = delete;
	~IoEngine();

	static IoEngine& Get();

	boost::asio::io_service& GetIoService();

private:
	IoEngine();

	void RunEventLoop();

	static LazyInit<std::unique_ptr<IoEngine>> m_Instance;

	boost::asio::io_service m_IoService;
	boost::asio::io_service::work m_KeepAlive;
	std::vector<std::thread> m_Threads;
	boost::asio::deadline_timer m_AlreadyExpiredTimer;
	std::atomic_int_fast32_t m_CpuBoundSemaphore;
};

class TerminateIoThread : public std::exception
{
};

#endif /* IO_ENGINE_H */
