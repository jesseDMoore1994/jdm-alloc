#ifndef JDM_ALLOC_INTERNAL_H
#define JDM_ALLOC_INTERNAL_H
#include "jdm-alloc.h"
#include <stddef.h>
#include <stdint.h>

// Create an object to hold the memory allocation functions
struct jdm_alloc_t {
    jdm_alloc_type_t type;
    jdm_alloc_funcs_t funcs;
    // The following pointer is used to hold internal data for the allocator
    // For predefined types, this holds the internal data for the allocator
    // For custom types, this is a pointer to the user-defined data for the allocator
    pointer internal_data;
};


static inline size_t get_default_alignment() {
    return sizeof(pointer);
};

static inline size_t get_default_capacity() {
    return 1024 * 1024; // 1 MB default arena size
};

#endif // JDM_ALLOC_INTERNAL_H
