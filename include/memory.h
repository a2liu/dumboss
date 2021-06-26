#pragma once
#include "bootboot.h"
#include "logging.h"
#include <stddef.h>
#include <stdint.h>

#define _4KB ((uint64_t)4096)
// #define _2MB ((uint64_t)2097152)
// #define _1GB ((uint64_t)1073741824)

int64_t alloc__init(MMapEnt *entries, int64_t entry_count);

// Allocate `count` contiguous pages, each of size 4kb
void *alloc(int64_t count);

// Free contiguous pages starting at data
void free(void *data, int64_t count);

// Check that the heap is in a valid state
void alloc__validate_heap(void);