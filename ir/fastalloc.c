#include "ir.h"

static void ** chunks = NULL;
static size_t chunkslen = 0;

static void * chunk = NULL;
static size_t chunk_left = 0;

/** alloc things that are not meant to be freed before end of compilation */
void * fastalloc(size_t bytes) {
    if (chunk == NULL || bytes > chunk_left) {
        if (chunk) {
	    void * temp = realloc(chunks, sizeof(void*) * (chunkslen + 1));
            if (temp == NULL) {
	        return NULL;
	    }
	    chunks = temp;
	    chunks[chunkslen ++] = chunk;
        }

        size_t new = 1024;
        if (bytes > new)
            new = bytes * 2;
        void * temp = malloc(new);
	if (temp == NULL) {
	    return NULL;
	}
        chunk = temp;
	chunk_left = new;
    }

    chunk_left -= bytes;
    void *alloc = chunk;
    chunk = ((char*)chunk) + bytes;

    return alloc;
}

void * fastrealloc(void * old, size_t oldBytes, size_t newBytes) {
    if (newBytes <= oldBytes) {
        return old;
    }

    size_t diff = newBytes - oldBytes;
    if (((char*)old) + oldBytes == chunk && diff <= chunk_left) {
        (void) fastalloc(diff);
        return old;
    }

    void * new = fastalloc(newBytes);
    if (new == NULL)
        return NULL;
    memcpy(new, old, oldBytes);
    return new;
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

char * faststrdup(const char * str) {
    assert(str);
    size_t len = strlen(str) + 1;
    len *= sizeof(char);
    char * ret = (char*) fastalloc(len);
    assert(ret);
    memcpy(ret, str, len);
    return ret;
}

