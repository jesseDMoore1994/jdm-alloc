#ifndef JDM_ALLOC_STACK_H
#define JDM_ALLOC_STACK_H
#include "jdm-alloc.h"

jdm_alloc_t* jdm_alloc_create_stack(const jdm_alloc_config_t cfg);
void jdm_alloc_destroy_stack(jdm_alloc_t* alloc);
jdm_alloc_stats_t jdm_alloc_get_stats_stack(jdm_alloc_t* alloc);

#endif  // JDM_ALLOC_STACK_H
