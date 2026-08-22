#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jdm-alloc.h"
#include "jdm-alloc-internal.h"
#include "jdm-alloc-arena.h"

// internal data structure for arena allocator
typedef struct {
    u8* memory; // pointer to the memory block
    size_t size; // total size of the memory block
    size_t curr_offset; // current offset in the memory block
    size_t prev_offset; // previous offset in the memory block
    size_t alignment; // alignment for allocations
} jdm_arena_t;

static void* arena_alloc(jdm_alloc_t* alloc, size_t size) {
    if (!alloc || !alloc->internal_data) {
        return NULL;
    }
    if (alloc->type != JDM_ALLOC_ARENA) {
        fprintf(stderr, "Invalid allocator type for arena_alloc\n");
        return NULL;
    }
    jdm_arena_t* arena = (jdm_arena_t*)alloc->internal_data;
    size_t aligned_size = JDM_ALIGN_UP(size, arena->alignment);
    if (arena->curr_offset + aligned_size > arena->size) {
        fprintf(stderr, "Arena out of memory: requested %zu bytes, but only %zu bytes available\n", aligned_size, arena->size - arena->curr_offset);
        return NULL;
    }
    void* ptr = arena->memory + arena->curr_offset;
    arena->prev_offset = arena->curr_offset;
    arena->curr_offset += aligned_size;
    return ptr;
}

// free operation can only free the last allocated block in the arena,
// otherwise it does nothing. This is a limitation of the arena allocator.
static void arena_free_last(jdm_alloc_t* alloc) {
    if (!alloc || !alloc->internal_data) {
        return;
    }
    jdm_arena_t* arena = (jdm_arena_t*)alloc->internal_data;
    arena->curr_offset = arena->prev_offset;
}

static void arena_free(jdm_alloc_t* alloc, void* ptr) {
    if (!alloc || !alloc->internal_data || !ptr) {
        return;
    }
    if (alloc->type != JDM_ALLOC_ARENA) {
        fprintf(stderr, "Invalid allocator type for arena_free_last\n");
        return;
    }
    jdm_arena_t* arena = (jdm_arena_t*)alloc->internal_data;
    // Check if the pointer is the last allocated block
    if (ptr == arena->memory + arena->prev_offset) {
        arena_free_last(alloc);
    } else {
        fprintf(stderr, "Arena free operation can only free the last allocated block\n");
    }
}

// realloc works on any block in the arena, but it can only grow or shrink the
// last allocated block. If the pointer is not the last allocated block.
static void* arena_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    if (!alloc || !alloc->internal_data || !ptr) {
        return NULL;
    }
    if (alloc->type != JDM_ALLOC_ARENA) {
        fprintf(stderr, "Invalid allocator type for arena_free_last\n");
        return NULL;
    }
    jdm_arena_t* arena = (jdm_arena_t*)alloc->internal_data;
    // Check if the pointer is the last allocated block
    if (ptr != arena->memory + arena->prev_offset) {
        fprintf(stderr, "Arena realloc operation can only resize the last allocated block\n");
        return NULL;
    }
    size_t aligned_size = JDM_ALIGN_UP(size, arena->alignment);
    if (arena->prev_offset + aligned_size > arena->size) {
        fprintf(stderr, "Arena out of memory: requested %zu bytes, but only %zu bytes available\n", aligned_size, arena->size - arena->prev_offset);
        return NULL;
    }
    arena->curr_offset = arena->prev_offset + aligned_size;
    return ptr;
}

static void* arena_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    size_t total_size = num * size;
    void* ptr = arena_alloc(alloc, total_size);
    if (ptr) {
        JDM_ZERO_MEMORY(ptr, total_size);
    }
    return ptr;
}

static jdm_alloc_funcs_t arena_funcs = {
    .alloc = arena_alloc,
    .free = arena_free,
    .realloc = arena_realloc,
    .calloc = arena_calloc
};

