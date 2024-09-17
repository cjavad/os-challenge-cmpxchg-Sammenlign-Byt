#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define BUFFER_SIZE 1024

typedef struct {
    uint64_t buffer[BUFFER_SIZE]; // The actual data buffer
    int free_list[BUFFER_SIZE];   // The free list storing available indices
    int free_count;               // Number of free slots available
} free_list_t;

inline void free_list_init(free_list_t* rb) {
    rb->free_count = BUFFER_SIZE;
    for (int i = 0; i < BUFFER_SIZE; ++i) {
	rb->free_list[i] = i; // Initially, all slots are free
    }
}

inline int free_list_store(free_list_t* rb, const uint64_t data) {
    if (rb->free_count == 0) {
	// No available slots
	return -1;
    }

    // Get the next free index
    int index = rb->free_list[--rb->free_count];
    rb->buffer[index] = data; // Store the data in the buffer
    return index;
}

inline uint64_t free_list_get(const free_list_t* rb, const int index) {
    if (index < 0 || index >= BUFFER_SIZE) {
	printf("Invalid index!\n");
	return 0; // Handle invalid index error
    }
    return rb->buffer[index];
}

inline void free_list_free(free_list_t* rb, const int index) {
    if (index < 0 || index >= BUFFER_SIZE || rb->free_count == BUFFER_SIZE) {
	printf("Invalid index or buffer full!\n");
	return; // Handle invalid index or full free list
    }

    // Add the index back to the free list
    rb->free_list[rb->free_count++] = index;
}