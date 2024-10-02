#include "malloc.h"

ChunkHeader *startOfHeap = NULL; //global variable to keep track of the very first start of the heap


//function that takes in any integer, checks if it is divisible by 16, if not it will round up until it is possible.
uint64_t make_16(uint64_t number){
    while((number % 16) != 0){
        number++;
    }
    return number;
}

void* initialize_heap(){
    startOfHeap = sbrk(0); //tells me where the heap is going to start. this is the global variable that I am updating
    if(startOfHeap == (void*)-1){ //sbrk returns a pointer to -1 when it fails
        errno = ENOMEM;
        return NULL;
    }

    //must check if that is divisible by 16  if not I have to make it that way and move the pointer accordingly
    //cast to an int so I can do math on it 
    uintptr_t mathableAddy = (uintptr_t)startOfHeap;
    if((mathableAddy % 16) != 0){
        uintptr_t byteAdjustment = 16 - (mathableAddy % 16); //gives you how many bytes to move the pointer forward until its divisible by 16

        printf("byte adjustment: %zu\n", byteAdjustment);

        void* AlignedStartOfHeap = sbrk(byteAdjustment); //move the program break that many bytes forward until the next multiple of 16

        printf("new aligned start of heap: %p\n", AlignedStartOfHeap);

        if(AlignedStartOfHeap == (void*)-1){ //if the sbrk fails make it error
            errno = ENOMEM;
            return NULL;
        }
    }

    void* firstChunkStart = sbrk(STANDARD_HEAP_SIZE); //first call to sbrk will allocate 64KB of memory
    if(firstChunkStart == (void*)-1){ 
        errno = ENOMEM;
        return NULL;
    }

    printf("First chunk start address: %p\n", firstChunkStart);

    ChunkHeader *firstChunk = (ChunkHeader*)firstChunkStart; //casting the void pointer to a chunkHeader pointer
    //this essentially puts the first node in the chunk of memory because the start of the memory chunk points to the chunk header
    uint64_t calculated_size = STANDARD_HEAP_SIZE - CHUNK_HEADER;

    printf("calculated size before making it 16 multiple: %zu\n", calculated_size);

    firstChunk->size = make_16(calculated_size);

    printf("First chunk size (should be divisible by 16): %zu\n", firstChunk->size);

    firstChunk->is_it_free = 1; //yes this chunk is free
    firstChunk->prev = NULL;
    firstChunk->next = NULL;

    //make sure that the fields get assigned properly
    printf("Chunk Header - Address: %p, Size: %zu, Is It Free?: %d\n", (void*)firstChunk, firstChunk->size, firstChunk->is_it_free);

    return firstChunk; //return a pointer to where the first chunk header starts and / is!
}


ChunkInfo find_free_chunk(ChunkHeader* currentChunk, uint64_t requestedSize){ //current chunk is the same as the first chunk that gets returned by initialize_heap()
    ChunkInfo chunk_info;

    requestedSize = make_16(requestedSize); //make sure what they ask for is aligned 

    //start off with there is no valid chunk
    chunk_info.enough_space = 0;
    chunk_info.amount_asked_for = 0;
    chunk_info.start_of_free_chunk_ptr = NULL;

    while (currentChunk != NULL){
        if (currentChunk->is_it_free == 1 && currentChunk->size >= requestedSize){ //if its a free chunk and enough space
            chunk_info.enough_space = 1; //yes there is enough space
            chunk_info.start_of_free_chunk_ptr = currentChunk; //so the start of the free chunk gets stored at the start of the header of the free chunk the traversal function found
            chunk_info.amount_asked_for = requestedSize; 
            return chunk_info;
        }
        else{ //else move onto the next chunk
            currentChunk = currentChunk->next;
        }
    }
    return chunk_info;   //will have zero and null if nothing is found
}


