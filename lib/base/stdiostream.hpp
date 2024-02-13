/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef STDIOSTREAM_H
#define STDIOSTREAM_H

#include "base/i2-base.hpp"
#include "base/stream.hpp"
#include <iosfwd>
#include <iostream>

namespace icinga {

class StdioStream final : public Stream
{
public:
	DECLARE_PTR_TYPEDEFS(StdioStream);

	StdioStream(std::iostream *innerStream, bool ownsStream);
	~StdioStream() override;

	size_t Read(void *buffer, size_t size) override;
	void Write(const void *buffer, size_t size) override;

	void Close() override;

	bool IsDataAvailable() const override;
	bool IsEof() const override;

private:
	std::iostream *m_InnerStream;
	bool m_OwnsStream;
};

}

#endif /* STDIOSTREAM_H */
