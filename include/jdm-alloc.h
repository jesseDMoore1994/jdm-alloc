#ifndef JDM_ALLOC_H
#define JDM_ALLOC_H
#include <stddef.h>
#include <stdint.h>

// Macro to mark a variable as unused to avoid compiler warnings
#define UNUSED(x) (void)(x)

// Common data types
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define s8 int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t
#define f32 float
#define f64 double
#define b8 u8
#define b16 u16
#define b32 u32
#define b64 u64
#define pointer void*

#define JDM_TRUE 1
#define JDM_FALSE 0

// Define function like macros for common utilities
#define JDM_MIN(a, b) ((a) < (b) ? (a) : (b))
#define JDM_MAX(a, b) ((a) > (b) ? (a) : (b))
#define JDM_CLAMP(x, min, max) (JDM_MIN(JDM_MAX((x), (min)), (max)))
#define JDM_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define JDM_ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#define JDM_ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define JDM_IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)
#define JDM_SWAP(a, b)       \
    do {                     \
        typeof(a) tmp = (a); \
        (a) = (b);           \
        (b) = tmp;           \
    } while (0)

#define JDM_ZERO_MEMORY(ptr, size) \
    do {                           \
        memset((ptr), 0, (size));  \
    } while (0)

#define JDM_COPY_MEMORY(dest, src, size) \
    do {                                 \
        memcpy((dest), (src), (size));   \
    } while (0)

#define JDM_SET_MEMORY(ptr, value, size) \
    do {                                 \
        memset((ptr), (value), (size));  \
    } while (0)
#define JDM_MOVE_MEMORY(dest, src, size) \
    do {                                 \
        memmove((dest), (src), (size));  \
    } while (0)

#define JDM_IS_POWER_OF_TWO(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))

#define JDM_ROUND_UP_TO_POWER_OF_TWO(x) \
    (JDM_IS_POWER_OF_TWO(x) ? (x) : (1 << (32 - __builtin_clz((x) - 1))))

#define JDM_ROUND_DOWN_TO_POWER_OF_TWO(x) \
    (JDM_IS_POWER_OF_TWO(x) ? (x) : (1 << (31 - __builtin_clz((x)))))

#define JDM_NEXT_POWER_OF_TWO(x) \
    (JDM_IS_POWER_OF_TWO(x) ? (x) : (1 << (32 - __builtin_clz((x)))))

// types of allocator supported
typedef enum {
    JDM_ALLOC_DEFAULT = 0,
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
