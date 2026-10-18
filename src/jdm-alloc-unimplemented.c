#include <jdm-log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc-internal.h"
#include "jdm-alloc.h"

static void* unimplemented_alloc(jdm_alloc_t* alloc, size_t size) {
    UNUSED(alloc);
    UNUSED(size);
    JDM_LOG_ERROR(
        "Memory allocation function not implemented for this allocator type");
    return NULL;
}

static void unimplemented_free(jdm_alloc_t* alloc, void* ptr) {
    UNUSED(alloc);
    UNUSED(ptr);
    JDM_LOG_ERROR(
        "Memory free function not implemented for this allocator type");
}

static void* unimplemented_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    UNUSED(alloc);
    UNUSED(ptr);
    UNUSED(size);
    JDM_LOG_ERROR(
        "Memory realloc function not implemented for this allocator type");
    return NULL;
}

static void* unimplemented_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    UNUSED(alloc);
    UNUSED(num);
    UNUSED(size);
    JDM_LOG_ERROR(
        "Memory calloc function not implemented for this allocator type");
    return NULL;
}

static jdm_alloc_funcs_t unimplemented_funcs = {
    .alloc = unimplemented_alloc,
    .free = unimplemented_free,
    .realloc = unimplemented_realloc,
    .calloc = unimplemented_calloc};

jdm_alloc_t* jdm_alloc_create_unimplemented() {
    JDM_LOG_ERROR("Allocator type not implemented");
    return NULL;
}

void jdm_alloc_destroy_unimplemented(jdm_alloc_t* alloc) {
    JDM_LOG_ERROR("Allocator type not implemented");
}

jdm_alloc_stats_t jdm_alloc_get_stats_unimplemented(jdm_alloc_t* alloc) {
    UNUSED(alloc);
    JDM_LOG_ERROR("Allocator type not implemented");
    jdm_alloc_stats_t stats = {0};
    return stats;
}
