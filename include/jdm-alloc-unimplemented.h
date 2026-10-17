#ifndef JDM_ALLOC_UNIMPLEMENTED_H
#define JDM_ALLOC_UNIMPLEMENTED_H
#include "jdm-alloc.h"

jdm_alloc_t* jdm_alloc_create_unimplemented(jdm_alloc_config_t cfg);
void jdm_alloc_destroy_unimplemented(jdm_alloc_t* alloc);
jdm_alloc_stats_t jdm_alloc_get_stats_unimplemented(jdm_alloc_t* alloc);

#endif  // JDM_ALLOC_UNIMPLEMENTED_H
