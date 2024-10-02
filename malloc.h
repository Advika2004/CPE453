#ifndef LIBMALLOC_H
#define LIBMALLOC_H
#include <stdio.h>     // For printf()
#include <stdint.h>    // For uint64_t, uintptr_t
#include <unistd.h>    // For sbrk()
#include <errno.h>     // For errno and ENOMEM
#include <inttypes.h>  // For PRIu64


#define STANDARD_HEAP_SIZE 65536  // Define this as 64KB (you can change it if needed)
#define CHUNK_HEADER sizeof(ChunkHeader)  // Size of the chunk header

// Global variable to keep track of the very first start of the heap
ChunkHeader *startOfHeap = NULL;

// Struct for the ChunkHeader
#pragma pack(push, 1) // Turn off padding for the struct
typedef struct ChunkHeader {
    uint64_t size;          // Size of the chunk
    int is_it_free;         // Flag indicating if the chunk is free
    struct ChunkHeader *prev;  // Pointer to the previous chunk in the linked list
    struct ChunkHeader *next;  // Pointer to the next chunk in the linked list
} ChunkHeader;
#pragma pack(pop) // Turn padding back on

// Struct for returning information about a free chunk
typedef struct ChunkInfo {
    ChunkHeader *start_of_free_chunk_ptr;  // Pointer to the start of the free chunk
    uint64_t amount_asked_for;             // Size of the memory requested
    int enough_space;                      // Flag indicating if there is enough space
} ChunkInfo;

#endif
