#include "jdm-alloc.h"
#include "jdm-alloc-internal.h"
#include "jdm-alloc-default.h"
#include "jdm-alloc-arena.h"
#include "jdm-alloc-stack.h"
#include "jdm-alloc-custom.h"
#include "jdm-alloc-unimplemented.h"

jdm_alloc_t* jdm_alloc_create(const jdm_alloc_config_t cfg) {
    if (cfg.type == JDM_ALLOC_CUSTOM && !cfg.funcs) {
        return NULL; // custom allocator requires function pointers
    }
    if(cfg.type != JDM_ALLOC_CUSTOM && cfg.funcs) {
        return NULL; // non-custom allocator should not have function pointers
    }

    switch (cfg.type) {
        case JDM_ALLOC_DEFAULT:
            return jdm_alloc_create_default(cfg);
        case JDM_ALLOC_ARENA:
            return jdm_alloc_create_arena(cfg);
        case JDM_ALLOC_STACK:
            return jdm_alloc_create_stack(cfg);
        case JDM_ALLOC_POOL:
        case JDM_ALLOC_FREE_LIST:
            return jdm_alloc_create_unimplemented(cfg);
        case JDM_ALLOC_CUSTOM:
            return jdm_alloc_create_custom(cfg);
        default:
            return NULL; // invalid type
    }
}

void jdm_alloc_destroy(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }

    switch (alloc->type) {
        case JDM_ALLOC_DEFAULT:
            jdm_alloc_destroy_default(alloc);
            break;
        case JDM_ALLOC_ARENA:
            jdm_alloc_destroy_arena(alloc);
            break;
        case JDM_ALLOC_STACK:
            jdm_alloc_destroy_stack(alloc);
        case JDM_ALLOC_POOL:
        case JDM_ALLOC_FREE_LIST:
            jdm_alloc_destroy_unimplemented(alloc);
            break;
        case JDM_ALLOC_CUSTOM:
            jdm_alloc_destroy_custom(alloc);
            break;
        default:
            // invalid type, do nothing
            break;
    }
}

void* jdm_alloc(jdm_alloc_t* alloc, size_t size) {
    void* ptr = alloc->funcs.alloc(alloc, size);
    if (!ptr) {
        return NULL;
    }

    return ptr;
}

void jdm_free(jdm_alloc_t* alloc, void* ptr) {
    alloc->funcs.free(alloc, ptr);
}

void* jdm_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    void* new_ptr = alloc->funcs.realloc(alloc, ptr, size);
    if (!new_ptr) {
        return NULL;
    }

    return new_ptr;
}

void* jdm_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    void* ptr = alloc->funcs.calloc(alloc, num, size);
    if (!ptr) {
        return NULL;
    }

    return ptr;
}
