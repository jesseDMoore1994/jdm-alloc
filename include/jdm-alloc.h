#ifndef JDM_ALLOC_H
#define JDM_ALLOC_H
#include <jdm-defs.h>

// types of allocator supported
typedef enum {
    JDM_ALLOC_SYSTEM = 0,
    JDM_ALLOC_ARENA,
    JDM_ALLOC_STACK,
    JDM_ALLOC_POOL,
    JDM_ALLOC_FREELIST,
    JDM_ALLOC_CUSTOM,
    JDM_ALLOC_UNIMPLEMENTED
} jdm_alloc_type_t;

// Create an object to hold the memory allocation internals
typedef struct jdm_alloc_t jdm_alloc_t;

typedef struct {
    size_t arena_capacity;
    size_t arena_used;
} jdm_alloc_arena_stats_t;

typedef struct {
    size_t stack_capacity;
    size_t stack_used;
} jdm_alloc_stack_stats_t;

typedef struct {
    size_t pool_capacity;
    size_t pool_used;
    size_t chunk_capacity;
    size_t chunk_used;
} jdm_alloc_pool_stats_t;

typedef struct {
    size_t freelist_capacity;
    size_t freelist_used;
} jdm_alloc_freelist_stats_t;

typedef pointer jdm_alloc_custom_stats_t;

typedef struct {
    jdm_alloc_type_t type;
    union {
        jdm_alloc_arena_stats_t arena;
        jdm_alloc_stack_stats_t stack;
        jdm_alloc_pool_stats_t pool;
        jdm_alloc_freelist_stats_t freelist;
        jdm_alloc_custom_stats_t custom;
    };
} jdm_alloc_stats_t;

// function pointer types for memory allocation functions
typedef void* (*jdm_alloc_func_t)(jdm_alloc_t* alloc, size_t size);
typedef void (*jdm_free_func_t)(jdm_alloc_t* alloc, void* ptr);
typedef void* (*jdm_realloc_func_t)(jdm_alloc_t* alloc, void* ptr, size_t size);
typedef void* (*jdm_calloc_func_t)(jdm_alloc_t* alloc, size_t num, size_t size);
typedef jdm_alloc_stats_t (*jdm_get_stats_func_t)(jdm_alloc_t* alloc);

// struct to hold memory allocation function pointers
typedef struct {
    jdm_alloc_func_t alloc;
    jdm_free_func_t free;
    jdm_realloc_func_t realloc;
    jdm_calloc_func_t calloc;
    jdm_get_stats_func_t get_stats;
} jdm_alloc_funcs_t;

typedef struct {
    size_t alignment;
    size_t capacity;
} jdm_alloc_arena_config_t;

typedef struct {
    size_t alignment;
    size_t capacity;
} jdm_alloc_stack_config_t;

typedef struct {
    size_t alignment;
    size_t capacity;
    size_t chunk_size;
} jdm_alloc_pool_config_t;

typedef struct {
    size_t alignment;
    size_t capacity;
} jdm_alloc_freelist_config_t;

typedef struct {
    size_t alignment;  // 0 yields default, otherwise supplied is used
    size_t capacity;   // 0 yields 1MB, otherwise supplied is used
    // custom types
    jdm_alloc_funcs_t* funcs;
    pointer user_data;
} jdm_alloc_custom_config_t;

typedef struct {
    jdm_alloc_type_t type;
    b8 thread_safe; // default 0 (false)
    union {
        jdm_alloc_arena_config_t arena;
        jdm_alloc_stack_config_t stack;
        jdm_alloc_pool_config_t pool;
        jdm_alloc_freelist_config_t freelist;
        jdm_alloc_custom_config_t custom;
    };
} jdm_alloc_config_t;

// function to operate on jdm_alloc_t object
jdm_alloc_t* jdm_alloc_create(const jdm_alloc_config_t cfg);
void jdm_alloc_destroy(jdm_alloc_t* alloc);
void* jdm_alloc(jdm_alloc_t* alloc, size_t size);
void jdm_free(jdm_alloc_t* alloc, void* ptr);
void* jdm_realloc(jdm_alloc_t* alloc, void* ptr, size_t size);
void* jdm_calloc(jdm_alloc_t* alloc, size_t num, size_t size);

jdm_alloc_stats_t jdm_alloc_get_stats(jdm_alloc_t* alloc);

#endif  // JDM_ALLOC_H
