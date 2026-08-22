#ifndef JDM_ALLOC_ARENA_H
#define JDM_ALLOC_ARENA_H
#include "jdm-alloc.h"

// holds a previous reference that can be saved and restored
typedef struct jdm_alloc_arena_checkpoint_t {
    size_t curr_offset;
    size_t prev_offset;
} jdm_alloc_arena_checkpoint_t;

jdm_alloc_t* jdm_alloc_create_arena(const jdm_alloc_config_t cfg);
void jdm_alloc_destroy_arena(jdm_alloc_t* alloc);
uint8_t* jdm_alloc_arena_get_base_address(jdm_alloc_t* alloc);
size_t jdm_alloc_arena_get_current_offset(jdm_alloc_t* alloc);
size_t jdm_alloc_arena_get_previous_offset(jdm_alloc_t* alloc);

jdm_alloc_arena_checkpoint_t jdm_alloc_arena_create_checkpoint(jdm_alloc_t* alloc);
void jdm_alloc_arena_restore_checkpoint(jdm_alloc_t* alloc, jdm_alloc_arena_checkpoint_t checkpoint);

#endif // JDM_ALLOC_ARENA_H
