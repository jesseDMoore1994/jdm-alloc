#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jdm-alloc.h"
#include "jdm-alloc-internal.h"

typedef struct jdm_pool_free_node_t jdm_pool_free_node_t;
struct jdm_pool_free_node_t {
    jdm_pool_free_node_t* next; // pointer to next free node
};

typedef struct {
    u8* memory; // pointer to memory block
    u8* start; // pointer to first chunk
    size_t size; // total size of the memory block
    size_t chunk_size; // size of each chunk
    jdm_pool_free_node_t* head; // head of free list
} jdm_pool_t;

void* jdm_alloc_pool_alloc(jdm_alloc_t* alloc) {
    if (alloc->type != JDM_ALLOC_POOL) {
        fprintf(stderr, "Invalid allocator type for jdm_alloc_pool_alloc\n");
        return NULL;
    }
    if (!alloc || !alloc->internal_data) {
        return NULL;
    }
    jdm_pool_t* pool = (jdm_pool_t*)alloc->internal_data;
    if (!pool->head) {
        fprintf(stderr, "Pool out of memory\n");
        return NULL;
    }
    jdm_pool_free_node_t* node = pool->head;
    pool->head = node->next;
    return (void*)node;
}

static void* pool_alloc(jdm_alloc_t* alloc, size_t size) {
    if (!alloc || !alloc->internal_data) {
        return NULL;
    }
    jdm_pool_t* pool = (jdm_pool_t*)alloc->internal_data;
    if (size != pool->chunk_size) {
        fprintf(stderr, "Pool allocator can only allocate fixed-size chunks of size %zu, requested size %zu\n", pool->chunk_size, size);
        return NULL;
    }
    return jdm_alloc_pool_alloc(alloc);
}

static void pool_free(jdm_alloc_t* alloc, void* ptr) {
    if (!alloc || !alloc->internal_data) {
        return;
    }
    if (alloc->type != JDM_ALLOC_POOL) {
        fprintf(stderr, "Invalid allocator type for jdm_alloc_pool_free\n");
        return;
    }
    jdm_pool_t* pool = (jdm_pool_t*)alloc->internal_data;
    jdm_pool_free_node_t* node = (jdm_pool_free_node_t*)ptr;
    node->next = pool->head;
    pool->head = node;
}

static void* pool_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    UNUSED(alloc);
    UNUSED(ptr);
    UNUSED(size);
    fprintf(stderr, "Cannot realloc memory in pool allocator (fixed-size chunks only).\n");
    return NULL;
}

void* jdm_alloc_pool_calloc(jdm_alloc_t* alloc) {
    void* ptr = jdm_alloc_pool_alloc(alloc);
    if (ptr) {
        JDM_ZERO_MEMORY(ptr, ((jdm_pool_t*)alloc->internal_data)->chunk_size);
    }
    return ptr;
}

static void* pool_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    if (num != 1) {
        fprintf(stderr, "Pool allocator can only allocate one chunk at a time, requested %zu chunks\n", num);
        return NULL;
    }
    if (size != ((jdm_pool_t*)alloc->internal_data)->chunk_size) {
        fprintf(stderr, "Pool allocator can only allocate fixed-size chunks of size %zu, requested size %zu\n", ((jdm_pool_t*)alloc->internal_data)->chunk_size, size);
        return NULL;
    }
    return jdm_alloc_pool_calloc(alloc);
}

static jdm_alloc_funcs_t pool_funcs = {
    .alloc = pool_alloc,
    .free = pool_free,
    .realloc = pool_realloc,
    .calloc = pool_calloc
};

void jdm_alloc_pool_free_all(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }
    if (alloc->type != JDM_ALLOC_POOL) {
        fprintf(stderr, "Invalid allocator type for jdm_alloc_pool_free_all\n");
        return;
    }
    jdm_pool_t* pool = (jdm_pool_t*)alloc->internal_data;
    // add each chunk to the free list
    // note we account for the start pointer being offset from the beginning
    // so as to not obliterate the allocator and pool structures at the
    // beginning of the memory region.
    size_t total_chunks = (pool->size - (pool->start - pool->memory)) / pool->chunk_size;
    for (size_t i = 0; i < total_chunks; i++) {
        jdm_pool_free_node_t* node = (jdm_pool_free_node_t*)(pool->start + i * pool->chunk_size);
        node->next = pool->head;
        pool->head = node;
    }
}

