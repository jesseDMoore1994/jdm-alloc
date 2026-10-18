#include <jdm-log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc-pool.h"
#include "jdm-alloc.h"

static int test_pool_allocator(void) {
    size_t chunk_size = 64;
    size_t num_chunks = 10;
    size_t total_size = chunk_size * num_chunks;
    jdm_alloc_config_t cfg = {.type = JDM_ALLOC_POOL,
                              .pool = {
                                  .capacity = total_size,
                                  .chunk_size = chunk_size,
                              }};

    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        JDM_LOG_ERROR("Failed to create pool allocator");
        return EXIT_FAILURE;
    }

    void* chunks[num_chunks];
    for (size_t i = 0; i < num_chunks; i++) {
        chunks[i] = jdm_alloc(alloc, chunk_size);
        if (!chunks[i]) {
            JDM_LOG_ERROR("Failed to allocate chunk %zu", i);
            jdm_alloc_destroy(alloc);
            return EXIT_FAILURE;
        }
        memset(chunks[i], (int)i, chunk_size);  // Fill with identifiable data
    }

    // Free all chunks
    for (size_t i = 0; i < num_chunks; i++) {
        jdm_free(alloc, chunks[i]);
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

static int test_pool_allocator_stats(void) {
    size_t chunk_size = 64;
    size_t num_chunks = 10;
    size_t total_size = chunk_size * num_chunks;
    jdm_alloc_config_t cfg = {.type = JDM_ALLOC_POOL,
                              .pool = {
                                  .capacity = total_size,
                                  .chunk_size = chunk_size,
                              }};

    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        JDM_LOG_ERROR("Failed to create pool allocator");
        return EXIT_FAILURE;
    }

    jdm_alloc_stats_t stats_before = jdm_alloc_get_stats(alloc);
    if (stats_before.pool.chunk_used != 0) {
        JDM_LOG_ERROR("Expected 0 chunks used before allocation, got %zu",
                      stats_before.pool.chunk_used);
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    void* chunk = jdm_alloc(alloc, chunk_size);
    if (!chunk) {
        JDM_LOG_ERROR("Failed to allocate chunk");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    jdm_alloc_stats_t stats_after = jdm_alloc_get_stats(alloc);
    if (stats_after.pool.chunk_used != 1) {
        JDM_LOG_ERROR("Expected 1 chunk used after allocation, got %zu",
                      stats_after.pool.chunk_used);
        jdm_free(alloc, chunk);
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    jdm_free(alloc, chunk);
    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    jdm_log_init();
    jdm_log_enable_thread_id(JDM_LOG_DISABLE_THREAD_ID);
    if (test_pool_allocator() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Pool allocator test failed");
        return EXIT_FAILURE;
    }
    if (test_pool_allocator_stats() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Pool allocator stats test failed");
        return EXIT_FAILURE;
    }
    JDM_LOG_INFO("Pool allocator tests passed");
    jdm_log_cleanup();
    return EXIT_SUCCESS;
}