int split_chunk(ChunkInfo chunk_info){
    ChunkHeader *split_allocated = chunk_info.start_of_free_chunk_ptr; //the allocated chunk will be pointing to where the chunk that was found as free from the last function's header starts
    split_allocated->is_it_free = 0; //edit that header and make it not free
    //calculate how much free space is left 
    //(current chunk size - requested size - sizeof(ChunkHeader))

    printf("Chunk size: %" PRIu64 "\n", split_allocated->size);
    printf("Requested size (aligned): %" PRIu64 "\n", chunk_info.amount_asked_for);
    printf("Header size: %zu\n", sizeof(ChunkHeader));  

    
    // check if the segment I need is larger than what I have
    if (chunk_info.amount_asked_for + sizeof(ChunkHeader) > split_allocated->size) {
    // not enough space, the math will go negative and seg fault
        printf("user asked for more than what is available, no splitting. \n");
        return -1; 
    }

    uint64_t user_free_space_left = split_allocated->size - chunk_info.amount_asked_for - sizeof(ChunkHeader);
    printf("FREE SPACE LEFT: %" PRIu64 "\n", user_free_space_left);

    // Check if there is enough space left for another chunk
    if (user_free_space_left <= sizeof(ChunkHeader) + 16) { // +16 is the minimum space to allocate
        printf("REACHING HERE OR NAH\n");
        printf("Not enough space for a new header, using the entire chunk.\n");
        return 0; // Allocate the whole chunk
    }

    //now that I have calculated the amount of free space left for the user, I can update the header for the size of how much was allocated 
    split_allocated->size = chunk_info.amount_asked_for; //the size is the aligned amount the user asked for 
    //calculate where the new chunk starts (should be where the next header starts)
    //next header starts after the amount allocated ends. Make sure that it is lined up with 16 
    uint64_t free_chunk_start = make_16((((uintptr_t)split_allocated) + chunk_info.amount_asked_for + sizeof(ChunkHeader)));
    ChunkHeader* split_free = (ChunkHeader*)free_chunk_start;

    split_free->is_it_free = 1;
    split_free->size = user_free_space_left;
    //make sure the linked list is being constructed properly

    //the free chunk is next and the allocated chunk is first
    split_free->prev = split_allocated;
    split_free->next = split_allocated->next;
    split_allocated->next = split_free;

    // If there's a chunk after the new free chunk its previous is now split_free and not split_allocated
    if (split_free->next != NULL) {
        split_free->next->prev = split_free; 
        //split_free->next is what used to be the big chunk's next 
        //so split_free->next->prev is the big chunk's next's previous (refer to drawing on ipad)
    }

    return 1;
}


void* get_more_heap(){

    void* startOfNewHeap = sbrk(0); //tells me where the new heap chunk will start
    if(startOfNewHeap == (void*)-1){ //sbrk returns a pointer to -1 when it fails
        errno = ENOMEM;
        return NULL;
    }

    printf("new chunk of the heap start address: %p\n", startOfNewHeap);

    //must check if that is divisible by 16  if not I have to make it that way and move the pointer accordingly
    //cast to an int so I can do math on it 

    uintptr_t mathableNewAddy = (uintptr_t)startOfNewHeap;
    if((mathableNewAddy % 16) != 0){
        uintptr_t byteNewAdjustment = 16 - (mathableNewAddy % 16); //gives you how many bytes to move the pointer forward until its divisible by 16

        printf("byte adjustment: %zu\n", byteNewAdjustment);

        void* AlignedStartOfNewHeap = sbrk(byteNewAdjustment); //move the program break that many bytes forward until the next multiple of 16

        printf("new aligned start of heap: %p\n", AlignedStartOfNewHeap);

        if(AlignedStartOfNewHeap == (void*)-1){ //if the sbrk fails make it error
            errno = ENOMEM;
            return NULL;
        }
    }

    void* moreHeapChunkStart = sbrk(STANDARD_HEAP_SIZE); //another call to sbrk will allocate 64KB of memory and store where the new heap starts
    if(moreHeapChunkStart == (void*)-1){ 
        errno = ENOMEM;
        return NULL;
    }

    //set all of the chunk's fields

    ChunkHeader *newHeapChunk = (ChunkHeader*)moreHeapChunkStart; //casting the void pointer to a chunkHeader pointer
    //this essentially puts the first node in the chunk of memory because the start of the memory chunk points to the chunk header
    uint64_t new_calculated_size = STANDARD_HEAP_SIZE - CHUNK_HEADER;

    printf("getting more memory, new calculated size: %zu\n", new_calculated_size);

    newHeapChunk->size = make_16(new_calculated_size);

    printf("the new heap chunk's size: %zu\n", newHeapChunk->size);

    newHeapChunk->is_it_free = 1; //yes this chunk is free

    //set the previous pointer to what the last node was in the list to add it to the linked list properly
    ChunkHeader* current_chunk = startOfHeap;



   if (startOfHeap->next == NULL) { //if there is only one node in the list, then it is the last one. 
        printf("Heap is currently empty, need to add on the new heap to this heap.\n");
        newHeapChunk->prev = startOfHeap;
        startOfHeap->next = newHeapChunk;
        newHeapChunk->next = NULL; //next is NULL since it is now the last in the list
       
    } else {
        while (current_chunk->next != NULL) { //
            printf("BOOB");
            current_chunk = current_chunk->next;  //go until the last chunk in the current linked list
        }   //now that the current_chunk is the last one in the list, update the pointers

        newHeapChunk->prev = current_chunk;
        current_chunk->next = newHeapChunk;
        newHeapChunk->next = NULL; //next is NULL since it is now the last in the list
    }

    printf("did it get to the end of get heap\n");
    return newHeapChunk;
}


