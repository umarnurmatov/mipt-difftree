#include "vector.h"

#include <assert.h>
#include <memory.h>

#include "logutils.h"
#include "assertutils.h"

static const size_t   CAPACITY_EXP          = 2;
static const unsigned CAPACITY_SHRINK_SCALE = 4;
static const char*    LOG_CATEGORY_VEC      = "VECTOR";

#define VECTOR_AT(vec, i) \
    (void*)((char*) vec->buffer + i * vec->tsize)

#ifdef _DEBUG

static VectorErr _vector_validate(Vector* vec);

static utils_hash_t _vector_recalc_hashsum(Vector* vec);

#define VECTOR_ASSERT_OK_(vec)                                      \
    {                                                               \
        err = _vector_validate(vec);                                \
        if(err != VECTOR_ERR_NONE) {                                \
            UTILS_LOGE(LOG_CATEGORY_VEC, "%s", vector_strerr(err)); \
            utils_assert(err == VECTOR_ERR_NONE);                   \
        }                                                           \
    }   

#else

#define VECTOR_ASSERT_OK_(vec)

#endif // _DEBUG


static VectorErr vector_realloc_(Vector* vec, size_t capacity);

VectorErr vector_copy_from(Vector* from, Vector* to)
{
    to->tsize = from->tsize;
    to->size = from->size;

    VectorErr err = vector_realloc_(to, from->capacity);
    err verified(return err);

    memcpy(to->buffer, from->buffer, from->tsize * from->size);

    return VECTOR_ERR_NONE;
}

VectorErr vector_ctor(Vector* vec, size_t capacity, size_t tsize)
{
    VectorErr err = VECTOR_ERR_NONE;

    vec->buffer = NULL;
    vec->size   = 0;
    vec->tsize  = tsize;

    err = vector_realloc_(vec, capacity);

    if(err == VECTOR_ERR_ALLOC_FAIL) {
        IF_DEBUG(VECTOR_DUMP(vec, err, "failed to allocate buffer", NULL));
        return err;
    }

    IF_DEBUG(
        _vector_recalc_hashsum(vec);
    )

    return err;
}

void* vector_at(Vector* vec, size_t ind)
{
    VectorErr err = VECTOR_ERR_NONE;
    VECTOR_ASSERT_OK_(vec);

    return VECTOR_AT(vec, ind);
}

VectorErr vector_push(Vector* vec, void* val)
{
    VectorErr err = VECTOR_ERR_NONE;
    VECTOR_ASSERT_OK_(vec);

    if(vec->size == vec->capacity) {
        err = vector_realloc_(vec, vec->capacity * CAPACITY_EXP);
        if(err != VECTOR_ERR_NONE) {
            IF_DEBUG(VECTOR_DUMP(vec, err, "", NULL));
            return err;
        }
    }

    memcpy(vector_at(vec, vec->size++), val, vec->tsize);

    IF_DEBUG(
        _vector_recalc_hashsum(vec);
    );
    
    return err;
}


VectorErr vector_pop(Vector* vec, void** val)
{
    VectorErr err = VECTOR_ERR_NONE;
    VECTOR_ASSERT_OK_(vec);

    *val = vector_at(vec, --vec->size);

    if(vec->capacity / vec->size >= CAPACITY_SHRINK_SCALE) {
        err = vector_realloc_(vec, vec->capacity / CAPACITY_EXP);
        if(err != VECTOR_ERR_NONE) {
            IF_DEBUG(VECTOR_DUMP(vec, err, "", NULL));
            return err;
        }

        IF_DEBUG(
            _vector_recalc_hashsum(vec);
        );
    }

    return err;
}

void vector_free(Vector* vec)
{
    vec->size = 0;
}

static VectorErr vector_realloc_(Vector* vec, size_t capacity)
{
    utils_assert(vec);

    void* buf_tmp = realloc(
        vec->buffer, 
        capacity * vec->tsize);

    if(!buf_tmp) {
        free(vec->buffer);
        return VECTOR_ERR_ALLOC_FAIL;
    }

    memset((char*)buf_tmp + vec->size * vec->tsize, 0, (capacity - vec->size) * vec->tsize);

    vec->buffer = buf_tmp;
    vec->capacity = capacity;

    return VECTOR_ERR_NONE;
}


void vector_dtor(Vector* vec)
{
    utils_assert(vec);

    free(vec->buffer);

    vec->buffer   = NULL;
    vec->capacity = 0;
    vec->size     = 0;
}

