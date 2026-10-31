#include <jdm-log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc-internal.h"
#include "jdm-alloc.h"

static void* system_alloc(jdm_alloc_t* alloc, size_t size) {
    UNUSED(alloc);
    return malloc(size);
}

static void system_free(jdm_alloc_t* alloc, void* ptr) {
    UNUSED(alloc);
    free(ptr);
}

static void* system_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    UNUSED(alloc);
    return realloc(ptr, size);
}

static void* system_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    UNUSED(alloc);
    return calloc(num, size);
}

static jdm_alloc_funcs_t system_funcs = {
    .alloc = system_alloc,
    .free = system_free,
    .realloc = system_realloc,
    .calloc = system_calloc};

jdm_alloc_t* jdm_alloc_create_system(void) {
    jdm_alloc_t* alloc = malloc(sizeof(jdm_alloc_t));
    if (alloc == NULL) return NULL;
    alloc->type = JDM_ALLOC_SYSTEM;
    JDM_COPY_MEMORY(&alloc->funcs, &system_funcs, sizeof(jdm_alloc_funcs_t));
    alloc->internal_data = NULL;
    return alloc;
}

void jdm_alloc_destroy_system(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }
    free(alloc);
}

jdm_alloc_stats_t jdm_alloc_get_stats_system(jdm_alloc_t* alloc) {
    UNUSED(alloc);
    JDM_LOG_ERROR("Stats not available for system allocator.");
    jdm_alloc_stats_t stats = {0};
    return stats;
}
