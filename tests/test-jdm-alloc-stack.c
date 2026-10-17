#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc-stack.h"
#include "jdm-alloc.h"

static int test_stack_allocator(void) {
    jdm_alloc_config_t cfg = {.type = JDM_ALLOC_STACK};
    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        fprintf(stderr, "Failed to create stack\n");
        return EXIT_FAILURE;
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

static int test_stack_stats(void) {
    size_t alignment = sizeof(void*);
    size_t capacity = 4096;
    jdm_alloc_config_t cfg = {
        .type = JDM_ALLOC_STACK,
        .stack = {.capacity = capacity},
    };
    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        fprintf(stderr, "Failed to create stack\n");
        return EXIT_FAILURE;
    }

    // Get stats before allocation
    jdm_alloc_stats_t stats1 = jdm_alloc_get_stats(alloc);
    if (stats1.type != JDM_ALLOC_STACK) {
        fprintf(stderr, "Stats type is not stack\n");
        return EXIT_FAILURE;
    }
    if (stats1.stack.stack_used != 0) {
        fprintf(stderr, "Stats used is not zero before allocation\n");
        return EXIT_FAILURE;
    }
    if (stats1.stack.stack_capacity != capacity) {
        fprintf(stderr, "Stats capacity does not match expected capacity\n");
        return EXIT_FAILURE;
    }

    size_t size = 1024;
    void* ptr = jdm_alloc(alloc, size);
    if (!ptr) {
        fprintf(stderr, "Failed to alloc in stack\n");
        return EXIT_FAILURE;
    }

    jdm_alloc_stats_t stats2 = jdm_alloc_get_stats(alloc);
    if (stats2.type != JDM_ALLOC_STACK) {
        fprintf(stderr, "Stats type is not stack\n");
        return EXIT_FAILURE;
    }

    // The stack used should be equal to the size allocated plus any padding
    size_t expected_used = stats1.stack.stack_used + size;
    if (stats2.stack.stack_used < expected_used) {
        fprintf(stderr, "Stats used does not match allocated size\n");
        return EXIT_FAILURE;
    }
    if (stats2.stack.stack_capacity != capacity) {
        fprintf(stderr, "Stats capacity does not match expected capacity\n");
        return EXIT_FAILURE;
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}


int main(int argc, char** argv) {
    if (test_stack_allocator() != EXIT_SUCCESS) {
        fprintf(stderr, "Stack allocator test failed\n");
        return EXIT_FAILURE;
    }
    if (test_stack_stats() != EXIT_SUCCESS) {
        fprintf(stderr, "Stack stats test failed\n");
        return EXIT_FAILURE;
    }
    printf("Stack allocator test passed\n");
    return EXIT_SUCCESS;
}
