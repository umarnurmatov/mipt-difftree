#pragma once

#include <stdlib.h>

#include "varinfo.h"
#include "hashutils.h"

static const size_t   CAPACITY_EXP          = 2;
static const unsigned CAPACITY_SHRINK_SCALE = 4;

#ifdef VECTOR_DEBUG
// FIXME varname in the beginning
#define VECTOR_MAKE(VARNAME)           \
    Vector VARNAME = {               \
        .buffer   = NULL,             \
        .size     = 0,                \
        .capacity = 0,                \
        .varinfo = {                  \
            .line     = __LINE__,     \
            .filename = __FILE__,     \
            .funcname = __func__,     \
            .varname  = #VARNAME      \
        }                             \
    }

#define IF_DEBUG(statement) statement

#else

#define VECTOR_MAKE(VARNAME) \
    Vector VARNAME = {     \
        .buffer   = NULL,   \
        .size     = 0,      \
        .capacity = 0       \
    } 

#define IF_DEBUG(statement) 

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

VectorErr vector_ctor(Vector* vec, size_t capacity, size_t tsize);

VectorErr vector_push(Vector* vec, void* val);

void* vector_at(Vector* vec, size_t ind);

VectorErr vector_pop(Vector* vec, void** val);

void vector_dtor(Vector* vec);

