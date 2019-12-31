// Wrapper for memory allocation functions
// Useful for clearing all allocated memory in the event of a failure

#include <smem.h>

struct alloced {
    void *address;
    struct alloced *next;
	struct alloced *prev;
};

static struct {
    struct alloced *first;
    struct alloced *last;
} alloced_mem = {
    .first = NULL,
    .last = NULL
};

void release(void)
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

static struct alloced *find_memory(void *ptr)
{
    for (struct alloced *mem = alloced_mem.last; mem != NULL; mem = mem->prev) {
        if (mem->address == ptr) {
            return mem;
        }
    }
    die("Failed to locate provided address, are you sure you salloced it?\n");
}

void smem_diagnostic(void)
{
    struct alloced *a = alloced_mem.first;
    while (a != NULL) {
        printf("%p\n", a->address);
        a = a->next;
    }
    puts("");
}

void *salloc(size_t size)
{
    void *ptr = NULL;
    if ((ptr = malloc(size)) == NULL) {
        die("Failed to allocate memory\n");
    }
    
	saquire(ptr);
    
    return ptr;
}

void *saquire(void *ptr)
{
    if (ptr == NULL) {
        return NULL;
    }

	struct alloced *a;
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

void *srealloc(void *ptr, size_t size)
{
    struct alloced *a = find_memory(ptr);
    
    if ((ptr = realloc(ptr, size)) == NULL) {
        die("Failed to reallocate memory\n");
    }
    
    a->address = ptr;
    return ptr;
}

void sfree(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

	struct alloced *a = find_memory(ptr);

	struct alloced *prev = a->prev;
	struct alloced *next = a->next;

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
