#include "malloc.h"
#include "malloc.c"

int main(void) {
    void* user_space = malloc(0);
    void* user_zero_space = calloc(0, sizeof(int));
    snprintf(buf, BUFFER_SIZE, "main%p", user_space);
    snprintf(buf, BUFFER_SIZE, "main%p", user_zero_space);
    puts(buf);

    free(user_space);
    free(user_zero_space);
    
    return 0;
}
