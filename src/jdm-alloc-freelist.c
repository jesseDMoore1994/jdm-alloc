#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc-internal.h"
#include "jdm-alloc.h"

typedef struct jdm_freelist_header_t jdm_freelist_header_t;
struct jdm_freelist_header_t {
    size_t size;     // size of the block
    size_t padding;  // padding to align the block
};

typedef struct jdm_freelist_node_t jdm_freelist_node_t;
struct jdm_freelist_node_t {
    jdm_freelist_node_t* next;  // pointer to next free node
    size_t size;                // size of the block
};

typedef enum jdm_freelist_placement_t jdm_freelist_placement_t;
enum jdm_freelist_placement_t {
    JDM_FREELIST_PLACEMENT_FIRST_FIT,
    JDM_FREELIST_PLACEMENT_BEST_FIT
};

typedef struct jdm_freelist_t jdm_freelist_t;
struct jdm_freelist_t {
    u8* memory;        // pointer to memory block
    u8* start;         // pointer to first block
    size_t size;       // total size of the memory block
    size_t alignment;  // alignment of the blocks
    size_t used;       // used size of the memory block

    jdm_freelist_node_t* head;           // head of free list
    jdm_freelist_placement_t placement;  // placement strategy
};

static void jdm_freelist_free_all(jdm_freelist_t* freelist) {
    if (!freelist) {
        return;
    }
    freelist->used = 0;
    jdm_freelist_node_t* node = (jdm_freelist_node_t*)freelist->start;
    node->next = NULL;
    node->size = freelist->size;
    freelist->head = node;
}

size_t calc_padding_with_header(uintptr_t ptr, uintptr_t alignment,
                                size_t header_size) {
    uintptr_t p, a, modulo, padding, needed_space;

    p = ptr;
    a = alignment;
    modulo = p & (a - 1);  // (p % a) as it assumes alignment is a power of two

    padding = 0;
    needed_space = 0;

    if (modulo != 0) {  // Same logic as 'align_forward'
        padding = a - modulo;
    }

    needed_space = (uintptr_t)header_size;

    if (padding < needed_space) {
        needed_space -= padding;

        if ((needed_space & (a - 1)) != 0) {
            padding += a * (1 + (needed_space / a));
        } else {
            padding += a * (needed_space / a);
        }
    }

    return (size_t)padding;
}

jdm_freelist_node_t* jdm_freelist_find_fit(jdm_freelist_t* freelist,
                                           size_t size, size_t alignment) {
    jdm_freelist_node_t* best_fit = NULL;
    jdm_freelist_node_t* current = freelist->head;

    while (current) {
        uintptr_t current_addr = (uintptr_t)current;
        size_t padding = calc_padding_with_header(
            current_addr, alignment, sizeof(jdm_freelist_header_t));
        size_t total_size = size + padding + sizeof(jdm_freelist_header_t);

        if (current->size >= total_size) {
            if (freelist->placement == JDM_FREELIST_PLACEMENT_FIRST_FIT) {
                return current;  // Return the first fit
            } else if (freelist->placement == JDM_FREELIST_PLACEMENT_BEST_FIT) {
                if (!best_fit || current->size < best_fit->size) {
                    best_fit = current;  // Update best fit
                }
            }
        }
        current = current->next;
    }

    return best_fit;  // Return the best fit found or NULL if none found
}

