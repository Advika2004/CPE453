# CPE453 Project 1 - Malloc Library

This project implements a custom memory management system in C that replicates the standard calloc, malloc, free, and realloc functions, using the sbrk(2) system call. The system manages the heap by maintaining a linked-list-like structure to track allocated and free memory chunks. It dynamically adjusts the program's heap and handles memory allocation requests efficiently, including resizing and freeing memory. The system is implemented as both a shared library and a static archive.
