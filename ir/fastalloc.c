#include "ir.h"

static void ** chunks = NULL;
static size_t chunkslen = 0;

static void * chunk = NULL;
static size_t chunk_left = 0;

/** alloc things that are not meant to be freed before end of compilation */
void * fastalloc(size_t bytes) {
    if (chunk == NULL || bytes > chunk_left) {
        if (chunk) {
            chunks = realloc(chunks, sizeof(void*) * (chunkslen + 1));
            chunks[chunkslen ++] = chunk;
        }

        size_t new = 1024;
        if (bytes > new)
            new = bytes * 2;
        chunk = malloc(new);
        chunk_left = new;
    }

    chunk_left -= bytes;
    void *alloc = chunk;
    chunk = ((char*)chunk) + bytes;

    return alloc;
}

void fastfreeall(void) {
    for (size_t i = 0; i < chunkslen; i ++)
        free(chunks[i]);
    if (chunk)
        free(chunk);
    free(chunks);
    chunks = NULL;
    chunkslen = 0;
}


