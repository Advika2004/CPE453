#include "malloc.h"

//buffer for printing stuff
static char buf[BUFFER_SIZE];

//start of the heap
ChunkHeader *startOfHeap = NULL; 

//will align to 16 byte boundary
size_t make_16(size_t number){
    return (number + 15) & -16;
}

//initialize the heap
void* initialize_heap(){
    //where the linked list is going to start
    //global variable that I am updating
    startOfHeap = sbrk(0); 

    //check if that failed
    if(startOfHeap == (void*)-1){ 
        errno = ENOMEM;
        return NULL;
    }
    
    //check if divisible by 16
    //cast to int to do math on it
    //adjust pointer accordingly so its aligned
    //move the program break that many bytes forward 
    //until the next multiple of 16
    uintptr_t mathableAddy = (uintptr_t)startOfHeap;
    if((mathableAddy % 16) != 0){
        uintptr_t byteAdjustment = 16 - (mathableAddy % 16); 
        void* AlignedStartOfHeap = sbrk(byteAdjustment); 

        //check if that fails
        if(AlignedStartOfHeap == (void*)-1){ 
            //if the sbrk fails make it error
            errno = ENOMEM;
            return NULL;
        }
    }

    //first call to sbrk will allocate 64KB of memory
    void* firstChunkStart = sbrk(STANDARD_HEAP_SIZE); 

    //check if that fails too
    if(firstChunkStart == (void*)-1){ 
        errno = ENOMEM;
        return NULL;
    }

    //cast the void pointer to a chunkHeader pointer
    //puts the first node in the chunk of memory 
    //bc start of memory chunk points to chunk header
    //size available = total - header
    ChunkHeader *firstChunk = (ChunkHeader*)firstChunkStart; 
    size_t calculated_size = STANDARD_HEAP_SIZE - CHUNK_HEADER;

    //make sure user size is aligned
    firstChunk->size = make_16(calculated_size);

    //set parameters of struct
    //first node
    //yes is free
    firstChunk->is_it_free = 1; 
    firstChunk->prev = NULL;
    firstChunk->next = NULL;

    //make sure fields get assigned properly

    //ptr to where chunk header starts
    return firstChunk; 
}




ChunkInfo find_free_chunk(ChunkHeader* currChunk, size_t reqSize){ 
    //initialzie struct on stack
    ChunkInfo chunk_info;

    //make sure user's ask is aligned
    reqSize = make_16(reqSize); 

    //start off with there is no chunk_info
    chunk_info.enough_space = 0;
    chunk_info.amount_asked_for = 0;
    chunk_info.start_of_free_chunk_ptr = NULL;

    //if its a free chunk and enough space
    //yes there is enough space
    //start_of_free_chunk_ptr is set to current free chunk of the right size
    while (currChunk != NULL){
        if (currChunk->is_it_free == 1 && currChunk->size >= reqSize){ 
            chunk_info.enough_space = 1; 
            chunk_info.start_of_free_chunk_ptr = currChunk; 
            chunk_info.amount_asked_for = reqSize; 
            return chunk_info;
        }
        else{ //else move onto the next chunk
            currChunk = currChunk->next;
        }
    }
    return chunk_info;   
    //will have zero and null if nothing is found
}


