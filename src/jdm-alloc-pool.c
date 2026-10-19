#include <jdm-log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc-internal.h"
#include "jdm-alloc.h"

typedef struct jdm_pool_free_node_t jdm_pool_free_node_t;
struct jdm_pool_free_node_t {
    jdm_pool_free_node_t* next;  // pointer to next free node
};

typedef struct {
    u8* memory;                  // pointer to memory block
    u8* start;                   // pointer to first chunk
    size_t size;                 // total size of the memory block
    size_t chunk_size;           // size of each chunk
    jdm_pool_free_node_t* head;  // head of free list
} jdm_pool_t;

void* jdm_alloc_pool_alloc(jdm_alloc_t* alloc) {
    if (alloc->type != JDM_ALLOC_POOL) {
        JDM_LOG_ERROR("Invalid allocator type for jdm_alloc_pool_alloc");
        return NULL;
    }
    if (!alloc || !alloc->internal_data) {
        return NULL;
    }
    jdm_pool_t* pool = (jdm_pool_t*)alloc->internal_data;
    if (!pool->head) {
        JDM_LOG_ERROR("Pool out of memory");
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
        JDM_LOG_ERROR(
            "Pool allocator can only allocate fixed-size chunks of size "
            "%zu, requested size %zu",
            pool->chunk_size, size);
        return NULL;
    }
    return jdm_alloc_pool_alloc(alloc);
}

static void pool_free(jdm_alloc_t* alloc, void* ptr) {
    if (!alloc || !alloc->internal_data) {
        return;
    }
    if (alloc->type != JDM_ALLOC_POOL) {
        JDM_LOG_ERROR("Invalid allocator type for jdm_alloc_pool_free");
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
    JDM_LOG_ERROR(
        "Cannot realloc memory in pool allocator (fixed-size chunks only).");
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
        JDM_LOG_ERROR(
            "Pool allocator can only allocate one chunk at a time, "
            "requested %zu chunks",
            num);
        return NULL;
    }
    if (size != ((jdm_pool_t*)alloc->internal_data)->chunk_size) {
        JDM_LOG_ERROR(
            "Pool allocator can only allocate fixed-size chunks of size "
            "%zu, requested size %zu",
            ((jdm_pool_t*)alloc->internal_data)->chunk_size, size);
        return NULL;
    }
    return jdm_alloc_pool_calloc(alloc);
}

static jdm_alloc_stats_t pool_get_stats(jdm_alloc_t* alloc) {
    jdm_alloc_stats_t stats = {0};
    if (!alloc || !alloc->internal_data) {
        return stats;
    }
    if (alloc->type != JDM_ALLOC_POOL) {
        JDM_LOG_ERROR("Invalid allocator type for jdm_alloc_get_stats_pool");
        return stats;
    }
    jdm_pool_t* pool = (jdm_pool_t*)alloc->internal_data;
    stats.type = JDM_ALLOC_POOL;
    stats.pool.pool_capacity = pool->size;
    stats.pool.chunk_capacity =
        (pool->size - (pool->start - pool->memory)) / pool->chunk_size;

    // count the number of free chunks
    size_t free_chunks = 0;
    jdm_pool_free_node_t* node = pool->head;
    while (node) {
        free_chunks++;
        node = node->next;
    }
    stats.pool.chunk_used =
        (pool->size - (pool->start - pool->memory)) / pool->chunk_size -
        free_chunks;
    stats.pool.pool_used = stats.pool.chunk_used * pool->chunk_size;

    return stats;
}

static jdm_alloc_funcs_t pool_funcs = {.alloc = pool_alloc,
                                       .free = pool_free,
                                       .realloc = pool_realloc,
                                       .calloc = pool_calloc,
                                       .get_stats = pool_get_stats};

