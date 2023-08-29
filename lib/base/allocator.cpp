/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#ifndef _WIN32

#include "base/allocator.hpp"
#include <algorithm>
#include <boost/config.hpp>
#include <cstddef>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace icinga;

struct DAPage
{
	size_t TotalSize = sizeof(DAPage);
	DAPage* Next = nullptr;
	max_align_t* MemoryBegin = nullptr;
	max_align_t* MemoryEnd = nullptr;
	max_align_t Memory[1] = {};
};

static thread_local struct {
	unsigned int InUse = 0;
	unsigned int Paused = 0;
	DAPage* TopPage = nullptr;
} l_DefragAllocator;

extern "C" void* malloc(size_t bytes)
{
	static const auto libcMalloc = (void*(*)(size_t))dlsym(RTLD_NEXT, "malloc");
	static const size_t pageSize = std::max(128L * 1024L, sysconf(_SC_PAGESIZE));

	if (BOOST_LIKELY(!l_DefragAllocator.InUse || l_DefragAllocator.Paused)) {
		return libcMalloc(bytes);
	}

	if (BOOST_UNLIKELY(!bytes)) {
		return nullptr;
	}

	auto chunks ((bytes + sizeof(max_align_t) - 1u) / sizeof(max_align_t));

	if (BOOST_UNLIKELY(!l_DefragAllocator.TopPage || l_DefragAllocator.TopPage->MemoryEnd - l_DefragAllocator.TopPage->MemoryBegin < chunks)) {
		auto total ((sizeof(DAPage) + sizeof(max_align_t) * (chunks - 1u) + pageSize - 1u) / pageSize * pageSize);

		auto nextPage (mmap(nullptr, total, PROT_READ | PROT_WRITE, MAP_ANONYMOUS
#ifdef MAP_HASSEMAPHORE
		| MAP_HASSEMAPHORE
#endif
		| MAP_PRIVATE, -1, 0));

		if (BOOST_UNLIKELY(nextPage == MAP_FAILED)) {
			return nullptr;
		}

		auto p ((DAPage*)nextPage);

		p->TotalSize = total;
		p->Next = l_DefragAllocator.TopPage;
		p->MemoryBegin = p->Memory;
		p->MemoryEnd = p->MemoryBegin + chunks;

		l_DefragAllocator.TopPage = p;
	}

	auto memory ((void*)l_DefragAllocator.TopPage->MemoryBegin);

	l_DefragAllocator.TopPage->MemoryBegin += chunks;

	return memory;
}

extern "C" void free(void* memory)
{
	static const auto libcFree = (void(*)(void*))dlsym(RTLD_NEXT, "free");

	if (BOOST_LIKELY(!l_DefragAllocator.InUse || l_DefragAllocator.Paused)) {
		libcFree(memory);
	}
}

DefragAllocator::DefragAllocator()
{
	++l_DefragAllocator.InUse;
}

DefragAllocator::~DefragAllocator()
{
	if (!--l_DefragAllocator.InUse) {
		while (l_DefragAllocator.TopPage) {
			auto next (l_DefragAllocator.TopPage->Next);

			munmap(l_DefragAllocator.TopPage, l_DefragAllocator.TopPage->TotalSize);
			l_DefragAllocator.TopPage = next;
		}
	}
}

DefaultAllocator::DefaultAllocator()
{
	++l_DefragAllocator.Paused;
}

DefaultAllocator::~DefaultAllocator()
{
	--l_DefragAllocator.Paused;
}

#endif /* _WIN32 */
