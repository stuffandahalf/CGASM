#ifndef __SMEM_H
#define __SMEM_H

#include <stdio.h>
#include <stdlib.h>

void release();
void *salloc(size_t size);
void *srealloc(void *ptr, size_t size);
void sfree(void *ptr);

#define die(fmt, ...) { \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    release(); \
    exit(1); \
}

#endif
