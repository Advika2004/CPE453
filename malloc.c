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
    if (chunk_info.amount_asked_for + sizeof(ChunkHeader) 
    + 16 > split_allocated->size) {
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
    snprintf(buf, BUFFER_SIZE, "Calling malloc succeeded:\n");
    write(STDOUT_FILENO, buf, strlen(buf));

    //starts off as no memory found so not successful
    int successFlag = 0; 

    //what malloc will return eventually
    void* ptr_to_user_memory = NULL;

    //make sure what they ask for is aligned
    requested_amount = make_16(requested_amount);

    snprintf(buf, BUFFER_SIZE, 
    "Requested amount (aligned to 16): %zu\n", requested_amount);
    write(STDOUT_FILENO, buf, strlen(buf));

    //if there is no chunk yet, make one
    if (startOfHeap == NULL){

        snprintf(buf, BUFFER_SIZE, "Initializing heap.\n");
        write(STDOUT_FILENO, buf, strlen(buf));

        initialize_heap();
    }

    //while there is no memory found for the user to use
    //traverse and find a free node
    while(successFlag == 0){

        snprintf(buf, BUFFER_SIZE, "Finding free chunk.\n");
        write(STDOUT_FILENO, buf, strlen(buf));

        ChunkInfo chunkInfo = find_free_chunk(startOfHeap, requested_amount);

        //if a big enough chunk is found
        if (chunkInfo.enough_space == 1){

            snprintf(buf, BUFFER_SIZE, "Found a chunk with enough space.\n");
            write(STDOUT_FILENO, buf, strlen(buf));

            //then split that chunk up into allocated and what's free left over
            int result = split_chunk(chunkInfo);

            //if the whole chunk was allocated
            //need to grab a new chunk and return the start of that to user
            if (result == 0) {

                snprintf(buf, BUFFER_SIZE, 
                "Entire chunk allocated. Getting more heap.\n");
                write(STDOUT_FILENO, buf, strlen(buf));

                //allocate a new chunk
                ChunkHeader* got_more_chunk = get_more_heap();
    
                //check if that fails
                if (got_more_chunk == NULL) {

                    snprintf(buf, BUFFER_SIZE, 
                    "Error: Could not allocate more memory.\n");
                    write(STDOUT_FILENO, buf, strlen(buf));

                    return NULL;
                }

                //return start of the new chunk's usable memory (after header)
                //it was successfully mallocked
                //return ptr to user
                ptr_to_user_memory = (void*)((uintptr_t)got_more_chunk 
                + sizeof(ChunkHeader));
                successFlag = 1;

                snprintf(buf, BUFFER_SIZE, 
                "Returning new chunk pointer: %p\n", ptr_to_user_memory);
                write(STDOUT_FILENO, buf, strlen(buf));

                return ptr_to_user_memory;
            } 
            
            //if the chunk was split up and allocated
            //return pointer to the split_free header
            else if (result == 1) {

                snprintf(buf, BUFFER_SIZE, 
                "Chunk split successfully. Allocating memory.\n");
                write(STDOUT_FILENO, buf, strlen(buf));

                ptr_to_user_memory = (void*)((uintptr_t)
                chunkInfo.start_of_free_chunk_ptr + sizeof(ChunkHeader));
                successFlag = 1;

                snprintf(buf, BUFFER_SIZE, 
                "Returning pointer after split: %p\n", ptr_to_user_memory);
                write(STDOUT_FILENO, buf, strlen(buf));

                return ptr_to_user_memory;  // Return that to the user
            }

            //need to allocate more and split
            //If no chunk had enough space (enough_space == 0) 
            //or if the result was -1 (nothing was allocated bc too small)
            else if (chunkInfo.enough_space == 0 || result == -1) {

                snprintf(buf, BUFFER_SIZE, 
                "Not enough space in current chunk. Allocating new chunk.\n");
                write(STDOUT_FILENO, buf, strlen(buf));

                //allocate the entire thing
                //calculate how much spills over for the new chunk
                size_t allocated_already = 
                chunkInfo.start_of_free_chunk_ptr->size;
                size_t left_to_allocate = requested_amount - allocated_already;

                snprintf(buf, BUFFER_SIZE, 
                "Allocated already: %zu, Left to allocate: %zu\n", 
                allocated_already, left_to_allocate);
                write(STDOUT_FILENO, buf, strlen(buf));
            
                //say current chunk is fully used
                chunkInfo.start_of_free_chunk_ptr->is_it_free = 0; 

                //call allocate leftover to allocate what is left
                void* leftover_ptr = allocate_leftover(left_to_allocate);

                //successful, return the pointer
                if (leftover_ptr != NULL) {
                    ptr_to_user_memory = leftover_ptr;
                    successFlag = 1;

                    snprintf(buf, BUFFER_SIZE, 
                    "Allocated leftover memory. Returning pointer: %p\n",
                     ptr_to_user_memory);
                    write(STDOUT_FILENO, buf, strlen(buf));

                    return ptr_to_user_memory;
                }
                else {
                    
                    snprintf(buf, BUFFER_SIZE, 
                    "Error: Could not allocate leftover memory.\n");
                    write(STDOUT_FILENO, buf, strlen(buf));

                    return NULL;
                }
            }
        }
    }       
    return ptr_to_user_memory;
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
        prevChunk->size = combined_size;
        prevChunk->next = currChk->next;

        if (currChk->next != NULL) {
            currChk->next->prev = prevChunk;
        }
        currChk = prevChunk;
    }

    //case 2: one after is free and current is free
    //the next chunk gets added to the current
    if((nextChunk != NULL) && (currChk->is_it_free == 1)
     && (nextChunk->is_it_free == 1)){
        combined_size = currChk->size + nextChunk->size + sizeof(ChunkHeader);
        currChk->size = combined_size;
        currChk->next = nextChunk->next;

        if (nextChunk->next != NULL) {
            nextChunk->next->prev = currChk;
        }
    }

    //case 3: before after and current are all free
    if((prevChunk != NULL) && (nextChunk != NULL) && (currChk->is_it_free == 1)
     && (nextChunk->is_it_free == 1) && (prevChunk->is_it_free == 1)){
        combined_size = currChk->size + nextChunk->size + prevChunk->size
         + sizeof(ChunkHeader) + sizeof(ChunkHeader);
        prevChunk->size = combined_size;
        prevChunk->next = nextChunk->next; 

        if (nextChunk->next != NULL) {
            nextChunk->next->prev = prevChunk;
        }

        currChk = prevChunk;
    }
    return currChk;
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

