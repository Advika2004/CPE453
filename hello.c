#include "malloc.h"
#include "malloc.c"

int main(void) {
    void* p = malloc(0);
    snprintf(buf, BUFFER_SIZE, "main%p", p);
    puts(buf);
    return 0;
}
