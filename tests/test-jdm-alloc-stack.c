#include <jdm-log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc-stack.h"
#include "jdm-alloc.h"

static int test_stack_allocator(void) {
    jdm_alloc_config_t cfg = {.type = JDM_ALLOC_STACK};
    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        JDM_LOG_ERROR("Failed to create stack allocator");
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
        JDM_LOG_ERROR("Failed to create stack allocator");
        return EXIT_FAILURE;
    }

    // Get stats before allocation
    jdm_alloc_stats_t stats1 = jdm_alloc_get_stats(alloc);
    if (stats1.type != JDM_ALLOC_STACK) {
        JDM_LOG_ERROR("Stats type is not stack");
        return EXIT_FAILURE;
    }
    if (stats1.stack.stack_used != 0) {
        JDM_LOG_ERROR("Stats used is not zero before allocation");
        return EXIT_FAILURE;
    }
    if (stats1.stack.stack_capacity != capacity) {
        JDM_LOG_ERROR("Stats capacity does not match expected capacity");
        return EXIT_FAILURE;
    }

    size_t size = 1024;
    void* ptr = jdm_alloc(alloc, size);
    if (!ptr) {
        JDM_LOG_ERROR("Failed to alloc in stack");
        return EXIT_FAILURE;
    }

    jdm_alloc_stats_t stats2 = jdm_alloc_get_stats(alloc);
    if (stats2.type != JDM_ALLOC_STACK) {
        JDM_LOG_ERROR("Stats type is not stack after allocation");
        return EXIT_FAILURE;
    }

    // The stack used should be equal to the size allocated plus any padding
    size_t expected_used = stats1.stack.stack_used + size;
    if (stats2.stack.stack_used < expected_used) {
        JDM_LOG_ERROR("Stats used is less than expected after allocation");
        return EXIT_FAILURE;
    }
    if (stats2.stack.stack_capacity != capacity) {
        JDM_LOG_ERROR(
            "Stats capacity does not match expected capacity after allocation");
        return EXIT_FAILURE;
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    jdm_log_init();
    jdm_log_enable_thread_id(JDM_LOG_DISABLE_THREAD_ID);
    if (test_stack_allocator() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Stack allocator test failed");
        return EXIT_FAILURE;
    }
    if (test_stack_stats() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Stack stats test failed");
        return EXIT_FAILURE;
    }
    JDM_LOG_DEBUG("Stack allocator and stats tests passed");
    jdm_log_cleanup();
    return EXIT_SUCCESS;
}