void print_heap() {
    ChunkHeader *current_chunk = startOfHeap;

    if (current_chunk == NULL) {
        snprintf(buf, BUFFER_SIZE, "Heap is empty.\n");
        write(STDOUT_FILENO, buf, strlen(buf));
        return;
    }

    snprintf(buf, BUFFER_SIZE, "Heap structure:\n");
    write(STDOUT_FILENO, buf, strlen(buf));

    // Iterate through all chunks in the heap
    while (current_chunk != NULL) {
        // Print the chunk's memory address, size, and whether it's free or not
        snprintf(buf, BUFFER_SIZE,
                 "Chunk at address: %p\n  Size: %zu bytes\n  Is free: %d\n",
                 (void*)current_chunk, current_chunk->size, current_chunk->is_it_free);
        write(STDOUT_FILENO, buf, strlen(buf));

        // Move to the next chunk in the heap
        current_chunk = current_chunk->next;
    }

    snprintf(buf, BUFFER_SIZE, "End of heap.\n");
    write(STDOUT_FILENO, buf, strlen(buf));
}

void* realloc(void* ptr, size_t new_size){

    ChunkHeader* reallocked_ptr = NULL;
    
    //check if the pointer given is NULL just call malloc
    if(ptr == NULL){
        reallocked_ptr = malloc(new_size);
        return reallocked_ptr;
    }

    //check if the size given is zero
    if(new_size == 0){
        free(ptr);
        return NULL;
    }

    //if the pointer is not null and the size is not zero can truly realloc
    if((ptr != NULL) && (new_size != 0)){

        //need to calculate where the header starts
        ChunkHeader* theHeaderStarts;
        uintptr_t mathableAddy = (uintptr_t)ptr;

        //go backwards by the size of the chunk header to find the header
        //get the size of that chunk
        theHeaderStarts = (ChunkHeader*)(mathableAddy - sizeof(ChunkHeader));
        size_t current_chunk_size = theHeaderStarts->size;

        //case 1: the new_size given is less than current size
        //allocate what I need by updating the header
        if(new_size < current_chunk_size){
            theHeaderStarts->size = new_size;
            theHeaderStarts->is_it_free = 0;

            //find how much the leftover space is
            //make that into a new chunk
            //calculate where the leftover chunk starts
            size_t leftover_space = current_chunk_size - new_size;

            //if the leftover space is big enough to make a chunk out of
            if(leftover_space > sizeof(ChunkHeader) + 16){
                ChunkHeader* leftover_free_chunk = (ChunkHeader*)
                ((uintptr_t)theHeaderStarts 
                + new_size + sizeof(ChunkHeader));

                //check if the one after that is free too
                if(leftover_free_chunk->next){
                    combine_free_chunks(leftover_free_chunk);
                }

                //if the next one is not free, just leave this free chunk alone
                //add new broken off chunk to the linked list and fill its fields
                leftover_free_chunk->next = theHeaderStarts->next;
                theHeaderStarts->next->prev = leftover_free_chunk;
                leftover_free_chunk->prev = theHeaderStarts;
                leftover_free_chunk->is_it_free = 1; 
                leftover_free_chunk->size = leftover_space - sizeof(ChunkHeader);

                //reallocked pointer is theHeaderStarts
                reallocked_ptr = theHeaderStarts;
            }
        }

        //case 2: if realloc is asking for the same size, just return same ptr
        if(new_size == current_chunk_size){
            reallocked_ptr = ptr;
        }

        //case 3: new size is larger than current
        //need to check if the next is free
        else if (new_size > current_chunk_size){
            
            //check if the next chunk is free and not NULL
            ChunkHeader* nextdoorChunk = theHeaderStarts->next;
            if(nextdoorChunk != NULL && nextdoorChunk->is_it_free == 0){
                //check if combining them will make it large enough
                //then allocate it
                if((current_chunk_size + nextdoorChunk->size + sizeof(ChunkHeader)) >= new_size){
                    combine_free_chunks(theHeaderStarts);
                    theHeaderStarts->is_it_free = 0;
                    theHeaderStarts->next = nextdoorChunk->next;
                    theHeaderStarts->size = (current_chunk_size + nextdoorChunk->size + sizeof(ChunkHeader));
                }

                //if combining them will not make it bigger, then get more mem
                //copy stuff over
                //free the old chunk
                else{
                    ChunkHeader* gotnewChunk = malloc(new_size);

                    //check if mallocking that failed
                    if(gotnewChunk == NULL){
                        reallocked_ptr = NULL;
                    }

                    memmove(gotnewChunk, ptr, current_chunk_size);

                    free(ptr);

                    reallocked_ptr = gotnewChunk;
                }
            }
        }
    }
    return reallocked_ptr;
}
