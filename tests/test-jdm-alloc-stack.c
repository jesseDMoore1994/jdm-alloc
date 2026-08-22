#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jdm-alloc.h"
#include "jdm-alloc-stack.h"

static int test_stack_allocator(void) {
    jdm_alloc_config_t cfg = {
        .type = JDM_ALLOC_STACK
    };
    jdm_alloc_t *alloc = jdm_alloc_create(cfg);
    if (!alloc) {
        fprintf(stderr, "Failed to create stack\n");
        return EXIT_FAILURE;
    }


    jdm_alloc_destroy(alloc);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    if (test_stack_allocator() != EXIT_SUCCESS) {
        fprintf(stderr, "Stack allocator test failed\n");
        return EXIT_FAILURE;
    }
    printf("Stack allocator test passed\n");
    return EXIT_SUCCESS;
}
