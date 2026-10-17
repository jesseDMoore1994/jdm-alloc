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
        fprintf(stderr, "Failed to create pool\n");
        return EXIT_FAILURE;
    }

    void* chunks[num_chunks];
    for (size_t i = 0; i < num_chunks; i++) {
        chunks[i] = jdm_alloc(alloc, chunk_size);
        if (!chunks[i]) {
            fprintf(stderr, "Failed to allocate chunk %zu\n", i);
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

int main(int argc, char** argv) {
    if (test_pool_allocator() != EXIT_SUCCESS) {
        fprintf(stderr, "Pool allocator test failed\n");
        return EXIT_FAILURE;
    }
    printf("Pool allocator test passed\n");
    return EXIT_SUCCESS;
}
