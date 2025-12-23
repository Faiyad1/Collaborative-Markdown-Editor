#ifndef DOCUMENT_H

#define DOCUMENT_H
/**
 * This file is the header file for all the document functions. You will be tested on the functions inside markdown.h
 * You are allowed to and encouraged multiple helper functions and data structures, and make your code as modular as possible. 
 * Ensure you DO NOT change the name of document struct.
 */

typedef struct chunk{
    char* data;
    uint64_t no_of_char;

    int is_new_data;
    int is_deleted;
    struct chunk* next_chunk;
} chunk;

typedef struct document {
    uint64_t version;
    char* data;
    uint64_t no_of_char;

    chunk* starting_chunk;
} document;

// Functions from here onwards.
#endif
