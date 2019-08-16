/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef FIBERS_H
#define FIBERS_H

#include <boost/asio.hpp>
#include <boost_shipped/libs/fiber/examples/asio/yield.hpp>
#include <memory>

namespace icinga
{
namespace Fibers
{

extern std::shared_ptr<boost::asio::io_context> IoContext;

extern thread_local
boost::fibers::asio::yield_t Yield;

void StartEngine();

}
}

#endif /* FIBERS_H */
