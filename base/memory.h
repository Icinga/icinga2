#ifndef MEMORY_H
#define MEMORY_H

namespace icinga
{

DEFINE_EXCEPTION_CLASS(OutOfMemoryException);

class I2_BASE_API Memory
{
private:
	Memory(void);

public:
	static void *Allocate(size_t size);
	static void *Reallocate(void *ptr, size_t size);
	static char *StrDup(const char *str);
	static void Free(void *ptr);
};

}

#endif /* MEMORY_H */
