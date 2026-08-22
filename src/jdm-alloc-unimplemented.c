#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jdm-alloc.h"
#include "jdm-alloc-internal.h"

static void* unimplemented_alloc(jdm_alloc_t* alloc, size_t size) {
    UNUSED(alloc);
    UNUSED(size);
    fprintf(stderr, "Memory allocation function not implemented for this allocator type\n");
    return NULL;
}

static void unimplemented_free(jdm_alloc_t* alloc, void* ptr) {
    UNUSED(alloc);
    UNUSED(ptr);
    fprintf(stderr, "Memory free function not implemented for this allocator type\n");
}

static void* unimplemented_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    UNUSED(alloc);
    UNUSED(ptr);
    UNUSED(size);
    fprintf(stderr, "Memory realloc function not implemented for this allocator type\n");
    return NULL;
}

static void* unimplemented_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    UNUSED(alloc);
    UNUSED(num);
    UNUSED(size);
    fprintf(stderr, "Memory calloc function not implemented for this allocator type\n");
    return NULL;
}

static jdm_alloc_funcs_t unimplemented_funcs = {
    .alloc = unimplemented_alloc,
    .free = unimplemented_free,
    .realloc = unimplemented_realloc,
    .calloc = unimplemented_calloc
};

jdm_alloc_t* jdm_alloc_create_unimplemented() {
    fprintf(stderr, "Allocator type not implemented\n");
    return NULL;
}

void jdm_alloc_destroy_unimplemented(jdm_alloc_t* alloc) {
    fprintf(stderr, "Allocator type not implemented\n");
}
