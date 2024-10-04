#include "malloc.h"

//buffer for printing stuff
static char buf[BUFFER_SIZE];

//start of the heap
ChunkHeader *startOfHeap = NULL; 

//will align to 16 byte boundary
size_t make_16(size_t number){
    return (number + 15) & ~0xF;
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
        if (currChunk->is_it_free == 1 && currChunk->size >= (reqSize
         + sizeof(ChunkHeader) + 16)){ 
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
}

//will only be called when it is splittalbe
void split_chunk(ChunkInfo chunk_info){

    //chunk that will be allocated
    ChunkHeader *split_allocated = chunk_info.start_of_free_chunk_ptr; 

    //user will never ask for more than what I have
    //no longer need that check

    //calculate how much free space is left 
    //(current chunk size - requested size - sizeof(ChunkHeader)) + 16
    // offset for addr of new chunk 
    size_t user_free_space_left = split_allocated->size 
                                  - chunk_info.amount_asked_for 
                                  - sizeof(ChunkHeader);


    //if there is enough space, then allocate the chunk and split it
    split_allocated->size = chunk_info.amount_asked_for;
    split_allocated->is_it_free = 0;

    //calculate where new chunk starts (should be where next header starts)
    //next header starts after the amount allocated ends.
    //make sure that it is lined up with 16 
    //make new node
    uintptr_t free_chunk_start = (uintptr_t)split_allocated 
                        + chunk_info.amount_asked_for + sizeof(ChunkHeader);
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
    //if the free's next is NULL
    return;
}

void* get_more_heap(size_t amount_to_allocate){

    //tells me where the new heap chunk will start
    void* startOfNewHeap = sbrk(0); 

    //check if it fails
    if(startOfNewHeap == (void*)-1){ 
        errno = ENOMEM;
        return NULL;
    }

    int multiplier = (amount_to_allocate / STANDARD_HEAP_SIZE) + 1;

    //another call to sbrk will allocate 64KB of memory 
    //pointer will store where the new heap starts
    void* moreHeapChunkStart = sbrk(multiplier * STANDARD_HEAP_SIZE);

    //check if it fails
    if(moreHeapChunkStart == (void*)-1){ 
        errno = ENOMEM;
        return NULL;
    }
    
    //casting the void pointer to a chunkHeader pointer
    //puts the first node in the chunk of memory 
    //find how much more memory the user can have
    ChunkHeader *newHeapChunk = (ChunkHeader*)moreHeapChunkStart; 
    size_t new_calculated_size = amount_to_allocate - CHUNK_HEADER;

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

void* malloc(size_t requested_amount){
    //what malloc will return eventually
    void* ptr_to_user_memory = NULL;

    if(requested_amount == 0){
        return NULL;
    }

    //make sure what they ask for is aligned
    requested_amount = make_16(requested_amount);

    //if there is no chunk yet, make one
    if (startOfHeap == NULL){
        initialize_heap();
    }

    //while there is no memory found for the user to use
    //traverse and find a free node
    ChunkInfo chunkInfo = find_free_chunk(startOfHeap, requested_amount);

    //if a big enough chunk is found
    if (chunkInfo.enough_space == 1){
        split_chunk(chunkInfo);
        return (uintptr_t)chunkInfo.start_of_free_chunk_ptr 
                + (uintptr_t)sizeof(ChunkHeader);
    }

    else{
    //allocate a new chunk

        ChunkHeader* got_more_chunk = 
        get_more_heap(requested_amount);
    
        //check if that fails
        if (got_more_chunk == NULL) {
            return NULL;
        }

        //return start of the new chunk's usable memory (after header)
        //it was successfully mallocked
        //return ptr to user
        ptr_to_user_memory = (void*)((uintptr_t)got_more_chunk 
        + sizeof(ChunkHeader));

        return ptr_to_user_memory;

    }

    return NULL;
}

void* calloc(size_t num_elements, size_t element_size){
    //total memory needed is how many elements * the size of them
    size_t total_mem_wanted = num_elements * element_size;
    //return pointer to where the allocated memory is
    void* got_mallocked = malloc(total_mem_wanted);

    //check if that failed
    if(got_mallocked == NULL){
        return NULL;
    }

    //start at pointer of where the allocated memory is
    //set all of it to 0
    //do that for the total memory wanted
    memset(got_mallocked, 0, total_mem_wanted);

    //return the same pointer
    return got_mallocked;
}

ChunkHeader* combine_free_chunks(ChunkHeader* currChk){
    //make sure both chunks are not NULL

    ChunkHeader* prevChunk = currChk->prev;
    ChunkHeader* nextChunk = currChk->next;
    size_t combined_size = 0;
    
    if(currChk == NULL){
        return NULL;
    }

    //case 1: the one before is free and current is free
    //the current chunk gets merged to the previous chunk
    //turns into one big previous chunk
    if((prevChunk != NULL) && (currChk->is_it_free == 1)
     && (prevChunk->is_it_free == 1)){
        combined_size = currChk->size + prevChunk->size + sizeof(ChunkHeader);

        if (combined_size >= sizeof(ChunkHeader) + 16){
            prevChunk->size = combined_size;
            prevChunk->next = currChk->next;

            if (currChk->next != NULL) {
                currChk->next->prev = prevChunk;
            }
            currChk = prevChunk;
        }
    }

    //case 2: one after is free and current is free
    //the next chunk gets added to the current
    if((nextChunk != NULL) && (currChk->is_it_free == 1)
     && (nextChunk->is_it_free == 1)){
        combined_size = currChk->size + nextChunk->size + sizeof(ChunkHeader);

        if (combined_size >= sizeof(ChunkHeader) + 16){
            currChk->size = combined_size;
            currChk->next = nextChunk->next;

            if (nextChunk->next != NULL) {
                nextChunk->next->prev = currChk;
            }
        }
    }

    //case 3: before after and current are all free
    if((prevChunk != NULL) && (nextChunk != NULL) && (currChk->is_it_free == 1)
     && (nextChunk->is_it_free == 1) && (prevChunk->is_it_free == 1)){
        combined_size = currChk->size + nextChunk->size + prevChunk->size
         + sizeof(ChunkHeader) + sizeof(ChunkHeader);

        if (combined_size >= sizeof(ChunkHeader) + 16){
            prevChunk->size = combined_size;
            prevChunk->next = nextChunk->next; 

            if (nextChunk->next != NULL) {
                nextChunk->next->prev = prevChunk;
            }

            currChk = prevChunk;
        }
    }
    return currChk; //combining chunks returns the pointer to the larger chunk
}

ChunkHeader* find_which_chunk(void* ptr){
    //start at the top of the linked list
    ChunkHeader* currChunk = startOfHeap;

    //while I am not at the end of the list
    while(currChunk != NULL){

        //calculate where the chunk actually starts 
        void* chunk_data_start = (void*)((uintptr_t)currChunk
         + sizeof(ChunkHeader));
        void* chunk_data_end = (void*)((uintptr_t)chunk_data_start
         + currChunk->size);
    
        //if it is pointing to the header, return NULL
        //that should not be possible bc malloc will always return actual chunk
        if(ptr == currChunk->next){
            return NULL;
        }

        //if it points to the first header then thats okay bc thats the chunk
        if(ptr == currChunk){
            return currChunk;
        }
        
        //if the pointer is between 2 chunks
        if (ptr >= chunk_data_start && ptr < chunk_data_end){ 
            return currChunk;
        }
        //else move onto the next chunk
        else{ 
            currChunk = currChunk->next;
        }
    }
    return NULL;
}

void free(void* ptr){
    if(ptr == NULL){
        return;
    }

    //finds which chunk the pointer is pointing to
    ChunkHeader* chunk2free = find_which_chunk(ptr);

    //check if the chunk found was valid
    if (chunk2free == NULL) {
        return;
    }
    //if valid then set it as free
    chunk2free->is_it_free = 1;

    //do the combining checks woohoo
    combine_free_chunks(chunk2free);

    return;
}

//function to find what is left over and make that its own chunk
//takes in the 
ChunkHeader* create_leftover_chunk(ChunkHeader* chunk, size_t new_size) {

    new_size = make_16(new_size);

    //the leftover space is the chunk's size - the new size
    size_t leftover_space = chunk->size - new_size;

    //if the leftover space is big enough to make a chunk out of
    //the new chunk made from what is leftover will start at:
    //the current chunk + the new size used
    if(leftover_space >= sizeof(ChunkHeader) + 16){
    ChunkHeader* leftover_free_chunk_header = (ChunkHeader*)
    ((uintptr_t)chunk + + sizeof(ChunkHeader) + new_size);
        
        // !could be causing segfault
        // //check if the one after that is free too 
        // if(leftover_free_chunk_header->next->is_it_free == 1){
        //     leftover_free_chunk_header = 
        //     combine_free_chunks(leftover_free_chunk_header);
        // }

        //if the one after is not free then I dont need to combine
        //just need to add the new node to end of the list
        //update the new node's fields 
        leftover_free_chunk_header->next = chunk->next;

        if (chunk->next != NULL) {
            chunk->next->prev = leftover_free_chunk_header;
        }
        chunk->next = leftover_free_chunk_header;
        leftover_free_chunk_header->prev = chunk;
        leftover_free_chunk_header->is_it_free = 1; 
        chunk->is_it_free = 0;
        chunk->size = new_size;
        leftover_free_chunk_header->size = leftover_space - 
        sizeof(ChunkHeader);

        return leftover_free_chunk_header;
        //returning header. will need to calculate where data stars in realloc
    }
    //if its not big enough 
    //so in realloc, check if this function returns NULL
    //if it does, then I have to call allocate_new_chunk
    return NULL;
}

//function to allocate a new chunk and move data
void* allocate_new_chunk_and_copy(void* ptr, size_t new_size) {
    void* new_chunk = malloc(new_size);

    //check if the malocking failed
    if (new_chunk == NULL) {
        return NULL;
    }

    // Copy data from old chunk to new chunk
    memmove(new_chunk, ptr, new_size);

    // Free the old chunk
    free(ptr);

    // Return pointer to the user data part of the new chunk
    return new_chunk;
}

void* realloc(void* ptr, size_t new_size){

    //make sure the new size wanted is aligned
    new_size = make_16(new_size);

    //check if the pointer given is NULL just call malloc
    if(ptr == NULL){
        return malloc(new_size);
    }
        
    //check if the size given is zero
    if(new_size == 0){
        free(ptr);
        return NULL;
    }

    //need to calculate where the header starts
    ChunkHeader* theHeaderStarts;

    //go backwards by the size of the chunk header to find the header
    //get the size of that chunk
    theHeaderStarts = (ChunkHeader*)((uintptr_t)ptr
     - (uintptr_t)sizeof(ChunkHeader));
    size_t current_chunk_size = theHeaderStarts->size;

    //SHRINKING: 
    //case 1: new size is smaller than current chunk size
    //allocate what I need by updating the header
    //make the leftover space into its own node
    //combine it with other free stuff
    //return pointer to where data starts 

    if(new_size < current_chunk_size){

        ChunkHeader* leftoverChunk = 
        create_leftover_chunk(theHeaderStarts, new_size);

        //that means a left over chunk was made into its own chunk. 
        //return the og ptr

        return ptr;
    }
    return ptr;
}

    // //DO NOTHING:
    // //it is the same size so just return the same pointer
    // else if (new_size == current_chunk_size) {
    //     return ptr;
    // }

    // //GROWING:
    // //new size is larger than current and the next one is free
    // //and that is large enough
    // //combine the chunks, allocate what you need
    // //make new chunk with leftover
    // //check if other is free next to it
    // //combine that too
    // //return pointer to first node

    // else if(new_size >= current_chunk_size){ //if size larger than current


    //     //check if the next chunk is free and not NULL
    //     ChunkHeader* nextdoorChunk = theHeaderStarts->next;

    //     //check if the next chunk is free
    //     if(nextdoorChunk != NULL && nextdoorChunk->is_it_free == 1){

    //         //check if combining them will make it large enough
    //         //then allocate it
    //         if((current_chunk_size + nextdoorChunk->size + 
    //             sizeof(ChunkHeader) + 16) >= new_size){
                
    //             //combine the chunks 
    //             combine_free_chunks(theHeaderStarts);

    //             //if there is a leftover part, then make a chunk out of it
    //             create_leftover_chunk(theHeaderStarts, new_size);
    //             return ptr;
    //         }
    //     }

    //         //if the next chunk is NULL
    //         //or if the next chunk is not free
    //         //or if combining the chunks doesn't make it large enough
    //         //get more heap
    //         //combine it with the current chunk
    //         //create a leftover chunk
    //         get_more_heap(new_size);
    //         ChunkHeader* biggerFreeChunk=combine_free_chunks(nextdoorChunk);
    //         create_leftover_chunk(biggerFreeChunk, new_size);
    //         return ptr;
    //     }  
    //     return NULL;
    // }