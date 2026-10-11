#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include "sys/mman.h"

#include "amalloc.h"

typedef struct block {
    unsigned int is_free : 1;
    size_t size;

    struct chunk *owner;
    struct block *next;
    struct block *prev;
} block_t;

typedef struct chunk {
    block_t *free_list;

    struct chunk *next;
} chunk_t;

_Static_assert(CHUNK_SIZE > sizeof(chunk_t) + sizeof(block_t), "CHUNK_SIZE too small for chunk and block headers");

static chunk_t *chunks;

static chunk_t *get_last_chunk(void) {
    if (!chunks) {
        return NULL;
    }

    chunk_t *last_chunk = chunks;
    while (last_chunk->next) {
        last_chunk = last_chunk->next;
    }

    return last_chunk;
}

static int create_chunk(void) {
    void *mem = mmap(NULL, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (mem == MAP_FAILED) {
        return ENOMEM;
    }

    chunk_t *chunk = mem;

    block_t *block = (block_t*)((char*)mem + sizeof(chunk_t));
    block->is_free = 1;
    block->size = CHUNK_SIZE - sizeof(chunk_t) - sizeof(block_t);
    block->owner = chunk;
    block->next = NULL;
    block->prev = NULL;

    chunk->free_list = block;
    chunk->next = NULL;

    chunk_t *last_chunk = get_last_chunk();
    if (last_chunk) {
        last_chunk->next = chunk;
    } else {
        chunks = chunk;
    }

    return 0;
}

static void cut_block(chunk_t *chunk, block_t *block, size_t size) {
    if (block->size - size >= sizeof(block_t) + ALIGNMENT) {
        block_t *new_block = (block_t*)((char*)block + sizeof(block_t) + size);

        if (block->prev) {
            block->prev->next = new_block;
        } else {
            chunk->free_list = new_block;
        }

        if (block->next) {
            block->next->prev = new_block;
        }

        new_block->is_free = 1;
        new_block->size = block->size - size - sizeof(block_t);
        new_block->owner = chunk;
        new_block->next = block->next;
        new_block->prev = block->prev;

        block->size = size;
    } else {
        if (block->prev) {
            block->prev->next = block->next;
        } else {
            chunk->free_list = block->next;
        }

        if (block->next) {
            block->next->prev = block->prev;
        }
    }

    block->next = NULL;
    block->prev = NULL;
    block->is_free = 0;
}

void *amalloc(size_t size) {
    if (size == 0 || size > SIZE_MAX - ALIGNMENT) {
        return NULL;
    }

    size = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

    if (size > CHUNK_SIZE - sizeof(chunk_t) - sizeof(block_t)) {
        return NULL;
    }

    if (chunks == NULL && create_chunk() != 0) {
        return NULL;
    }

    for (chunk_t *chunk = chunks; chunk; chunk = chunk->next) {
        if (chunk->free_list == NULL) {
            continue;
        }

        for (block_t *block = chunk->free_list; block; block = block->next) {
            if (block->size >= size) {
                cut_block(chunk, block, size);

                return (char*)block + sizeof(block_t);
            }
        }
    }

    if (create_chunk() != 0) {
        return NULL;
    }

    chunk_t *last_chunk = get_last_chunk();

    block_t *block = last_chunk->free_list;

    cut_block(last_chunk, block, size);

    return (char*)block + sizeof(block_t);
}

void afree(void *mem) {
    if (mem == NULL) {
        return;
    }

    block_t *block = (block_t*)((char*)mem - sizeof(block_t));
    chunk_t *chunk = block->owner;

    if (block->is_free) {
        return;
    }

    block_t *next_block = (block_t*)((char*)block + sizeof(block_t) + block->size);

    if ((char*)next_block < (char*)chunk + CHUNK_SIZE && next_block->is_free) {
        block->size += sizeof(block_t) + next_block->size;

        if (next_block->prev) {
            next_block->prev->next = next_block->next;
        } else {
            chunk->free_list = next_block->next;
        }

        if (next_block->next) {
            next_block->next->prev = next_block->prev;
        }
    }

    block_t *current_block = (block_t*)((char*)chunk + sizeof(chunk_t));
    block_t *prev_block = NULL;

    while (current_block && current_block != block) {
        prev_block = current_block;
        current_block = (block_t*)((char*)current_block + sizeof(block_t) + current_block->size);
    }

    if (prev_block && prev_block->is_free) {
        prev_block->size += sizeof(block_t) + block->size;
    } else {
        block->is_free = 1;
        block->next = chunk->free_list;

        if (block->next) {
            block->next->prev = block;
        }

        chunk->free_list = block;
    }
}
