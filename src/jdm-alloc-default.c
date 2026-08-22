#include <stdlib.h>
#include <string.h>
#include "jdm-alloc.h"
#include "jdm-alloc-internal.h"

static void* default_alloc(jdm_alloc_t* alloc, size_t size) {
    UNUSED(alloc);
    return malloc(size);
}

static void default_free(jdm_alloc_t* alloc, void* ptr) {
    UNUSED(alloc);
    free(ptr);
}

static void* default_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    UNUSED(alloc);
    return realloc(ptr, size);
}

static void* default_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    UNUSED(alloc);
    return calloc(num, size);
}

static jdm_alloc_funcs_t default_funcs = {
    .alloc = default_alloc,
    .free = default_free,
    .realloc = default_realloc,
    .calloc = default_calloc
};

jdm_alloc_t* jdm_alloc_create_default() {
    jdm_alloc_t* alloc = (jdm_alloc_t*)malloc(sizeof(jdm_alloc_t));
    if (!alloc) {
        return NULL; // allocation failed
    }
    alloc->type = JDM_ALLOC_DEFAULT;
    JDM_COPY_MEMORY(&alloc->funcs, &default_funcs, sizeof(jdm_alloc_funcs_t));
    alloc->internal_data = NULL; // no internal data for default allocator
    return alloc;
}

void jdm_alloc_destroy_default(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }
    free(alloc);
}
