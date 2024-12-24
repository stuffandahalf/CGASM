/* Wrapper for memory allocation functions */
/* Useful for clearing all allocated memory in the event of a failure */

#include <stdarg.h>
#include "smem.h"

struct alloced {
	void *address;
	struct alloced *next;
	struct alloced *prev;
};

static struct {
	struct alloced *first;
	struct alloced *last;
} alloced_mem = { NULL, NULL };

void
release(void)
{
	struct alloced *a = alloced_mem.first;
	while (a != NULL) {
		struct alloced *tmp = a;
		a = a->next;
		free(tmp->address);
		free(tmp);
	}
	alloced_mem.first = NULL;
	alloced_mem.last = NULL;
}

static struct alloced *
find_memory(void *ptr)
{
	struct alloced *mem;
	for (mem = alloced_mem.last; mem != NULL; mem = mem->prev) {
		if (mem->address == ptr) {
			return mem;
		}
	}
	die("Failed to locate provided address, are you sure you salloced it?\n");
	return NULL; /* not reachable */
}

void
smem_diagnostic(void)
{
	struct alloced *a = alloced_mem.first;
	while (a != NULL) {
		printf("%p\n", a->address);
		a = a->next;
	}
	puts("");
}

#ifdef SMEM_CPP
void *salloc_real(size_t size)
#else
void *salloc(size_t size)
#endif
{
	void *ptr = NULL;
	if ((ptr = malloc(size)) == NULL) {
		die("Failed to allocate memory\n");
	}

#ifdef SMEM_CPP
	saquire_real(ptr);
#else
	saquire(ptr);
#endif
	
	return ptr;
}

void *
#ifdef SMEM_CPP
saquire_real(void *ptr)
#else
saquire(void *ptr)
#endif
{
	struct alloced *a;

	if (ptr == NULL) {
		return NULL;
	}

	if ((a = malloc(sizeof(struct alloced))) == NULL) {
		die("Failed to allocate storage for allocated memory\n");
	}

	a->address = ptr;
	a->next = NULL;

	if (alloced_mem.first == NULL) {
		alloced_mem.first = a;
		a->prev = NULL;
	} else {
		alloced_mem.last->next = a;
		a->prev = alloced_mem.last;
	}
	alloced_mem.last = a;

	return a->address;
}

void *
#ifdef SMEM_CPP
srealloc_real(void *ptr, size_t size)
#else
srealloc(void *ptr, size_t size)
#endif
{
	struct alloced *a = find_memory(ptr);
	
	if ((ptr = realloc(ptr, size)) == NULL) {
		die("Failed to reallocate memory\n");
	}
	
	a->address = ptr;
	return ptr;
}

void
sfree(void *ptr)
{
	struct alloced *a, *prev, *next;

	if (ptr == NULL) {
		return;
	}

	a = find_memory(ptr);
	prev = a->prev;
	next = a->next;

	if (prev != NULL) {
		prev->next = next;
	}
	if (next != NULL) {
		next->prev = prev;
	}
	
	if (alloced_mem.first == a) {
		alloced_mem.first = a->next;
	}
	if (alloced_mem.last == a) {
		alloced_mem.last = a->prev;
	}
	
	free(a->address);
	free(a);
}

NORETURN void
die(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	release();
	AWAIT_WINDOWS;
	exit(1);
}

