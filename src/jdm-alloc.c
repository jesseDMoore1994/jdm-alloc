#include "jdm-alloc.h"

#include <stdio.h>

#include "jdm-alloc-system.h"
#include "jdm-alloc-arena.h"
#include "jdm-alloc-custom.h"
#include "jdm-alloc-freelist.h"
#include "jdm-alloc-internal.h"
#include "jdm-alloc-pool.h"
#include "jdm-alloc-stack.h"
#include "jdm-alloc-unimplemented.h"

jdm_alloc_t* jdm_alloc_create(const jdm_alloc_config_t cfg) {
    jdm_alloc_t* alloc = NULL;
    switch (cfg.type) {
        case JDM_ALLOC_SYSTEM:
            alloc = jdm_alloc_create_system(cfg);
            break;
        case JDM_ALLOC_ARENA:
            alloc = jdm_alloc_create_arena(cfg);
            break;
        case JDM_ALLOC_STACK:
            alloc = jdm_alloc_create_stack(cfg);
            break;
        case JDM_ALLOC_POOL:
            alloc = jdm_alloc_create_pool(cfg);
            break;
        case JDM_ALLOC_FREELIST:
            alloc = jdm_alloc_create_freelist(cfg);
            break;
        case JDM_ALLOC_CUSTOM:
            alloc = jdm_alloc_create_custom(cfg);
            break;
        default:
            break;
    }

    if (alloc == NULL) return alloc;

    // Now that allocator is built, init mutex if needed
    alloc->thread_safe = cfg.thread_safe;
    if(alloc->thread_safe) {
        pthread_mutex_init(&(alloc->m), NULL);
    }

    return alloc;
}

void jdm_alloc_destroy(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }

    if (alloc->thread_safe) {
        pthread_mutex_destroy(&(alloc->m));
    }

    switch (alloc->type) {
        case JDM_ALLOC_SYSTEM:
            jdm_alloc_destroy_arena(alloc);
            break;
        case JDM_ALLOC_ARENA:
            jdm_alloc_destroy_arena(alloc);
            break;
        case JDM_ALLOC_STACK:
            jdm_alloc_destroy_stack(alloc);
            break;
        case JDM_ALLOC_POOL:
            jdm_alloc_destroy_pool(alloc);
            break;
        case JDM_ALLOC_FREELIST:
            jdm_alloc_destroy_freelist(alloc);
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
    if(alloc->thread_safe) {
        pthread_mutex_lock(&(alloc->m));
    }

    void* ptr = alloc->funcs.alloc(alloc, size);
    if (!ptr) {
        return NULL;
    }

    if(alloc->thread_safe) {
        pthread_mutex_unlock(&(alloc->m));
    }

    return ptr;
}

void jdm_free(jdm_alloc_t* alloc, void* ptr) {
    if(alloc->thread_safe) {
        pthread_mutex_lock(&(alloc->m));
    }

    alloc->funcs.free(alloc, ptr);

    if(alloc->thread_safe) {
        pthread_mutex_unlock(&(alloc->m));
    }
}

void* jdm_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    if(alloc->thread_safe) {
        pthread_mutex_lock(&(alloc->m));
    }

    void* new_ptr = alloc->funcs.realloc(alloc, ptr, size);
    if (!new_ptr) {
        return NULL;
    }

    if(alloc->thread_safe) {
        pthread_mutex_unlock(&(alloc->m));
    }
    return new_ptr;
}

void* jdm_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    if(alloc->thread_safe) {
        pthread_mutex_lock(&(alloc->m));
    }

    void* ptr = alloc->funcs.calloc(alloc, num, size);
    if (!ptr) {
        return NULL;
    }

    if(alloc->thread_safe) {
        pthread_mutex_unlock(&(alloc->m));
    }
    return ptr;
}

jdm_alloc_stats_t jdm_alloc_get_stats(jdm_alloc_t* alloc) {
    if(alloc->thread_safe) {
        pthread_mutex_lock(&(alloc->m));
    }

    return alloc->funcs.get_stats(alloc);

    if(alloc->thread_safe) {
        pthread_mutex_unlock(&(alloc->m));
    }
}
