/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef FIFO_H
#define FIFO_H

#include "base/i2-base.hpp"
#include "base/stream.hpp"

namespace icinga
{

/**
 * A byte-based FIFO buffer.
 *
 * @ingroup base
 */
class FIFO final : public Stream
{
public:
	DECLARE_PTR_TYPEDEFS(FIFO);

	static const size_t BlockSize = 512;

	~FIFO() override;

	size_t Read(void *buffer, size_t count, bool allow_partial = false) override;
	void Write(const void *buffer, size_t count) override;
	void Close() override;
	bool IsEof() const override;
	bool SupportsWaiting() const override;
	bool IsDataAvailable() const override;

	size_t GetAvailableBytes() const;

private:
	char *m_Buffer{nullptr};
	size_t m_DataSize{0};
	size_t m_AllocSize{0};
	size_t m_Offset{0};

	void ResizeBuffer(size_t newSize, bool decrease);
	void Optimize();
};

}

#endif /* FIFO_H */
