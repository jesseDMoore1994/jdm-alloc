#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jdm-alloc.h"
#include "jdm-alloc-internal.h"

// internal data structure for stack allocator
typedef struct {
    u8* memory; // pointer to the memory block
    size_t size; // total size of the memory block
    size_t offset; // current offset in the memory block
    size_t alignment; // alignment for allocations
} jdm_stack_t;

// Use stack header to store extra padding
typedef struct {
    u8 padding; // padding to maintain alignment
    u64 size; //size of the allocation
} jdm_stack_header_t;

size_t calc_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size) {
    uintptr_t p, a, modulo, padding, needed_space;
    
    p = ptr;
    a = alignment;
    modulo = p & (a-1); // (p % a) as it assumes alignment is a power of two
    
    padding = 0;
    needed_space = 0;
    
    if (modulo != 0) { // Same logic as 'align_forward'
    	padding = a - modulo;
    }
    
    needed_space = (uintptr_t)header_size;
    
    if (padding < needed_space) {
    	needed_space -= padding;
    
    	if ((needed_space & (a-1)) != 0) {
    		padding += a * (1+(needed_space/a));
    	} else {
    		padding += a * (needed_space/a);
    	}
    }
    
    return (size_t)padding;
}


static void* stack_alloc(jdm_alloc_t* alloc, size_t size) {
    uintptr_t curr_addr, next_addr;
    size_t padding;
    jdm_stack_header_t *header;
    jdm_stack_t* stack; 


    if (!alloc || !alloc->internal_data) {
        return NULL;
    }
    if (alloc->type != JDM_ALLOC_STACK) {
        fprintf(stderr, "Invalid allocator type for stack_alloc\n");
        return NULL;
    }

    stack = (jdm_stack_t*)alloc->internal_data;
    if (!JDM_IS_POWER_OF_TWO(stack->alignment)) {
        return NULL;
    }
    if (stack->alignment > 128) {
        // As the padding is 8 bits (1 byte), the largest alignment that can
        // be used is 128 bytes
        stack->alignment = 128;
    }

    curr_addr = (uintptr_t)stack->memory + (uintptr_t)stack->offset;
    padding = calc_padding_with_header(curr_addr, (uintptr_t)stack->alignment, sizeof(jdm_stack_header_t));
    if (stack->offset + padding + size > stack->size) {
        // Stack allocator is out of memory
        return NULL;
    }
    stack->offset += padding;

    next_addr = curr_addr + (uintptr_t)padding;
    header = (jdm_stack_header_t *)(next_addr - sizeof(jdm_stack_header_t));
    header->padding = (uint8_t)padding;
    header->size = size;

    stack->offset += size;

    return memset((void *)next_addr, 0, size);
}

static void stack_free(jdm_alloc_t* alloc, void* ptr) {
    if (ptr != NULL) {
        uintptr_t start, end, curr_addr;
        size_t prev_offset;
        jdm_stack_t* stack;
        jdm_stack_header_t* header;

        stack = (jdm_stack_t*)alloc->internal_data;
        start = (uintptr_t)stack->memory;
        end = start + (uintptr_t)stack->size;
        curr_addr = (uintptr_t)ptr;

        if (!alloc || !alloc->internal_data) {
            return;
        }
        if (alloc->type != JDM_ALLOC_STACK) {
            fprintf(stderr, "Invalid allocator type for stack_alloc\n");
            return;
        }
        if (!(start <= curr_addr && curr_addr < end)) {
            fprintf(stderr, "Out of bounds memory address passed to stack allocator (free)\n");
            return;
        }

        if (curr_addr >= start+(uintptr_t)stack->offset) {
            // Allow double frees
            return;
        }

        header = (jdm_stack_header_t *)(curr_addr - sizeof(jdm_stack_header_t));
        prev_offset = (size_t)(curr_addr - (uintptr_t)header->padding - start);

        stack->offset = prev_offset;
    }
}

static void* stack_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    if (ptr == NULL) {
        return stack_alloc(alloc, size);
    } else if (size == 0) {
        stack_free(alloc, ptr);
        return NULL;
    } else {
        jdm_stack_t* stack = (jdm_stack_t*)alloc->internal_data;

        uintptr_t start, end, curr_addr;
        start = (uintptr_t)stack->memory;
        end = start + (uintptr_t)stack->size;
        curr_addr = (uintptr_t)ptr;

        jdm_stack_header_t* header = (jdm_stack_header_t *)(curr_addr - sizeof(jdm_stack_header_t));

        size_t old_size, new_size, min_size;
        old_size = header->size;
        new_size = size;
        min_size = old_size < new_size ? old_size : new_size;

        pointer new_ptr;

        if (!(start <= curr_addr && curr_addr < end)) {
                return NULL;
        }

        if (curr_addr >= start + (uintptr_t)stack->offset) {
                // Treat as a double free
                return NULL;
        }

        if (old_size == size) {
                return ptr;
        }

        new_ptr = stack_alloc(alloc, new_size);
        JDM_MOVE_MEMORY(new_ptr, ptr, min_size);
        return new_ptr;
    }
}

static void* stack_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    size_t total_size = num * size;
    void* ptr = stack_alloc(alloc, total_size);
    if (ptr) {
        JDM_ZERO_MEMORY(ptr, total_size);
    }
}

static jdm_alloc_funcs_t stack_funcs = {
    .alloc = stack_alloc,
    .free = stack_free,
    .realloc = stack_realloc,
    .calloc = stack_calloc
};

jdm_alloc_t* jdm_alloc_create_stack(jdm_alloc_config_t cfg) {
    size_t capacity = cfg.capacity == 0 ? get_default_capacity() : cfg.capacity; 
    size_t alignment = cfg.alignment == 0 ? get_default_alignment() : cfg.alignment;
    jdm_alloc_t* alloc = NULL;
    jdm_stack_t* stack = NULL;
    u8* memory = (u8*)malloc(capacity);
    size_t offset = 0;

    size_t aligned_size = JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment);
    if (offset + aligned_size > capacity) {
        fprintf(stderr, "Stack out of memory: requested %zu bytes, but only %zu bytes available\n", aligned_size, stack->size - stack->offset);
        return NULL;
    }
    alloc = (jdm_alloc_t*)(memory + offset);
    offset += aligned_size;

    aligned_size = JDM_ALIGN_UP(sizeof(jdm_stack_t), alignment);
    if (offset + aligned_size > capacity) {
        fprintf(stderr, "Stack out of memory: requested %zu bytes, but only %zu bytes available\n", aligned_size, stack->size - stack->offset);
        return NULL;
    }
    stack = (jdm_stack_t*)(memory + offset);
    offset += aligned_size;
    
    alloc->type = JDM_ALLOC_STACK;
    JDM_COPY_MEMORY(&alloc->funcs, &stack_funcs, sizeof(jdm_alloc_funcs_t));
    stack->size = capacity;
    stack->memory = memory;
    stack->offset = offset;
    stack->alignment = alignment;
    alloc->internal_data = stack;
    return alloc;
}

void jdm_alloc_destroy_stack(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }
    if (alloc->internal_data) {
        jdm_stack_t* stack = (jdm_stack_t*)alloc->internal_data;
        if (stack->memory) {
            free(stack->memory);
        }
    }
}
