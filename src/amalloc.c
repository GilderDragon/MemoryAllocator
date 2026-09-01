#include <asm-generic/errno-base.h>
#include <stddef.h>
#include "sys/mman.h"

#include "../include/amalloc.h"

static allocator_t allocator = {0};

int init_amalloc() {
	void *mem = mmap(NULL, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	if (mem == MAP_FAILED) {
		return ENOMEM;
	}

	chunk_t *chunk = (chunk_t*)mem;

	block_t *block = (block_t*)(mem + sizeof(chunk_t));
	block->is_free = 1;
	block->size = CHUNK_SIZE - sizeof(chunk_t) - sizeof(block_t);
	block->next = NULL;
	block->prev = NULL;

	chunk->free_list = block;
	chunk->next = NULL;

	allocator.chunks = chunk;

	return 0;
}

int add_chunk() {
	void *mem = mmap(NULL, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	if (mem == MAP_FAILED) {
		return ENOMEM;
	}

	chunk_t *chunk = (chunk_t*)mem;

	block_t *block = (block_t*)(mem + sizeof(chunk_t));
	block->is_free = 1;
	block->size = CHUNK_SIZE - sizeof(chunk_t) - sizeof(block_t);
	block->next = NULL;
	block->prev = NULL;

	chunk->free_list = block;
	chunk->next = NULL;

	if (allocator.chunks == NULL) {
		allocator.chunks = chunk;
	} else {
		chunk_t *curr = allocator.chunks;
		while (curr->next) {
			curr = curr->next;
		}

		curr->next = chunk;
	}

	return 0;
}

void *amalloc(size_t size);

void afree(void *mem);

