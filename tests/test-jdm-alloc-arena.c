#include <jdm-log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jdm-alloc-arena.h"
#include "jdm-alloc.h"

static int test_arena_allocator(void) {
    jdm_alloc_config_t cfg = {.type = JDM_ALLOC_ARENA};
    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        JDM_LOG_ERROR("Failed to create arena allocator");
        return EXIT_FAILURE;
    }

    uint8_t* start_base_address = jdm_alloc_arena_get_base_address(alloc);
    size_t start_prev_offset = jdm_alloc_arena_get_previous_offset(alloc);
    size_t start_curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    JDM_LOG_DEBUG("Starting base address after arena allocated: %p",
                  start_base_address);
    JDM_LOG_DEBUG("Starting previous offset after arena allocated: %zu",
                  start_prev_offset);
    JDM_LOG_DEBUG("Starting current offset after arena allocated: %zu",
                  start_curr_offset);

    // Allocate a block of memory
    size_t size = 1024;
    void* ptr = jdm_alloc(alloc, size);
    if (!ptr) {
        JDM_LOG_ERROR("Failed to alloc in arena");
        return EXIT_FAILURE;
    }

    // Zero allocate another block of memory
    void* ptr2 = jdm_calloc(alloc, 1, size);
    if (!ptr2) {
        JDM_LOG_ERROR("Failed to calloc in arena");
        return EXIT_FAILURE;
    }
    if (*(char*)ptr2 != '\0' && memcmp(ptr2, ptr2 + 1, size - 1) != 0) {
        JDM_LOG_ERROR("calloc did not zero initialize memory");
        // print memory in hex
        const unsigned char* byte = (const unsigned char*)ptr;
        size_t alignment = sizeof(void*);
        for (size_t i = 1; i <= size; i++) {
            // %02X prints each byte as a 2-digit uppercase hex value
            JDM_LOG_DEBUG("%02X ", byte[i - 1]);
            if (i % alignment == 0) JDM_LOG_DEBUG("\n");
        }
        JDM_LOG_DEBUG("\n");
        return EXIT_FAILURE;
    }

    // At this point current offset should be 2048 and prev pointer should
    // be ptr2
    uint8_t* base_address = jdm_alloc_arena_get_base_address(alloc);
    size_t prev_offset = jdm_alloc_arena_get_previous_offset(alloc);
    size_t curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset != 2048 + start_curr_offset) {
        JDM_LOG_ERROR("Current offset is not 2048 after two allocations");
        return EXIT_FAILURE;
    }
    if (ptr2 != base_address + prev_offset) {
        JDM_LOG_ERROR("ptr2 is not aligned on previous offset");
        return EXIT_FAILURE;
    }

    // Attempting to free ptr1 should not move anything, as it is illegal
    jdm_free(alloc, ptr);
    prev_offset = jdm_alloc_arena_get_previous_offset(alloc);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset == prev_offset) {
        JDM_LOG_ERROR("ptr was freed accidentally");
        return EXIT_FAILURE;
    }

    // freeing ptr2 however should work
    jdm_free(alloc, ptr2);
    prev_offset = jdm_alloc_arena_get_previous_offset(alloc);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset != prev_offset) {
        JDM_LOG_ERROR("ptr2 was not freed as it should be");
        return EXIT_FAILURE;
    }

    // Zero allocate another block of memory
    ptr2 = jdm_calloc(alloc, 1, size);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset != 2048 + start_curr_offset) {
        JDM_LOG_ERROR("Current offset is not 2048 after reallocating ptr2");
        return EXIT_FAILURE;
    }

    // attempting to reallocate ptr should not work
    jdm_realloc(alloc, ptr, 4096);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset != 2048 + start_curr_offset) {
        JDM_LOG_ERROR("ptr was reallocated when it should not have been");
        return EXIT_FAILURE;
    }

    // attempting to reallocate ptr2 should
    jdm_realloc(alloc, ptr2, 4096);
    curr_offset = jdm_alloc_arena_get_current_offset(alloc);
    if (curr_offset != 5120 + start_curr_offset) {
        JDM_LOG_ERROR("ptr2 was not reallocated when it should have been");
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
        JDM_LOG_ERROR("Current offset did not increase after junk allocation");
        return EXIT_FAILURE;
    }

    // restore checkpoint and validate that offsets are restored
    jdm_alloc_arena_restore_checkpoint(alloc, c);
    if (jdm_alloc_arena_get_current_offset(alloc) != curr_offset) {
        JDM_LOG_ERROR("Current offset did not restore");
        return EXIT_FAILURE;
    }
    if (jdm_alloc_arena_get_previous_offset(alloc) != prev_offset) {
        JDM_LOG_ERROR("Previous offset did not restore");
        return EXIT_FAILURE;
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

static int test_arena_checkpoint(void) {
    size_t capacity = 4096;
    jdm_alloc_config_t cfg = {
        .type = JDM_ALLOC_ARENA,
        .arena = {.capacity = capacity},
    };
    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        JDM_LOG_ERROR("Failed to create arena allocator");
        return EXIT_FAILURE;
    }

    size_t size = 1024;
    void* ptr = jdm_alloc(alloc, size);
    if (!ptr) {
        JDM_LOG_ERROR("Failed to alloc in arena");
        return EXIT_FAILURE;
    }

    jdm_alloc_arena_checkpoint_t checkpoint =
        jdm_alloc_arena_create_checkpoint(alloc);

    if (checkpoint.curr_offset != jdm_alloc_arena_get_current_offset(alloc)) {
        JDM_LOG_ERROR("Checkpoint current offset does not match");
        return EXIT_FAILURE;
    }
    if (checkpoint.prev_offset != jdm_alloc_arena_get_previous_offset(alloc)) {
        JDM_LOG_ERROR("Checkpoint previous offset does not match");
        return EXIT_FAILURE;
    }

    // Allocate some more memory
    void* ptr2 = jdm_alloc(alloc, size);
    if (!ptr2) {
        JDM_LOG_ERROR("Failed to alloc in arena");
        return EXIT_FAILURE;
    }

    // get new checkpoint and validate that it is different from the previous
    // one
    jdm_alloc_arena_checkpoint_t checkpoint2 =
        jdm_alloc_arena_create_checkpoint(alloc);
    if (checkpoint2.curr_offset == checkpoint.curr_offset) {
        JDM_LOG_ERROR("Checkpoint current offset did not change");
        return EXIT_FAILURE;
    }
    if (checkpoint2.prev_offset == checkpoint.prev_offset) {
        JDM_LOG_ERROR("Checkpoint previous offset did not change");
        return EXIT_FAILURE;
    }

    // Restore the first checkpoint and validate that the offsets are restored
    jdm_alloc_arena_checkpoint_t checkpoint3 = checkpoint;
    jdm_alloc_arena_restore_checkpoint(alloc, checkpoint3);
    if (checkpoint3.curr_offset != checkpoint.curr_offset) {
        JDM_LOG_ERROR("Restored checkpoint current offset does not match");
        return EXIT_FAILURE;
    }
    if (checkpoint3.prev_offset != checkpoint.prev_offset) {
        JDM_LOG_ERROR("Restored checkpoint previous offset does not match");
        return EXIT_FAILURE;
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

static int test_arena_stats(void) {
    size_t capacity = 4096;
    jdm_alloc_config_t cfg = {
        .type = JDM_ALLOC_ARENA,
        .arena = {.capacity = capacity},
    };
    jdm_alloc_t* alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        JDM_LOG_ERROR("Failed to create arena allocator");
        return EXIT_FAILURE;
    }

    size_t size = 1024;
    void* ptr = jdm_alloc(alloc, size);
    if (!ptr) {
        JDM_LOG_ERROR("Failed to alloc in arena");
        return EXIT_FAILURE;
    }

    jdm_alloc_stats_t stats = jdm_alloc_get_stats(alloc);
    if (stats.type != JDM_ALLOC_ARENA) {
        JDM_LOG_ERROR("Stats type is not arena");
        return EXIT_FAILURE;
    }
    if (stats.arena.arena_used != size) {
        JDM_LOG_ERROR("Stats used does not meet expected used");
        return EXIT_FAILURE;
    }
    if (stats.arena.arena_capacity != capacity) {
        JDM_LOG_ERROR("Stats capacity does not meet expected capacity");
        return EXIT_FAILURE;
    }

    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    jdm_log_init();
    jdm_log_enable_thread_id(JDM_LOG_DISABLE_THREAD_ID);
    if (test_arena_allocator() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Arena allocator test failed");
        return EXIT_FAILURE;
    }
    if (test_arena_checkpoint() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Arena checkpoint test failed");
        return EXIT_FAILURE;
    }
    if (test_arena_stats() != EXIT_SUCCESS) {
        JDM_LOG_ERROR("Arena stats test failed");
        return EXIT_FAILURE;
    }
    JDM_LOG_INFO("Arena allocator test passed");
    jdm_log_cleanup();
    return EXIT_SUCCESS;
}