jdm_alloc_t* jdm_alloc_create_arena(jdm_alloc_config_t cfg) {
    size_t capacity = cfg.capacity == 0 ? get_default_capacity() : cfg.capacity; 
    size_t alignment = cfg.alignment == 0 ? get_default_alignment() : cfg.alignment;
    jdm_alloc_t* alloc = NULL;
    jdm_arena_t* arena = NULL;
    u8* memory = (u8*)malloc(capacity);
    size_t current_offset = 0;

    size_t aligned_size = JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment);
    if (current_offset + aligned_size > capacity) {
        fprintf(stderr, "Arena out of memory: requested %zu bytes, but only %zu bytes available\n", aligned_size, arena->size - arena->curr_offset);
        return NULL;
    }
    alloc = (jdm_alloc_t*)(memory + current_offset);
    current_offset += aligned_size;

    aligned_size = JDM_ALIGN_UP(sizeof(jdm_arena_t), alignment);
    if (current_offset + aligned_size > capacity) {
        fprintf(stderr, "Arena out of memory: requested %zu bytes, but only %zu bytes available\n", aligned_size, arena->size - arena->curr_offset);
        return NULL;
    }
    arena = (jdm_arena_t*)(memory + current_offset);
    current_offset += aligned_size;
    
    alloc->type = JDM_ALLOC_ARENA;
    JDM_COPY_MEMORY(&alloc->funcs, &arena_funcs, sizeof(jdm_alloc_funcs_t));
    arena->size = capacity;
    arena->memory = memory;
    arena->curr_offset = current_offset;
    arena->prev_offset = current_offset;
    arena->alignment = alignment;
    alloc->internal_data = arena;
    return alloc;
}

void jdm_alloc_destroy_arena(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }
    if (alloc->internal_data) {
        jdm_arena_t* arena = (jdm_arena_t*)alloc->internal_data;
        if (arena->memory) {
            free(arena->memory);
        }
    }
}

uint8_t* jdm_alloc_arena_get_base_address(jdm_alloc_t* alloc) {
    if (!alloc || !alloc->internal_data) {
        return 0;
    }
    if (alloc->type != JDM_ALLOC_ARENA) {
        fprintf(stderr, "Invalid allocator type for jdm_alloc_arena_get_base_address\n");
        return 0;
    }
    jdm_arena_t* arena = (jdm_arena_t*)alloc->internal_data;
    return arena->memory;
}

size_t jdm_alloc_arena_get_current_offset(jdm_alloc_t* alloc) {
    if (!alloc || !alloc->internal_data) {
        return 0;
    }
    if (alloc->type != JDM_ALLOC_ARENA) {
        fprintf(stderr, "Invalid allocator type for jdm_alloc_arena_get_current_offset\n");
        return 0;
    }
    jdm_arena_t* arena = (jdm_arena_t*)alloc->internal_data;
    return arena->curr_offset;
}

size_t jdm_alloc_arena_get_previous_offset(jdm_alloc_t* alloc) {
    if (!alloc || !alloc->internal_data) {
        return 0;
    }
    if (alloc->type != JDM_ALLOC_ARENA) {
        fprintf(stderr, "Invalid allocator type for jdm_alloc_arena_get_previous_offset\n");
        return 0;
    }
    jdm_arena_t* arena = (jdm_arena_t*)alloc->internal_data;
    return arena->prev_offset;
}

jdm_alloc_arena_checkpoint_t jdm_alloc_arena_create_checkpoint(jdm_alloc_t* alloc) {
    if (!alloc || !alloc->internal_data) {
        return (jdm_alloc_arena_checkpoint_t) {
            .curr_offset = 0,
            .prev_offset = 0,
        };
    }
    if (alloc->type != JDM_ALLOC_ARENA) {
        fprintf(stderr, "Invalid allocator type for jdm_alloc_arena_get_previous_offset\n");
        return (jdm_alloc_arena_checkpoint_t) {
            .curr_offset = 0,
            .prev_offset = 0,
        };
    }
    jdm_arena_t* arena = (jdm_arena_t*)alloc->internal_data;
    return (jdm_alloc_arena_checkpoint_t) {
        .curr_offset = arena->curr_offset,
        .prev_offset = arena->prev_offset,
    };
}

void jdm_alloc_arena_restore_checkpoint(jdm_alloc_t* alloc, jdm_alloc_arena_checkpoint_t checkpoint) {
    if (!alloc || !alloc->internal_data) {
        return;
    }
    if (alloc->type != JDM_ALLOC_ARENA) {
        fprintf(stderr, "Invalid allocator type for jdm_alloc_arena_get_previous_offset\n");
        return;
    }
    jdm_arena_t* arena = (jdm_arena_t*)alloc->internal_data;
    arena->curr_offset = checkpoint.curr_offset;
    arena->prev_offset = checkpoint.prev_offset;
}