void* my_malloc(uint64_t requested_amount){
    int successFlag = 0; //starts off as no memory found so not successful
    void* ptr_to_user_memory = NULL;

    requested_amount = make_16(requested_amount); //make sure what they ask for is aligned

    if (startOfHeap == NULL){
        if (initialize_heap() == NULL) {
            return NULL;  //return NULL
        }
    }

        while(successFlag == 0){
            ChunkInfo chunkInfo = find_free_chunk(startOfHeap, requested_amount);

            if (chunkInfo.enough_space == 1){
                int result = split_chunk(chunkInfo);
                    if(result == 0 || result == 1){
                        ptr_to_user_memory = (void*)((uintptr_t)chunkInfo.start_of_free_chunk_ptr + sizeof(ChunkHeader));
                        //where the usable memory starts is where the start of the free chunk pointer + size of the chunk header. 
                        successFlag = 1;
                        return ptr_to_user_memory;  //yay return that to the user
                    }
                    else if (result == -1){ //not enough space in this chunk to allocate
                        printf("REACHING HERE??\n");
                        //need to use up what is left of the first chunk then allocate from the second chunk
                        //can allocate whatevers left of the free chunk
                        uint64_t allocated_already = chunkInfo.start_of_free_chunk_ptr->size;
                        uint64_t left_to_allocate = (requested_amount - allocated_already);


                        printf("Allocated already: %" PRIu64 " bytes, Left to allocate: %" PRIu64 " bytes, Requested Amount: %" PRIu64 " bytes\n",
                        allocated_already, left_to_allocate, requested_amount);

                        chunkInfo.start_of_free_chunk_ptr->is_it_free = 0; //since that chunk was fully used, mark it as used

                        //grab a new chunk now
                        ChunkHeader* new_heap_chunk = get_more_heap();
                        if (new_heap_chunk == NULL) {
                            return NULL;  //can't get more memory
                        }

                        ChunkInfo newChunkInfo = find_free_chunk(new_heap_chunk, left_to_allocate);
                        int leftoverResult = split_chunk(newChunkInfo);

                        //return the spot in the new chunk where the leftover memory was allocated and stopped. 
                        if(leftoverResult == 0 || leftoverResult == 1){
                            ptr_to_user_memory = (void*)((uintptr_t)newChunkInfo.start_of_free_chunk_ptr + sizeof(ChunkHeader));
                            return ptr_to_user_memory;
                        }

                    }
            }
            else if (chunkInfo.enough_space == 0){
                ChunkHeader* new_heap_chunk = get_more_heap();
                if (new_heap_chunk == NULL) {
                    return NULL;  //can't get more memory
                }
            }
        }
        return ptr_to_user_memory;
}





