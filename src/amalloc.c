#include <asm-generic/errno-base.h>
#include <stddef.h>
#include "sys/mman.h"

#include "../include/amalloc.h"

static allocator_t allocator;

static int init_amalloc(void) {
	void *mem = mmap(NULL, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	if (mem == MAP_FAILED) return ENOMEM;

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

static int add_chunk(void) {
	void *mem = mmap(NULL, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	if (mem == MAP_FAILED) return ENOMEM;

	chunk_t *chunk = (chunk_t*)mem;

	block_t *block = (block_t*)(mem + sizeof(chunk_t));
	block->is_free = 1;
	block->size = CHUNK_SIZE - sizeof(chunk_t) - sizeof(block_t);
	block->next = NULL;
	block->prev = NULL;

	chunk->free_list = block;
	chunk->next = NULL;

	if (allocator.chunks == NULL) allocator.chunks = chunk;

	else {
		chunk_t *curr = allocator.chunks;
		while (curr->next) curr = curr->next;

		curr->next = chunk;
	}

	return 0;
}

static void cut_block(chunk_t *chunk, block_t *block, size_t size) {
	if (block->size - size >= sizeof(block_t) + ALIGNMENT) {
		block_t *new_block = (block_t*)((char*)block + sizeof(block_t) + size);

		if (block->prev) block->prev->next = new_block;
		else chunk->free_list = new_block;

		if (block->next) block->next->prev = new_block;

		new_block->is_free = 1;
		new_block->size = block->size - size - sizeof(block_t);
		new_block->next = block->next;
		new_block->prev = block->prev;

		block->size = size;
	} else {
		if (block->prev) block->prev->next = block->next;
		else chunk->free_list = block->next;

		if (block->next) block->next->prev = block->prev;
	}

	block->is_free = 0;
}

void *amalloc(size_t size) {
	if (size == 0) return NULL;

	size = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

	if (size > CHUNK_SIZE - sizeof(chunk_t) - sizeof(block_t)) return NULL;

	if (allocator.chunks == NULL && init_amalloc() != 0) return NULL;

	for (chunk_t *chunk = allocator.chunks; chunk; chunk = chunk->next) {
		if (!chunk->free_list) continue;

		for (block_t *block = chunk->free_list; block; block = block->next) {
			if (block->size >= size) {
				cut_block(chunk, block, size);

				return (char*)block + sizeof(block_t);
			}
		}
	}

	if (add_chunk() != 0) return NULL;

	chunk_t *chunk = allocator.chunks;
	while (chunk->next) chunk = chunk->next;

	block_t *block = chunk->free_list;

	cut_block(chunk, block, size);

	return (char*)block + sizeof(block_t); 
}

void afree(void *mem);

