#ifndef AMALLOC_H
#define AMALLOC_H

#include <stddef.h>

#define ALIGNMENT __alignof__(long double)
#define CHUNK_SIZE (1024 * 1024)

void *amalloc(size_t);

void afree(void*);

#endif // AMALLOC_H