const char* vector_strerr(const VectorErr err)
{
    switch(err)
    {
        case VECTOR_ERR_NONE:
            return "none";
            break;
        case VECTOR_ERR_NULL:
            return "vector pointer is null";
            break;
        case VECTOR_ERR_BUFFER_NULL:
            return "vector buffer pointer is null";
            break;
        case VECTOR_ERR_SIZE_EXCEED_CAPACITY:
            return "size > capacity";
            break;
        case VECTOR_ERR_ALLOC_FAIL:
            return "memory allocation failed";
            break;
        case VECTOR_ERR_OUT_OF_BOUND:
            return "boundary exceed";
            break;
        case VECTOR_ERR_HASH_UNMATCH:
            return "buffer hashsum unmatched";
            break;
        default:
            return "unknown";
            break;
    }
}

#ifdef _DEBUG

static VectorErr _vector_validate(Vector* vec)
{
    if(vec == NULL)
        return VECTOR_ERR_NULL;

    else if(vec->buffer == NULL)
        return VECTOR_ERR_BUFFER_NULL;

#ifdef HASH_ENABLED
    else if(
        vec->buffer_hash != 
            utils_djb2_hash(
                vec->buffer, 
                CANARY_SIZE(vec->capacity) * sizeof(vec->buffer[0])
            )
    )
        return VECTOR_ERR_HASH_UNMATCH;
#endif // HASH_ENABLED

    else if(vec->size > vec->capacity)
        return VECTOR_ERR_SIZE_EXCEED_CAPACITY;

    return VECTOR_ERR_NONE;
}

void vector_dump(FILE* stream, Vector* vec, VectorErr err, const char* msg, 
                        const char* filename, const char* funcname, int line, PrintCallback print)
{
    utils_assert(filename);
    utils_assert(funcname);
    utils_assert(stream);

    if(err != VECTOR_ERR_NONE)
        UTILS_LOGE(LOG_CATEGORY_VEC, "vector dump (see below)");
    else
        UTILS_LOGD(LOG_CATEGORY_VEC, "vector dump (see below)");

    fputs("========= VECTOR DUMP ==========\n", stream);

    if(msg)
        fprintf(stream, "what: %s\n", msg);

    fprintf(
        stream, 
        "from: %s:%d %s()\n\n", 
        filename, 
        line, 
        funcname
    );

    BEGIN {
        if(err == VECTOR_ERR_NULL) {
            fprintf(stream, "vector [NULL]\n");
            GOTO_END;
        }

        fprintf(stream, "vector [%p] (%s)\n", vec, vector_strerr(err));

        fputs("{\n", stream);

        const varinfo_t* varinfo = &vec->varinfo;
        fprintf(
            stream, 
            "  initizlized from: %s:%d %s()\n", 
            varinfo->filename, 
            varinfo->line, 
            varinfo->funcname
        );

        fprintf(
            stream, 
            "  capacity: %lu\n", 
            vec->capacity
        );

        fprintf(
            stream, 
            "  size: %lu %s\n", 
            vec->size, 
            err == VECTOR_ERR_SIZE_EXCEED_CAPACITY ? "(BAD)" : ""
        );

#ifdef HASH_ENABLED
        fprintf(
            stream, 
            "  buffer hash: %.16lx\n", 
            vec->buffer_hash
        );
#endif // HASH_ENABLED

        if(err == VECTOR_ERR_BUFFER_NULL) {
            fputs("  buffer [NULL]\n", stream);
            GOTO_END;
        }

        fprintf(stream, "  buffer [%p]\n", vec->buffer);
        fputs("  {\n", stream);

        if(!print) GOTO_END;

        for(size_t i = 0; i < vec->size; ++i) {
            fprintf(stream, "    *[%lu] = ", i);
            print(stream, VECTOR_AT(vec, i));
            err == VECTOR_ERR_HASH_UNMATCH ? fputs(" (BAD)\n", stream) : fputs("\n", stream);
        }

        fputs("  }\n}\n", stream);
    } END;

    fputs("================================\n\n", stream);
}

static utils_hash_t _vector_recalc_hashsum(Vector* vec)
{

#ifdef HASH_ENABLED
    utils_hash_t hash = utils_djb2_hash(
        vec->buffer, 
        CANARY_SIZE(vec->capacity) * sizeof(vec->buffer[0])
    );
    vec->buffer_hash = hash;
    return hash;
#else
    return 0;
#endif // HASH_ENABLED

}

#endif // _DEBUG
