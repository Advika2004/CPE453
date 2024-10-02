#ifndef LIBMALLOC_H
#define LIBMALLOC_H
#include <stdio.h>     
#include <stdint.h>    
#include <unistd.h>    
#include <errno.h>     
#include <inttypes.h>  

// Define this as 64KB (you can change it if needed)
// Size of the chunk header
#define STANDARD_HEAP_SIZE 65536  
#define CHUNK_HEADER sizeof(ChunkHeader)  

typedef struct ChunkHeader {
    size_t size;          
    int is_it_free;         
    struct ChunkHeader *prev;  
    struct ChunkHeader *next;  
} ChunkHeader;

typedef struct ChunkInfo {
    ChunkHeader *start_of_free_chunk_ptr;  
    uint64_t amount_asked_for;             
    int enough_space;                      
} ChunkInfo;

#endif