static void* freelist_alloc(jdm_alloc_t* alloc, size_t size) {
    if (!alloc || !alloc->internal_data) {
        return NULL;
    }
    if (alloc->type != JDM_ALLOC_FREELIST) {
        fprintf(stderr, "Invalid allocator type for freelist_alloc\n");
        return NULL;
    }

    jdm_freelist_t* freelist = (jdm_freelist_t*)alloc->internal_data;
    jdm_freelist_node_t* node =
        jdm_freelist_find_fit(freelist, size, freelist->alignment);
    if (!node) {
        fprintf(stderr,
                "Freelist allocator out of memory: requested %zu bytes, but "
                "only %zu bytes available\n",
                size, freelist->size - freelist->used);
        return NULL;
    }

    uintptr_t node_addr = (uintptr_t)node;
    size_t padding = calc_padding_with_header(node_addr, freelist->alignment,
                                              sizeof(jdm_freelist_header_t));
    size_t total_size = size + padding + sizeof(jdm_freelist_header_t);

    // Create header
    jdm_freelist_header_t* header =
        (jdm_freelist_header_t*)(node_addr + padding);
    header->size = size;
    header->padding = padding;

    // Update free list
    if (node->size > total_size) {
        jdm_freelist_node_t* new_node =
            (jdm_freelist_node_t*)(node_addr + total_size);
        new_node->size = node->size - total_size;
        new_node->next = node->next;
        if (freelist->head == node) {
            freelist->head = new_node;
        } else {
            jdm_freelist_node_t* prev = freelist->head;
            while (prev && prev->next != node) {
                prev = prev->next;
            }
            if (prev) {
                prev->next = new_node;
            }
        }
    } else {
        if (freelist->head == node) {
            freelist->head = node->next;
        } else {
            jdm_freelist_node_t* prev = freelist->head;
            while (prev && prev->next != node) {
                prev = prev->next;
            }
            if (prev) {
                prev->next = node->next;
            }
        }
    }

    freelist->used += total_size;
    return (void*)((uintptr_t)header + sizeof(jdm_freelist_header_t));
}

static void freelist_coalesce(jdm_freelist_t* freelist,
                              jdm_freelist_node_t* prev,
                              jdm_freelist_node_t* node) {
    if (!freelist || !node) {
        return;
    }

    uintptr_t node_addr = (uintptr_t)node;
    uintptr_t next_addr = node_addr + node->size;

    // Coalesce with next node if adjacent
    jdm_freelist_node_t* next = freelist->head;
    while (next) {
        if ((uintptr_t)next == next_addr) {
            node->size += next->size;
            node->next = next->next;
            break;
        }
        next = next->next;
    }

    // Coalesce with previous node if adjacent
    if (prev && ((uintptr_t)prev + prev->size == node_addr)) {
        prev->size += node->size;
        prev->next = node->next;
    }
}

static void freelist_free(jdm_alloc_t* alloc, void* ptr) {
    if (!alloc || !alloc->internal_data || !ptr) {
        return;
    }
    if (alloc->type != JDM_ALLOC_FREELIST) {
        fprintf(stderr, "Invalid allocator type for freelist_free\n");
        return;
    }

    jdm_freelist_t* freelist = (jdm_freelist_t*)alloc->internal_data;
    jdm_freelist_header_t* header =
        (jdm_freelist_header_t*)((uintptr_t)ptr -
                                 sizeof(jdm_freelist_header_t));
    size_t total_size =
        header->size + header->padding + sizeof(jdm_freelist_header_t);

    jdm_freelist_node_t* node =
        (jdm_freelist_node_t*)((uintptr_t)header - header->padding);
    jdm_freelist_node_t* prev = NULL;

    jdm_freelist_node_t* current = freelist->head;
    while (current && current < node) {
        prev = current;
        current = current->next;
    }

    node->size = total_size;
    node->next = current;

    if (prev) {
        prev->next = node;
    } else {
        freelist->head = node;
    }

    freelist->used -= total_size;
    freelist_coalesce(freelist, prev, node);
}

static void* freelist_realloc(jdm_alloc_t* alloc, void* ptr, size_t size) {
    if (!alloc || !alloc->internal_data) {
        return NULL;
    }
    if (alloc->type != JDM_ALLOC_FREELIST) {
        fprintf(stderr, "Invalid allocator type for freelist_realloc\n");
        return NULL;
    }

    if (ptr == NULL) {
        return freelist_alloc(alloc, size);
    } else if (size == 0) {
        freelist_free(alloc, ptr);
        return NULL;
    } else {
        jdm_freelist_header_t* header =
            (jdm_freelist_header_t*)((uintptr_t)ptr -
                                     sizeof(jdm_freelist_header_t));
        size_t old_size = header->size;

        void* new_ptr = freelist_alloc(alloc, size);
        if (!new_ptr) {
            return NULL;  // Allocation failed
        }

        // Copy the old data to the new location
        size_t copy_size = old_size < size ? old_size : size;
        memcpy(new_ptr, ptr, copy_size);

        freelist_free(alloc, ptr);
        return new_ptr;
    }
}

