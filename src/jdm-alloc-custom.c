#include <stdlib.h>
#include <string.h>
#include "jdm-alloc.h"
#include "jdm-alloc-internal.h"

jdm_alloc_t* jdm_alloc_create_custom(jdm_alloc_config_t cfg) {
    if (!cfg.funcs) {
        return NULL; // custom allocator requires function pointers
    }

    jdm_alloc_t* alloc = (jdm_alloc_t*)malloc(sizeof(jdm_alloc_t));
    if (!alloc) {
        return NULL; // allocation failed
    }
    alloc->type = JDM_ALLOC_CUSTOM;
    JDM_COPY_MEMORY(&alloc->funcs, cfg.funcs, sizeof(jdm_alloc_funcs_t));
    alloc->internal_data = cfg.user_data; // store user-defined data for the allocator
    return alloc;
}

void jdm_alloc_destroy_custom(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }
    free(alloc);
}
