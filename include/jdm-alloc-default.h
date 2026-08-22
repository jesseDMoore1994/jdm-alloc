#ifndef JDM_ALLOC_DEFAULT_H
#define JDM_ALLOC_DEFAULT_H
#include "jdm-alloc.h"

jdm_alloc_t* jdm_alloc_create_default(const jdm_alloc_config_t cfg);
void jdm_alloc_destroy_default(jdm_alloc_t* alloc);

#endif // JDM_ALLOC_DEFAULT_H