jdm_alloc_t* jdm_alloc_create_pool(const jdm_alloc_config_t cfg) {
    size_t capacity = cfg.capacity == 0 ? get_default_capacity() : cfg.capacity; 
    size_t alignment = cfg.alignment == 0 ? get_default_alignment() : cfg.alignment;
    size_t chunk_size = cfg.chunk_size == 0 ? get_default_chunk_size() : cfg.chunk_size;

    // this allocator is designed to allocate fixed-size chunks, but since
    // we want to be able to allocate the allocator itself, we need to make sure that
    // the chunk size is at least as large as the size of a free node, then 
    // we need to start the pool after the allocator and pool structures.

    // We also need to consider that the user is asking for a fixed capacity.
    // We need to make sure that the capacity is large enough to hold the allocator,
    // the pool, AND the capacity.

    // IE if the user requests 1MB, but the allocator and pool structures take up 1KB,
    // then we need to make sure that the capacity is at least 1MB + 1KB, otherwise
    // we will run out of memory when we try to allocate the pool.

    if (chunk_size < sizeof(jdm_pool_free_node_t)) {
        return NULL;
    }
    if (capacity < (chunk_size + 
        JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment) +
        JDM_ALIGN_UP(sizeof(jdm_pool_t), alignment))
    ){
        return NULL;
    }

    size_t capacity_plus_structs = capacity + 
        JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment) +
        JDM_ALIGN_UP(sizeof(jdm_pool_t), alignment);
    capacity_plus_structs = ((capacity_plus_structs / chunk_size) + 1) * chunk_size;

    jdm_alloc_t* alloc = NULL;
    jdm_pool_t* pool = NULL;
    u8* memory = (u8*)malloc(capacity_plus_structs);
    size_t current_offset = 0;

    size_t aligned_size = JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment);
    if (current_offset + aligned_size > capacity_plus_structs) {
        fprintf(stderr, "Pool out of memory: requested %zu bytes, but only %zu bytes available\n", current_offset+aligned_size, capacity_plus_structs);
        return NULL;
    }
    alloc = (jdm_alloc_t*)(memory + current_offset);
    current_offset += aligned_size;

    aligned_size = JDM_ALIGN_UP(sizeof(jdm_pool_t), alignment);
    if (current_offset + aligned_size > capacity_plus_structs) {
        fprintf(stderr, "Pool out of memory: requested %zu bytes, but only %zu bytes available\n", current_offset+aligned_size, capacity_plus_structs);
        return NULL;
    }
    pool = (jdm_pool_t*)(memory + current_offset);
    current_offset += aligned_size;
    
    alloc->type = JDM_ALLOC_POOL;
    JDM_COPY_MEMORY(&alloc->funcs, &pool_funcs, sizeof(jdm_alloc_funcs_t));

    pool->memory = memory;
    // align the start pointer to the next chunk boundary
    pool->start = memory + ((current_offset / chunk_size) + 1) * chunk_size;
    pool->size = capacity_plus_structs;
    pool->chunk_size = chunk_size;
    pool->head = NULL;
    alloc->internal_data = pool;
    jdm_alloc_pool_free_all(alloc);
    return alloc;
}

void jdm_alloc_destroy_pool(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }
    if (alloc->internal_data) {
        jdm_pool_t* pool = (jdm_pool_t*)alloc->internal_data;
        if (pool->memory) {
            free(pool->memory);
        }
    }
}

void jdm_alloc_pool_print_stats(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }
    if (alloc->type != JDM_ALLOC_POOL) {
        fprintf(stderr, "Invalid allocator type for jdm_alloc_pool_print_stats\n");
        return;
    }
    jdm_pool_t* pool = (jdm_pool_t*)alloc->internal_data;
    size_t total_chunks = (pool->size - (pool->start - pool->memory)) / pool->chunk_size;
    size_t free_chunks = 0;
    jdm_pool_free_node_t* current = pool->head;
    while (current) {
        free_chunks++;
        current = current->next;
    }
    printf("Pool Allocator Stats:\n");
    printf("Total Memory: %zu bytes\n", pool->size);
    printf("Reserved Memory: %zu bytes\n", pool->start - pool->memory);
    printf("Used Memory: %zu bytes\n", pool->size - (pool->start - pool->memory) - free_chunks * pool->chunk_size);
    printf("Free Memory: %zu bytes\n", free_chunks * pool->chunk_size);
    printf("Chunk Size: %zu bytes\n", pool->chunk_size);
    printf("Total Chunks: %zu\n", total_chunks);
    printf("Free Chunks: %zu\n", free_chunks);
}
