#ifndef JDM_ALLOC_POOL_H
#define JDM_ALLOC_POOL_H
#include "jdm-alloc.h"

void jdm_alloc_pool_free_all(jdm_alloc_t* alloc);
void* jdm_alloc_pool_alloc(jdm_alloc_t* alloc);
void* jdm_alloc_pool_calloc(jdm_alloc_t* alloc);
jdm_alloc_t* jdm_alloc_create_pool(jdm_alloc_config_t cfg);
void jdm_alloc_destroy_pool(jdm_alloc_t* alloc);
jdm_alloc_stats_t jdm_alloc_get_stats_pool(jdm_alloc_t* alloc);

#endif  // JDM_ALLOC_POOL_H
