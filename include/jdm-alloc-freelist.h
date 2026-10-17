#ifndef JDM_ALLOC_FREELIST_H
#define JDM_ALLOC_FREELIST_H
#include "jdm-alloc.h"

jdm_alloc_t* jdm_alloc_create_freelist(jdm_alloc_config_t cfg);
void jdm_alloc_destroy_freelist(jdm_alloc_t* alloc);
jdm_alloc_stats_t jdm_alloc_get_stats_freelist(jdm_alloc_t* alloc);

#endif  // JDM_ALLOC_FREELIST_H