void jdm_alloc_pool_free_all(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }
    if (alloc->type != JDM_ALLOC_POOL) {
        JDM_LOG_ERROR("Invalid allocator type for jdm_alloc_pool_free_all");
        return;
    }
    jdm_pool_t* pool = (jdm_pool_t*)alloc->internal_data;
    // add each chunk to the free list
    // note we account for the start pointer being offset from the beginning
    // so as to not obliterate the allocator and pool structures at the
    // beginning of the memory region.
    size_t total_chunks =
        (pool->size - (pool->start - pool->memory)) / pool->chunk_size;
    for (size_t i = 0; i < total_chunks; i++) {
        jdm_pool_free_node_t* node =
            (jdm_pool_free_node_t*)(pool->start + i * pool->chunk_size);
        node->next = pool->head;
        pool->head = node;
    }
}

jdm_alloc_t* jdm_alloc_create_pool(const jdm_alloc_config_t cfg) {
    size_t capacity =
        cfg.pool.capacity == 0 ? get_default_capacity() : cfg.pool.capacity;
    size_t alignment =
        cfg.pool.alignment == 0 ? get_default_alignment() : cfg.pool.alignment;
    size_t chunk_size = cfg.pool.chunk_size == 0 ? get_default_chunk_size()
                                                 : cfg.pool.chunk_size;
    JDM_LOG_DEBUG(
        "Creating pool allocator with capacity %zu, alignment %zu, chunk size "
        "%zu",
        capacity, alignment, chunk_size);

    // this allocator is designed to allocate fixed-size chunks, but since
    // we want to be able to allocate the allocator itself, we need to make sure
    // that the chunk size is at least as large as the size of a free node, then
    // we need to start the pool after the allocator and pool structures.

    // We also need to consider that the user is asking for a fixed capacity.
    // We need to make sure that the capacity is large enough to hold the
    // allocator, the pool, AND the capacity.

    // IE if the user requests 1MB, but the allocator and pool structures take
    // up 1KB, then we need to make sure that the capacity is at least 1MB +
    // 1KB, otherwise we will run out of memory when we try to allocate the
    // pool.

    if (chunk_size < sizeof(jdm_pool_free_node_t)) {
        JDM_LOG_ERROR(
            "Pool chunk size must be at least %zu bytes to hold a free node",
            sizeof(jdm_pool_free_node_t));
        return NULL;
    }
    if (capacity < (chunk_size + JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment) +
                    JDM_ALIGN_UP(sizeof(jdm_pool_t), alignment))) {
        JDM_LOG_ERROR(
            "Pool capacity must be at least %zu bytes to hold the "
            "allocator, pool, and at least one chunk",
            chunk_size + JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment) +
                JDM_ALIGN_UP(sizeof(jdm_pool_t), alignment));
        return NULL;
    }

    size_t capacity_plus_structs =
        capacity + JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment) +
        JDM_ALIGN_UP(sizeof(jdm_pool_t), alignment);
    capacity_plus_structs =
        ((capacity_plus_structs / chunk_size) + 1) * chunk_size;

    jdm_alloc_t* alloc = NULL;
    jdm_pool_t* pool = NULL;
    u8* memory = (u8*)malloc(capacity_plus_structs);
    size_t current_offset = 0;

    size_t aligned_size = JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment);
    if (current_offset + aligned_size > capacity_plus_structs) {
        JDM_LOG_ERROR(
            "Pool out of memory: requested %zu bytes, but only %zu bytes "
            "available",
            current_offset + aligned_size, capacity_plus_structs);
        return NULL;
    }
    alloc = (jdm_alloc_t*)(memory + current_offset);
    current_offset += aligned_size;

    aligned_size = JDM_ALIGN_UP(sizeof(jdm_pool_t), alignment);
    if (current_offset + aligned_size > capacity_plus_structs) {
        JDM_LOG_ERROR(
            "Pool out of memory: requested %zu bytes, but only %zu bytes "
            "available",
            current_offset + aligned_size, capacity_plus_structs);
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
