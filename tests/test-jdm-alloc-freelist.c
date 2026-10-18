#include <jdm-log.h>
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
        JDM_LOG_ERROR("Failed to create freelist allocator");
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
        JDM_LOG_ERROR("Failed to create freelist allocator");
        return EXIT_FAILURE;
    }

    void* ptr1 = jdm_alloc(alloc, 64);
    if (!ptr1) {
        JDM_LOG_ERROR("Failed to allocate memory from freelist");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    void* ptr2 = jdm_alloc(alloc, 128);
    if (!ptr2) {
        JDM_LOG_ERROR("Failed to allocate memory from freelist");
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
        JDM_LOG_ERROR("Failed to create freelist allocator");
        return EXIT_FAILURE;
    }

    void* ptr = jdm_alloc(alloc, 64);
    if (!ptr) {
        JDM_LOG_ERROR("Failed to allocate memory from freelist");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    void* new_ptr = jdm_realloc(alloc, ptr, 128);
    if (!new_ptr) {
        JDM_LOG_ERROR("Failed to reallocate memory from freelist");
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
        JDM_LOG_ERROR("Failed to create freelist allocator");
        return EXIT_FAILURE;
    }

    jdm_alloc_stats_t before_stats = jdm_alloc_get_stats(alloc);
    if (before_stats.type != JDM_ALLOC_FREELIST) {
        JDM_LOG_ERROR("Stats type is not freelist");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }
    if (before_stats.freelist.freelist_used != 0) {
        JDM_LOG_ERROR("Freelist allocator used memory should be 0");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }
    if (before_stats.freelist.freelist_capacity != capacity) {
        JDM_LOG_ERROR("Freelist allocator capacity does not match");
        JDM_LOG_ERROR("Expected: %zu, Got: %zu", capacity,
                      before_stats.freelist.freelist_capacity);
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    jdm_log_init();
    jdm_log_enable_thread_id(JDM_LOG_DISABLE_THREAD_ID);
    if (test_freelist_allocator() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Freelist allocator test failed");
        return EXIT_FAILURE;
    }
    JDM_LOG_INFO("Freelist allocator test passed");
    if (test_freelist_allocator_alloc_free() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Freelist allocator alloc/free test failed");
        return EXIT_FAILURE;
    }
    JDM_LOG_INFO("Freelist allocator free test passed");
    if (test_freelist_allocator_realloc() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Freelist allocator realloc test failed");
        return EXIT_FAILURE;
    }
    JDM_LOG_INFO("Freelist allocator realloc test passed");
    if (test_freelist_allocator_stats() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Freelist allocator stats test failed");
        return EXIT_FAILURE;
    }
    JDM_LOG_INFO("Freelist allocator stats test passed");
    jdm_log_cleanup();
    return EXIT_SUCCESS;
}
