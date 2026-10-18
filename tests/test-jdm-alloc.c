#include <jdm-log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc.h"

static void* custom_alloc(jdm_alloc_t* alloc, size_t size) {
    (void)alloc;  // suppress unused parameter warning
    return malloc(size);
}

static void custom_free(jdm_alloc_t* alloc, void* ptr) {
    (void)alloc;  // suppress unused parameter warning
    free(ptr);
}

static void* custom_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    (void)alloc;  // suppress unused parameter warning
    return realloc(ptr, size);
}

static void* custom_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    (void)alloc;  // suppress unused parameter warning
    return calloc(num, size);
}

static int test_custom_allocator(void) {
    jdm_alloc_funcs_t custom_funcs = {.alloc = custom_alloc,
                                      .free = custom_free,
                                      .realloc = custom_realloc,
                                      .calloc = custom_calloc};
    jdm_alloc_config_t cfg = {
        .type = JDM_ALLOC_CUSTOM,
        .custom = {.funcs = &custom_funcs, .user_data = NULL}};
    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        JDM_LOG_ERROR("Failed to create custom jdm_alloc");
        return EXIT_FAILURE;
    }

    // Allocate a block of memory
    size_t size = 1024;
    void* ptr = jdm_alloc(alloc, size);
    if (!ptr) {
        JDM_LOG_ERROR("Failed to allocate memory using custom jdm_alloc");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    // Use the allocated memory
    memset(ptr, 0, size);

    // Free the allocated memory
    jdm_free(alloc, ptr);

    // Destroy the allocator
    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    jdm_log_init();
    jdm_log_enable_thread_id(JDM_LOG_DISABLE_THREAD_ID);
    if (test_custom_allocator() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Custom allocator test failed");
        return EXIT_FAILURE;
    }
    JDM_LOG_INFO("Custom allocator test passed");
    jdm_log_cleanup();
    return EXIT_SUCCESS;
}