int split_chunk(ChunkInfo chunk_info){
    //current free chunk will be allocated
    //edit that header and make it not free
    ChunkHeader *split_allocated = chunk_info.start_of_free_chunk_ptr;  
   
    // check if user asks for more than I have
    // not enough space, return -1
    // do not allocate anything right now
    // this case will be handled in the malloc
    if (chunk_info.amount_asked_for + sizeof(ChunkHeader) + 16 > split_allocated->size) {
        return -1; 
    }

    //calculate how much free space is left 
    //(current chunk size - requested size - sizeof(ChunkHeader)) + 16
    size_t user_free_space_left = split_allocated->size 
                                  - chunk_info.amount_asked_for 
                                  - sizeof(ChunkHeader);


    //if not enough in the free chunk for user to use with min amount
    //mark entire chunk as allocated
    if (user_free_space_left <= sizeof(ChunkHeader) + 16){
        split_allocated->size = chunk_info.amount_asked_for;
        split_allocated->is_it_free = 0; 
        return 0; //not split, whole chunk used
    }

    //if there is enough space, then allocate the chunk and split it
    split_allocated->size = chunk_info.amount_asked_for;
    split_allocated->is_it_free = 0;

    //calculate where new chunk starts (should be where next header starts)
    //next header starts after the amount allocated ends.
    //make sure that it is lined up with 16 
    //make new node
    size_t free_chunk_start = make_16((((uintptr_t)split_allocated) 
    + chunk_info.amount_asked_for + sizeof(ChunkHeader)));
    ChunkHeader* split_free = (ChunkHeader*)free_chunk_start;

    //new node is free
    //user space left was calculated above
    split_free->is_it_free = 1;
    split_free->size = user_free_space_left;

    //the free chunk is next and the allocated chunk is first
    split_free->prev = split_allocated;
    split_free->next = split_allocated->next;
    split_allocated->next = split_free;

    // If there's a chunk after the new free chunk..
    //its previous is now split_free and not split_allocated
    //split_free->next is what used to be the big chunk's next 
    //so split_free->next->prev is the big chunk's next's previous
    // (refer to drawing on ipad)
    if (split_free->next != NULL) {
        split_free->next->prev = split_free; 
    }

    return 1; //it was split
}

void* get_more_heap(){

    //tells me where the new heap chunk will start
    void* startOfNewHeap = sbrk(0); 

    //check if it fails
    if(startOfNewHeap == (void*)-1){ 
        errno = ENOMEM;
        return NULL;
    }

    //check if aligned
    //if not do same as init heap
    //make it mathable then adjust pointer
    uintptr_t mathableNewAddy = (uintptr_t)startOfNewHeap;
    if((mathableNewAddy % 16) != 0){
        uintptr_t byteNewAdjustment = 16 - (mathableNewAddy % 16); 

        //move the program break that many bytes forward 
        //until the next multiple of 16
        void* AlignedStartOfNewHeap = sbrk(byteNewAdjustment); 

        //if the sbrk fails make it error
        if(AlignedStartOfNewHeap == (void*)-1){ 
            errno = ENOMEM;
            return NULL;
        }
    }

    //another call to sbrk will allocate 64KB of memory 
    //pointer will store where the new heap starts
    void* moreHeapChunkStart = sbrk(STANDARD_HEAP_SIZE);

    //check if it fails
    if(moreHeapChunkStart == (void*)-1){ 
        errno = ENOMEM;
        return NULL;
    }
    
    //casting the void pointer to a chunkHeader pointer
    //puts the first node in the chunk of memory 
    //find how much more memory the user can have
    ChunkHeader *newHeapChunk = (ChunkHeader*)moreHeapChunkStart; 
    size_t new_calculated_size = STANDARD_HEAP_SIZE - CHUNK_HEADER;

    //make sure it is aligned
    newHeapChunk->size = make_16(new_calculated_size);

    //yes this chunk is free
    newHeapChunk->is_it_free = 1; 

    //use the global variable to find the first node in linked list
    ChunkHeader* current_chunk = startOfHeap;


    //if there is only one node in the list, then it is the last one.
    //next of the new chunk is NULL since it is now the last in the list
    if (startOfHeap->next == NULL) {  
        newHeapChunk->prev = startOfHeap;
        startOfHeap->next = newHeapChunk;
        newHeapChunk->next = NULL; 
    } 

    //go until the last chunk in the current linked list
    //adjust the pointers to add the new chunk to the end
    else {
        while (current_chunk->next != NULL) { 
            current_chunk = current_chunk->next;  
        }  
        newHeapChunk->prev = current_chunk;
        current_chunk->next = newHeapChunk;
        newHeapChunk->next = NULL; 
    }

    return newHeapChunk;
}

