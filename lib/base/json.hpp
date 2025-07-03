/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef JSON_H
#define JSON_H

#include "base/i2-base.hpp"
#include <boost/asio/spawn.hpp>
#include <json.hpp>

namespace icinga
{

/**
 * AsyncJsonWriter allows writing JSON data to any output stream asynchronously.
 *
 * All users of this class must ensure that the underlying output stream will not perform any asynchronous I/O
 * operations when the @c write_character() or @c write_characters() methods are called. They shall only perform
 * such ops when the @c JsonEncoder allows them to do so by calling the @c Flush() method.
 *
 * @ingroup base
 */
class AsyncJsonWriter : public nlohmann::detail::output_adapter_protocol<char>
{
public:
	/**
	 * Flush instructs the underlying output stream to write any buffered data to wherever it is supposed to go.
	 *
	 * The @c JsonEncoder allows the stream to even perform asynchronous operations in a safe manner by calling
	 * this method with a dedicated @c boost::asio::yield_context object. The stream must not perform any async
	 * I/O operations triggered by methods other than this one. Any attempt to do so will result in undefined behavior.
	 *
	 * However, this doesn't necessarily enforce the stream to really flush its data immediately, but it's up
	 * to the implementation to do whatever it needs to. The encoder just gives it a chance to do so by calling
	 * this method.
	 *
	 * @param yield The yield context to use for asynchronous operations.
	 */
	virtual void Flush(boost::asio::yield_context& yield) = 0;
};

class String;
class Value;

String JsonEncode(const Value& value, bool pretty_print = false);
Value JsonDecode(const String& data);

}

#endif /* JSON_H */
