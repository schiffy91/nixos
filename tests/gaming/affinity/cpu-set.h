#ifndef NIXOS_TEST_CPU_SET_H
#define NIXOS_TEST_CPU_SET_H
#include <errno.h>
#include <sched.h>
#include <stdlib.h>

typedef struct TestCpuSetState {
    cpu_set_t *bits;
    size_t bytes;
    int capacity;
} *TestCpuSet;

static void testCpuSetDestroy(TestCpuSet set) {
    if (set) { CPU_FREE(set->bits); free(set); }
}

static TestCpuSet testCpuSetCreate(void) {
    TestCpuSet set = calloc(1, sizeof(*set));
    if (!set) return NULL;
    for (int capacity = 128; capacity <= 1048576; capacity *= 2) {
        set->bits = CPU_ALLOC(capacity);
        set->bytes = CPU_ALLOC_SIZE(capacity);
        set->capacity = capacity;
        if (!set->bits) { free(set); return NULL; }
        if (sched_getaffinity(0, set->bytes, set->bits) == 0) return set;
        int error = errno;
        CPU_FREE(set->bits);
        set->bits = NULL;
        if (error != EINVAL) break;
    }
    testCpuSetDestroy(set);
    return NULL;
}

static int testCpuSetCapacity(TestCpuSet set) { return set->capacity; }
static int testCpuSetContains(TestCpuSet set, int cpu) {
    return cpu >= 0 && cpu < set->capacity && CPU_ISSET_S(cpu, set->bytes, set->bits);
}
#endif
