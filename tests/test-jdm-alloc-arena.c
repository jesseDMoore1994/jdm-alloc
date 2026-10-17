#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc-arena.h"
#include "jdm-alloc.h"

static int test_arena_allocator(void) {
    jdm_alloc_config_t cfg = {.type = JDM_ALLOC_ARENA};
    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        fprintf(stderr, "Failed to create arena\n");
        return EXIT_FAILURE;
    }

    uint8_t* start_base_address = jdm_alloc_arena_get_base_address(alloc);
    size_t start_prev_offset = jdm_alloc_arena_get_previous_offset(alloc);
    size_t start_curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    fprintf(stderr, "Starting offset after arena allocated: %ld\n",
            start_curr_offset);

    // Allocate a block of memory
    size_t size = 1024;
    void* ptr = jdm_alloc(alloc, size);
    if (!ptr) {
        fprintf(stderr, "Failed to alloc in arena\n");
        return EXIT_FAILURE;
    }

    // Zero allocate another block of memory
    void* ptr2 = jdm_calloc(alloc, 1, size);
    if (!ptr2) {
        fprintf(stderr, "Failed to calloc in arena\n");
        return EXIT_FAILURE;
    }
    if (*(char*)ptr2 != '\0' && memcmp(ptr2, ptr2 + 1, size - 1) != 0) {
        fprintf(stderr, "calloc did not zero initialize memory\n");
        // print memory in hex
        const unsigned char* byte = (const unsigned char*)ptr;
        size_t alignment = sizeof(void*);
        for (size_t i = 1; i <= size; i++) {
            // %02X prints each byte as a 2-digit uppercase hex value
            fprintf(stderr, "%02X ", byte[i]);
            if (i % alignment == 0) fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
        return EXIT_FAILURE;
    }

    // At this point current offset should be 2048 and prev pointer should
    // be ptr2
    uint8_t* base_address = jdm_alloc_arena_get_base_address(alloc);
    size_t prev_offset = jdm_alloc_arena_get_previous_offset(alloc);
    size_t curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset != 2048 + start_curr_offset) {
        fprintf(stderr, "current offset misaligned\n");
        return EXIT_FAILURE;
    }
    if (ptr2 != base_address + prev_offset) {
        fprintf(stderr, "ptr2 not aligned on previous offset\n");
        return EXIT_FAILURE;
    }

    // Attempting to free ptr1 should not move anything, as it is illegal
    jdm_free(alloc, ptr);
    prev_offset = jdm_alloc_arena_get_previous_offset(alloc);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset == prev_offset) {
        fprintf(stderr, "ptr2 was freed accidentally\n");
        return EXIT_FAILURE;
    }

    // freeing ptr2 however should work
    jdm_free(alloc, ptr2);
    prev_offset = jdm_alloc_arena_get_previous_offset(alloc);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset != prev_offset) {
        fprintf(stderr, "ptr2 was not freed as it should be\n");
        return EXIT_FAILURE;
    }

    // Zero allocate another block of memory
    ptr2 = jdm_calloc(alloc, 1, size);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset != 2048 + start_curr_offset) {
        fprintf(stderr, "ptr2 could not be reserved again after free\n");
        return EXIT_FAILURE;
    }

    // attempting to reallocate ptr should not work
    jdm_realloc(alloc, ptr, 4096);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset != 2048 + start_curr_offset) {
        fprintf(stderr, "ptr was realloced discontiguously\n");
        return EXIT_FAILURE;
    }

    // attempting to reallocate ptr2 should
    jdm_realloc(alloc, ptr2, 4096);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset != 5120 + start_curr_offset) {
        fprintf(stderr, "ptr2 did not resize correctly\n");
        return EXIT_FAILURE;
    }

    // create checkpoint and save offset
    jdm_alloc_arena_checkpoint_t c = jdm_alloc_arena_create_checkpoint(alloc);
    prev_offset = jdm_alloc_arena_get_previous_offset(alloc);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);

    // allocate some junk
    jdm_calloc(alloc, 10, size);
    if (jdm_alloc_arena_get_current_offset(alloc) !=
        (5120 + start_curr_offset + (10 * size))) {
        fprintf(stderr, "Junk did not get allocated correctly\n");
        return EXIT_FAILURE;
    }

    // restore checkpoint and validate that offsets are restored
    jdm_alloc_arena_restore_checkpoint(alloc, c);
    if (jdm_alloc_arena_get_current_offset(alloc) != curr_offset) {
        fprintf(stderr, "Current offset did not restore\n");
        return EXIT_FAILURE;
    }
    if (jdm_alloc_arena_get_previous_offset(alloc) != prev_offset) {
        fprintf(stderr, "Previous offset did not restore\n");
        return EXIT_FAILURE;
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    if (test_arena_allocator() != EXIT_SUCCESS) {
        fprintf(stderr, "Arena allocator test failed\n");
        return EXIT_FAILURE;
    }
    printf("Arena allocator test passed\n");
    return EXIT_SUCCESS;
}
