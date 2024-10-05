C = gcc
CFLAGS = -Wall -g -fPIC

.PHONY: run clean

all: intel-all malloc 

libmalloc.so: malloc.o
	$(CC) $(CFLAGS) -shared -o libmalloc.so malloc.o

libmalloc.a: malloc.o
	ar rcs libmalloc.a malloc.o

malloc.o: malloc.c intel-all
	$(CC) $(CFLAGS) -c malloc.c

malloc: libmalloc.so libmalloc.a
	@echo "Malloc Libraries built."

intel-all: lib/libmalloc.so lib64/libmalloc.so

lib/libmalloc.so: lib malloc32.o
	$(CC) $(CFLAGS) -m32 -shared -o $@ malloc32.o

lib64/libmalloc.so: lib64 malloc64.o
	$(CC) $(CFLAGS) -shared -o $@ malloc64.o
                            
lib:
	mkdir lib

lib64:
	mkdir lib64

malloc32.o: malloc.c
	$(CC) $(CFLAGS) -m32 -c -o malloc32.o malloc.c

malloc64.o: malloc.c
	$(CC) $(CFLAGS) -m64 -c -o malloc64.o malloc.c

clean:
	rm -rf *.o *.a *.so  lib lib64