//recursive function to keep allocating what is left over
void* allocate_leftover(size_t left_to_allocate) {


    //allocate a new chunk
    ChunkHeader* new_heap_chunk = get_more_heap();

    //check if that fails
    if (new_heap_chunk == NULL) {
        return NULL;
    }

    //try to fit the remaining data in the new chunk
    ChunkInfo newChunkInfo = find_free_chunk(new_heap_chunk, left_to_allocate);
    int leftoverResult = split_chunk(newChunkInfo);

    // if it fits, return the pointer to the allocated space
    if (leftoverResult == 0 || leftoverResult == 1) {
        return (void*)((uintptr_t)newChunkInfo.start_of_free_chunk_ptr 
        + sizeof(ChunkHeader));
    }

    //still not enough space, recursively allocate again until enough space
    if (leftoverResult == -1) {
        // Calculate how much is left to allocate after using this chunk
        size_t allocated_already = newChunkInfo.start_of_free_chunk_ptr->size;
        size_t new_left_to_allocate = left_to_allocate - allocated_already;

        //call allocate_leftover for the remaining space
        return allocate_leftover(new_left_to_allocate);
    }
    //should never reach here
    return NULL;
}


void* malloc(size_t requested_amount){

    //check if malloc is linking and running
    snprintf(buf, BUFFER_SIZE, "MALLOC IS BEING CALLED:\n");
    write(STDOUT_FILENO, buf, strlen(buf));

    //starts off as no memory found so not successful
    int successFlag = 0; 

    //what malloc will return eventually
    void* ptr_to_user_memory = NULL;

    //make sure what they ask for is aligned
    requested_amount = make_16(requested_amount);

    //if there is no chunk yet, make one
    if (startOfHeap == NULL){
        initialize_heap();
    }

    //while there is no memory found for the user to use
    //traverse and find a free node
    while(successFlag == 0){
        ChunkInfo chunkInfo = find_free_chunk(startOfHeap, requested_amount);

        //if a big enough chunk is found
        if (chunkInfo.enough_space == 1){

            //then split that chunk up into allocated and what's free left over
            int result = split_chunk(chunkInfo);

            //if the whole chunk was allocated
            //need to grab a new chunk and return the start of that to user
            if (result == 0) {

                //allocate a new chunk
                ChunkHeader* got_more_chunk = get_more_heap();
    
                //check if that fails
                if (got_more_chunk == NULL) {
                    return NULL;
                }

                //return start of the new chunk's usable memory (after header)
                //it was successfully mallocked
                //return ptr to user
                ptr_to_user_memory = (void*)((uintptr_t)got_more_chunk 
                + sizeof(ChunkHeader));
                successFlag = 1;
                return ptr_to_user_memory;
            } 
            
            //if the chunk was split up and allocated
            //return pointer to the split_free header
            else if (result == 1) {
                ptr_to_user_memory = (void*)((uintptr_t)
                chunkInfo.start_of_free_chunk_ptr + sizeof(ChunkHeader));
                successFlag = 1;
                return ptr_to_user_memory;  // Return that to the user
            }

            //need to allocate more and split
            //If no chunk had enough space (enough_space == 0) 
            //or if the result was -1 (nothing was allocated bc too small)
            else if (chunkInfo.enough_space == 0 || result == -1) {

                //allocate the entire thing
                //calculate how much spills over for the new chunk
                size_t allocated_already = 
                chunkInfo.start_of_free_chunk_ptr->size;
                size_t left_to_allocate = requested_amount - allocated_already;

            
                //say current chunk is fully used
                chunkInfo.start_of_free_chunk_ptr->is_it_free = 0; 

                //call allocate leftover to allocate what is left
                void* leftover_ptr = allocate_leftover(left_to_allocate);

                //successful, return the pointer
                if (leftover_ptr != NULL) {
                    ptr_to_user_memory = leftover_ptr;
                    successFlag = 1;
                    return ptr_to_user_memory;
                }
                else {
                    return NULL;
                }
            }
        }
    }       
    return ptr_to_user_memory;
}