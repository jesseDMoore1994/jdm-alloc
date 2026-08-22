#ifndef JDM_ALLOC_CUSTOM_H
#define JDM_ALLOC_CUSTOM_H
#include "jdm-alloc.h"

jdm_alloc_t* jdm_alloc_create_custom(const jdm_alloc_config_t cfg);
void jdm_alloc_destroy_custom(jdm_alloc_t* alloc);

#endif // JDM_ALLOC_CUSTOM_H
