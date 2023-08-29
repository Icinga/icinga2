/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#pragma once

namespace icinga
{

/**
 * TODO
 *
 * @ingroup base
 */
class DefragAllocator
{
public:
	DefragAllocator();
	DefragAllocator(const DefragAllocator&) = delete;
	DefragAllocator(DefragAllocator&&) = delete;
	DefragAllocator& operator=(const DefragAllocator&) = delete;
	DefragAllocator& operator=(DefragAllocator&&) = delete;
	~DefragAllocator();
};

}
