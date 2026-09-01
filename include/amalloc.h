#ifndef AMALLOC_H
#define AMALLOC_H

#include <stddef.h>

#define CHUNK_SIZE (1024 * 1024)

typedef struct block {
	unsigned int is_free : 1;
	size_t size;

	struct block *next;
	struct block *prev;
} block_t;

typedef struct chunk {
	block_t *free_list;

	struct chunk *next;
} chunk_t;

typedef struct allocator {
	chunk_t *chunks;
} allocator_t;

void *amalloc(size_t size);

void afree(void *mem);

#endif // AMALLOC_H

