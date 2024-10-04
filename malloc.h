#ifndef LIBMALLOC_H
#define LIBMALLOC_H
#include <stdio.h>     
#include <stdint.h>    
#include <unistd.h>    
#include <errno.h>     
#include <inttypes.h>  
#include <string.h>
#include <stdlib.h>


#define STANDARD_HEAP_SIZE 65536
#define CHUNK_HEADER sizeof(ChunkHeader)  
#define BUFFER_SIZE 1500
#define length strlen(buf)

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

void* initialize_heap();
void* malloc(size_t requested_amount);
void* allocate_leftover(size_t left_to_allocate);
size_t make_16(size_t number);
void split_chunk(ChunkInfo chunk_info);
ChunkInfo find_free_chunk(ChunkHeader* currChunk, size_t reqSize);
void* get_more_heap();
void* calloc(size_t num_elements, size_t element_size);
void* realloc(void* ptr, size_t new_size);
void free(void* ptr);
ChunkHeader* find_which_chunk(void* ptr);
ChunkHeader* combine_free_chunks(ChunkHeader* currChk);
ChunkHeader* create_leftover_chunk(ChunkHeader* chunk, size_t new_size);
void* allocate_new_chunk_and_copy(void* ptr, size_t new_size);

void print_heap();

#endif
