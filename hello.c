#include <malloc.h>

int main() {
// Step 1: Initialize the heap
    startOfHeap = initialize_heap();
    if (startOfHeap == NULL) {
        fprintf(stderr, "Heap initialization failed.\n");
        return 1;
    }

    // Step 2: Request exactly the maximum amount of memory available
    uint64_t max_memory = 65489;  // Maximum usable memory after the first header
    printf("\nRequesting exact maximum allocation: %zu bytes.\n", max_memory);
    void* ptr1 = my_malloc(max_memory);
    if (ptr1 != NULL) {
        printf("Allocated %zu bytes at %p\n", max_memory, ptr1);
    } else {
        printf("Allocation failed.\n");
    }

    // // Step 3: Request more than available memory (should trigger more heap allocation)
    // uint64_t over_memory = 66000;  // More than available memory
    // printf("\nRequesting more than available memory: %zu bytes.\n", over_memory);
    // void* ptr2 = my_malloc(over_memory);
    // if (ptr2 != NULL) {
    //     printf("Allocated %zu bytes at %p\n", over_memory, ptr2);
    // } else {
    //     printf("Allocation failed.\n");
    // }

    // // Step 4: Request minimum allocation (16 bytes, because it's the smallest aligned size)
    // uint64_t small_alloc = 1;  // Request 1 byte, but it should allocate 16 bytes
    // printf("\nRequesting minimum allocation: %zu bytes (should allocate 16).\n", small_alloc);
    // void* ptr3 = my_malloc(small_alloc);
    // if (ptr3 != NULL) {
    //     printf("Allocated %zu bytes at %p\n", make_16(small_alloc), ptr3);
    // } else {
    //     printf("Allocation failed.\n");
    // }

    // // Step 5: Request a really large amount of memory (more than 64KB)
    // uint64_t large_alloc = 700000;  // Large memory request (700KB)
    // printf("\nRequesting large allocation: %zu bytes.\n", large_alloc);
    // void* ptr4 = my_malloc(large_alloc);
    // if (ptr4 != NULL) {
    //     printf("Allocated %zu bytes at %p\n", large_alloc, ptr4);
    // } else {
    //     printf("Allocation failed.\n");
    // }

    // Print heap status after allocations
    printf("\nHeap status after allocations:\n");
    ChunkHeader* current_chunk = startOfHeap;
    while (current_chunk != NULL) {
        printf("Chunk start: %p, Size: %" PRIu64 " bytes, Is it free?: %d\n", 
               (void*)current_chunk, current_chunk->size, current_chunk->is_it_free);
        // Move to the next chunk
        current_chunk = current_chunk->next;
    }

    return 0;
}