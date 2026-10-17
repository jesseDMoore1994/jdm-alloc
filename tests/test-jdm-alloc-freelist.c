#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc-freelist.h"
#include "jdm-alloc.h"

static int test_freelist_allocator(void) {
    jdm_alloc_config_t cfg = {
        .type = JDM_ALLOC_FREELIST,
    };

    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        fprintf(stderr, "Failed to create freelist\n");
        return EXIT_FAILURE;
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

static int test_freelist_allocator_alloc_free(void) {
    jdm_alloc_config_t cfg = {
        .type = JDM_ALLOC_FREELIST,
    };

    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        fprintf(stderr, "Failed to create freelist\n");
        return EXIT_FAILURE;
    }

    void* ptr1 = jdm_alloc(alloc, 64);
    if (!ptr1) {
        fprintf(stderr, "Failed to allocate memory from freelist\n");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    void* ptr2 = jdm_alloc(alloc, 128);
    if (!ptr2) {
        fprintf(stderr, "Failed to allocate memory from freelist\n");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    jdm_free(alloc, ptr1);
    jdm_free(alloc, ptr2);

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

static int test_freelist_allocator_realloc(void) {
    jdm_alloc_config_t cfg = {
        .type = JDM_ALLOC_FREELIST,
    };

    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        fprintf(stderr, "Failed to create freelist\n");
        return EXIT_FAILURE;
    }

    void* ptr = jdm_alloc(alloc, 64);
    if (!ptr) {
        fprintf(stderr, "Failed to allocate memory from freelist\n");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    void* new_ptr = jdm_realloc(alloc, ptr, 128);
    if (!new_ptr) {
        fprintf(stderr, "Failed to reallocate memory from freelist\n");
        jdm_free(alloc, ptr);
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    jdm_free(alloc, new_ptr);
    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

static int test_freelist_allocator_stats(void) {
    size_t capacity = 4096;
    jdm_alloc_config_t cfg = {
        .type = JDM_ALLOC_FREELIST,
        .freelist = {.capacity = capacity},
    };

    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        fprintf(stderr, "Failed to create freelist\n");
        return EXIT_FAILURE;
    }

    jdm_alloc_stats_t before_stats = jdm_alloc_get_stats(alloc);
    if (before_stats.type != JDM_ALLOC_FREELIST) {
        fprintf(stderr, "Freelist allocator stats type mismatch\n");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }
    if (before_stats.freelist.freelist_used != 0) {
        fprintf(stderr, "Freelist allocator used memory should be 0\n");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }
    if (before_stats.freelist.freelist_capacity != capacity) {
        fprintf(stderr, "Freelist allocator capacity does not match\n");
        fprintf(stderr, "Expected: %zu, Got: %zu\n", capacity,
                before_stats.freelist.freelist_capacity);
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}


int main(int argc, char** argv) {
    if (test_freelist_allocator() != EXIT_SUCCESS) {
        fprintf(stderr, "Freelist allocator test failed\n");
        return EXIT_FAILURE;
    }
    printf("Freelist allocator test passed\n");
    if (test_freelist_allocator_alloc_free() != EXIT_SUCCESS) {
        fprintf(stderr, "Freelist allocator alloc/free test failed\n");
        return EXIT_FAILURE;
    }
    printf("Freelist allocator free test passed\n");
    if (test_freelist_allocator_realloc() != EXIT_SUCCESS) {
        fprintf(stderr, "Freelist allocator realloc test failed\n");
        return EXIT_FAILURE;
    }
    printf("Freelist allocator realloc test passed\n");
    if (test_freelist_allocator_stats() != EXIT_SUCCESS) {
        fprintf(stderr, "Freelist allocator stats test failed\n");
        return EXIT_FAILURE;
    }
    printf("Freelist allocator stats test passed\n");
    return EXIT_SUCCESS;
}
