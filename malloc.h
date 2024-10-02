#ifndef LIBMALLOC_H
#define LIBMALLOC_H
#include <stdio.h>     
#include <stdint.h>    
#include <unistd.h>    
#include <errno.h>     
#include <inttypes.h>  

 // For printf()
 // For uint64_t, uintptr_t
 // For sbrk()
 // For errno and ENOMEM
 

// Define this as 64KB (you can change it if needed)
// Size of the chunk header
#define STANDARD_HEAP_SIZE 65536  
#define CHUNK_HEADER sizeof(ChunkHeader)  

// Global variable to keep track of the very first                     
//start of the heap
// ChunkHeader *startOfHeap = NULL;

// Struct for the ChunkHeader
// Turn off padding for the struct
// Size of the chunk
// Flag indicating if the chunk is free
// Pointer to the previous chunk in the linked list
// Pointer to the next chunk in the linked list
#pragma pack(push, 1) 
typedef struct ChunkHeader {
    size_t size;          
    int is_it_free;         
    struct ChunkHeader *prev;  
    struct ChunkHeader *next;  
} ChunkHeader;
#pragma pack(pop) 
// Turn padding back on

// Struct for returning information about a free chunk
// Pointer to the start of the free chunk
// Size of the memory requested
//Flag indicating if there is enough space//
typedef struct ChunkInfo {
    ChunkHeader *start_of_free_chunk_ptr;  
    uint64_t amount_asked_for;             
    int enough_space;                      
} ChunkInfo;

#endif
