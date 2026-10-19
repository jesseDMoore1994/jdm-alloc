#include <jdm-log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc.h"
#include "jdm-alloc-custom.h"

typedef struct {
    int alloc_called;
    int free_called;
    int realloc_called;
    int calloc_called;
} custom_stats;

static void* custom_alloc(jdm_alloc_t* alloc, size_t size) {
    ((custom_stats*)jdm_alloc_get_user_data_custom(alloc))->alloc_called++;
    return malloc(size);
}

static void custom_free(jdm_alloc_t* alloc, void* ptr) {
    ((custom_stats*)jdm_alloc_get_user_data_custom(alloc))->free_called++;
    free(ptr);
}

static void* custom_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    ((custom_stats*)jdm_alloc_get_user_data_custom(alloc))->realloc_called++;
    return realloc(ptr, size);
}

static void* custom_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    ((custom_stats*)jdm_alloc_get_user_data_custom(alloc))->calloc_called++;
    return calloc(num, size);
}

static jdm_alloc_stats_t custom_get_stats(jdm_alloc_t* alloc) {
    return (jdm_alloc_stats_t) {
        .type = JDM_ALLOC_CUSTOM,
        .custom = jdm_alloc_get_user_data_custom(alloc),
    };
}

typedef struct {
    int thread_id;
    jdm_alloc_t* alloc;
} thread_ctx;

static void* thread_alloc_ops(void *arg) {
    int thread_id = ((thread_ctx*)arg)->thread_id;
    jdm_alloc_t* alloc = ((thread_ctx*)arg)->alloc;
    // Allocate a block of memory
    size_t size = 1024;
    void* ptr = jdm_alloc(alloc, size);
    if (!ptr) {
        JDM_LOG_ERROR("Failed to allocate memory using custom jdm_alloc");
        jdm_alloc_destroy(alloc);
        return NULL;
    }

    // Use the allocated memory
    memset(ptr, 0, size);

    // Free the allocated memory
    jdm_free(alloc, ptr);

    return NULL;
}

#define NUM_THREADS 5
static int test_allocator_threading(void) {
    jdm_alloc_funcs_t custom_funcs = {.alloc = custom_alloc,
                                      .free = custom_free,
                                      .realloc = custom_realloc,
                                      .calloc = custom_calloc,
                                      .get_stats = custom_get_stats};
    custom_stats stats = {0};
    jdm_alloc_config_t cfg = {
        .type = JDM_ALLOC_CUSTOM,
        .thread_safe = JDM_TRUE,
        .custom = {
            .funcs = &custom_funcs,
            .user_data = &stats //store stats in user data
        }
    };
    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        JDM_LOG_ERROR("Failed to create custom jdm_alloc");
        return EXIT_FAILURE;
    }

    pthread_t threads[NUM_THREADS];
    thread_ctx thread_ctxs[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ctxs[i] = (thread_ctx) {
            .thread_id = i,
            .alloc = alloc,
        };
        pthread_create(&threads[i], NULL, thread_alloc_ops, &thread_ctxs[i]);
    }
    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    if (stats.alloc_called != 5) {
        JDM_LOG_ERROR("Stats do not match number of expected allocation calls");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }
    if (stats.free_called != 5) {
        JDM_LOG_ERROR("Stats do not match number of expected free calls");
        jdm_alloc_destroy(alloc);
        return EXIT_FAILURE;
    }

    // Destroy the allocator
    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    jdm_log_init();
    jdm_log_enable_thread_safe(JDM_LOG_ENABLE_THREAD_SAFE);
    if (test_allocator_threading() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("allocator threading test failed");
        return EXIT_FAILURE;
    }
    JDM_LOG_INFO("allocator test passed");
    jdm_log_cleanup();
    return EXIT_SUCCESS;
}
