#include "malloc.h"

int main() {
// Step 1: Initialize the heap
    startOfHeap = initialize_heap();
    if (startOfHeap == NULL) {
        fprintf(stderr, "Heap initialization failed.\n");
        return 1;
    }

    // Step 2: Request exactly the maximum amount of memory available
    // Maximum usable memory after the first header
    uint64_t max_memory = 65489;  
    printf("\nRequesting exact maximum allocation: %zu bytes.\n", max_memory);
    void* ptr1 = my_malloc(max_memory);
    if (ptr1 != NULL) {
        printf("Allocated %zu bytes at %p\n", max_memory, ptr1);
    } else {
        printf("Allocation failed.\n");
    }

    // Print heap status after allocations
    printf("\nHeap status after allocations:\n");
    ChunkHeader* current_chunk = startOfHeap;
    while (current_chunk != NULL) {
        printf("Chunk start: %p, Size: %" PRIu64 " bytes, Is it free?: %d\n", 
               (void*)current_chunk, current_chunk->size, 
               current_chunk->is_it_free);
        // Move to the next chunk
        current_chunk = current_chunk->next;
    }

    return 0;
}