static void* freelist_calloc(jdm_alloc_t* alloc, size_t num, size_t size) {
    size_t total_size = num * size;
    void* ptr = freelist_alloc(alloc, total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

static jdm_alloc_funcs_t freelist_funcs = {.alloc = freelist_alloc,
                                           .free = freelist_free,
                                           .realloc = freelist_realloc,
                                           .calloc = freelist_calloc};

jdm_alloc_t* jdm_alloc_create_freelist(jdm_alloc_config_t cfg) {
    size_t capacity = cfg.freelist.capacity == 0 ? get_default_capacity()
                                                 : cfg.freelist.capacity;
    size_t alignment = cfg.freelist.alignment == 0 ? get_default_alignment()
                                                   : cfg.freelist.alignment;

    size_t capacity_plus_structs =
        capacity + JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment) +
        JDM_ALIGN_UP(sizeof(jdm_freelist_t), alignment);

    jdm_alloc_t* alloc = NULL;
    jdm_freelist_t* freelist = NULL;
    u8* memory = (u8*)malloc(capacity_plus_structs);
    if (!memory) {
        fprintf(stderr, "Failed to allocate memory for freelist allocator\n");
        return NULL;
    }

    size_t current_offset = 0;
    size_t aligned_size = JDM_ALIGN_UP(sizeof(jdm_alloc_t), alignment);
    if (current_offset + aligned_size > capacity) {
        fprintf(stderr,
                "Freelist allocator out of memory: requested %zu bytes, but "
                "only %zu bytes available\n",
                current_offset + aligned_size, capacity);
        free(memory);
        return NULL;
    }

    alloc = (jdm_alloc_t*)(memory + current_offset);
    current_offset += aligned_size;

    aligned_size = JDM_ALIGN_UP(sizeof(jdm_freelist_t), alignment);
    if (current_offset + aligned_size > capacity) {
        fprintf(stderr,
                "Freelist allocator out of memory: requested %zu bytes, but "
                "only %zu bytes available\n",
                current_offset + aligned_size, capacity);
        free(memory);
        return NULL;
    }

    freelist = (jdm_freelist_t*)(memory + current_offset);
    current_offset += aligned_size;

    alloc->type = JDM_ALLOC_FREELIST;
    JDM_COPY_MEMORY(&alloc->funcs, &freelist_funcs, sizeof(jdm_alloc_funcs_t));

    freelist->memory = memory;
    freelist->start = memory + current_offset;
    freelist->size = capacity - current_offset;
    freelist->alignment = alignment;
    freelist->head = NULL;
    freelist->placement = JDM_FREELIST_PLACEMENT_BEST_FIT;
    alloc->internal_data = freelist;
    jdm_freelist_free_all(freelist);
    return alloc;
}

void jdm_alloc_destroy_freelist(jdm_alloc_t* alloc) {
    if (!alloc) {
        return;
    }
    if (alloc->internal_data) {
        jdm_freelist_t* freelist = (jdm_freelist_t*)alloc->internal_data;
        if (freelist->memory) {
            free(freelist->memory);
        }
    }
}

jdm_alloc_stats_t jdm_alloc_get_stats_freelist(jdm_alloc_t* alloc) {
    jdm_alloc_stats_t stats = {0};
    if (!alloc || !alloc->internal_data) {
        return stats;
    }
    if (alloc->type != JDM_ALLOC_FREELIST) {
        fprintf(stderr,
                "Invalid allocator type for jdm_alloc_get_stats_freelist\n");
        return stats;
    }
    jdm_freelist_t* freelist = (jdm_freelist_t*)alloc->internal_data;
    stats.type = JDM_ALLOC_FREELIST;
    stats.freelist.freelist_capacity = freelist->size;
    stats.freelist.freelist_used = freelist->used;
    return stats;
}
