#pragma once

#include <stdlib.h>
#include <stdio.h>

#include "utils.h"
#include "varinfo.h"
#include "hashutils.h"

typedef void (*PrintCallback) (FILE*, void*);

#ifdef _DEBUG

#define VECTOR_INITLIST               \
    {                                 \
        .buffer   = NULL,             \
        .size     = 0,                \
        .capacity = 0,                \
        .varinfo = {                  \
            .line     = __LINE__,     \
            .filename = __FILE__,     \
            .funcname = __func__,     \
        }                             \
    }

#define VECTOR_DUMP(VEC, ERR, MSG, PRNT) \
    vector_dump(stderr, VEC, ERR, MSG, __FILE__, __PRETTY_FUNCTION__, __LINE__, PRNT)

#else

#define VECTOR_INITLIST   \
    {                     \
        .buffer   = NULL, \
        .size     = 0,    \
        .capacity = 0     \
    } 

#endif // _DEBUG

enum VectorErr
{
    VECTOR_ERR_NONE,
    VECTOR_ERR_NULL,
    VECTOR_ERR_BUFFER_NULL,
    VECTOR_ERR_SIZE_EXCEED_CAPACITY,
    VECTOR_ERR_ALLOC_FAIL,
    VECTOR_ERR_OUT_OF_BOUND,
    VECTOR_ERR_HASH_UNMATCH
};

struct Vector
{
    void* buffer;
    size_t size;
    size_t capacity;
    size_t tsize;

    IF_DEBUG(
        const varinfo_t varinfo;

#ifdef HASH_ENABLED
        utils_hash_t buffer_hash;
#endif // HASH_ENABLED
    );

};

VectorErr vector_copy_from(Vector* from, Vector* to);

VectorErr vector_ctor(Vector* vec, size_t capacity, size_t tsize);

VectorErr vector_push(Vector* vec, void* val);

void* vector_at(Vector* vec, size_t ind);

VectorErr vector_pop(Vector* vec, void** val);

void vector_free(Vector* vec);

void vector_dtor(Vector* vec);

const char* vector_strerr(const VectorErr err);

void vector_dump(FILE* stream, Vector* vec, VectorErr err, const char* msg, 
                         const char* filename, const char* funcname, int line, PrintCallback print);

