C = gcc
CFLAGS = -Wall -g -fPIC

.PHONY: run clean

all: intel-all malloc hello main_prog

libmalloc.so: malloc.o
	$(CC) $(CFLAGS) -shared -o libmalloc.so malloc.o

malloc.o: malloc.c intel-all
	$(CC) $(CFLAGS) -c malloc.c

malloc: libmalloc.so
	@echo "Libraries built."


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

hello: hello.o libmalloc.so 
	$(CC) -L./lib64 -o hello hello.o -lmalloc
    
hello.o: hello.c
	$(CC) -Wall -g -c -o hello.o hello.c

#want to run my own stuff
main_prog: main.o libmalloc.so 
	$(CC) -g -L./lib64 -o main main.o -lmalloc
    
main.o: main.c
	$(CC) -Wall -g -c -o main.o main.c

run: main
	./main

clean:
	rm -rf *.o *.a *.so hello lib lib64
