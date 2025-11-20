#include "vector.h"

#include <assert.h>
#include <memory.h>

#include "sortutils.h"

#ifdef VECTOR_DEBUG

#define VECTOR_DUMP(STK, ERR, MSG) \
    _vector_dump(stderr, STK, ERR, MSG, __FILE__, __func__, __LINE__)

static void _vector_dump(FILE* stream, Vector* vec, vector_err_t err, const char* msg, 
                        const char* filename, const char* funcname, int line);

static vector_err_t _vector_validate(Vector* vec);

static utils_hash_t _vector_recalc_hashsum(Vector* vec);

const vector_data_t POISON       = (vector_data_t)0xCAFEBABE;

#else

#define VECTOR_ASSERT_OK_(vec)

#endif // VECTOR_DEBUG


static VectorErr vector_realloc_(Vector* vec, size_t capacity);

VectorErr vector_ctor(Vector* vec, size_t capacity, size_t tsize)
{
    VectorErr err = VECTOR_ERR_NONE;

    vec->buffer = NULL;
    vec->size   = 0;
    vec->tsize  = tsize;

    err = vector_realloc_(vec, capacity);
    if(err == VECTOR_ERR_ALLOC_FAIL) {
        IF_DEBUG(VECTOR_DUMP(vec, err, "failed to allocate buffer"));
        return err;
    }

    IF_DEBUG(
        vec->canary_begin = CANARY_BEGIN;
        vec->canary_end   = CANARY_END;

        _vector_recalc_hashsum(vec);
    )

    return err;
}

void* vector_at(Vector* vec, size_t ind)
{
    VECTOR_ASSERT_OK_(vec);

    return (char*)vec->buffer + vec->tsize * ind;
}

VectorErr vector_push(Vector* vec, void* val)
{
    VectorErr err = VECTOR_ERR_NONE;
    
    VECTOR_ASSERT_OK_(vec);

    if(vec->size == vec->capacity) {
        err = vector_realloc_(vec, vec->capacity * CAPACITY_EXP);
        if(err != VECTOR_ERR_NONE) {
            IF_DEBUG(VECTOR_DUMP(vec, err, ""));
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
            IF_DEBUG(VECTOR_DUMP(vec, err, ""));
            return err;
        }

        IF_DEBUG(
            _vector_recalc_hashsum(vec);
        );
    }

    return err;
}


static VectorErr vector_realloc_(Vector* vec, size_t capacity)
{
    void* buffer_tmp = realloc(
        vec->buffer, 
        capacity * vec->tsize 
    );

    if(buffer_tmp == NULL)
        return VECTOR_ERR_ALLOC_FAIL;

    IF_DEBUG(
        for(size_t i = vec->size; i < capacity; ++i)
            buffer_tmp[i] = POISON;
    );

    vec->buffer   = buffer_tmp;
    vec->capacity = capacity;

    return VECTOR_ERR_NONE;
}


void vector_dtor(Vector* vec)
{
    IF_DEBUG(
        vector_err_t err;
        err = _vector_validate(vec);

        if(err == VECTOR_ERR_NULL)
            VECTOR_DUMP(vec, err, "tried to deinit uninitialized vector");
    );

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

#ifdef VECTOR_DEBUG

static vector_err_t _vector_validate(vec_t* vec)
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

static void _vector_dump(FILE* stream, vec_t* vec, vector_err_t err, const char* msg, 
                        const char* filename, const char* funcname, int line)
{
    fputs("================================\n", stream);
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
            "  init: %s:%d %s(): %s\n", 
            varinfo->filename, 
            varinfo->line, 
            varinfo->funcname, 
            varinfo->varname
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

        fprintf(
            stream, 
            "    [#] = %x \t [CANARY] %s\n", 
            (unsigned)vec->buffer[0],
            err == VECTOR_ERR_CANARY_ESCAPED ? "(BAD)" : ""
        );

        for(size_t i = 0; i < vec->capacity; ++i)
            if(vec->buffer[i] == POISON)
                fprintf(
                    stream, 
                    "    [%lu] = %d \t [POISON]\n", 
                    i, 
                    vec->buffer[CANARY_INDEX(i)]
                );
            else
                fprintf(
                    stream, 
                    "   *[%lu] = %d %s\n", 
                    i, 
                    vec->buffer[CANARY_INDEX(i)],
                    err == VECTOR_ERR_HASH_UNMATCH ? "\t (BAD)" : ""
                );

        fprintf(
            stream, 
            "    [#] = %x \t [CANARY] %s\n", 
            (unsigned)vec->buffer[CANARY_SIZE(vec->capacity) - 1],
            err == VECTOR_ERR_CANARY_ESCAPED ? "(BAD)" : ""
        );

        fputs("}\n", stream);
    } END;

    fputs("================================\n\n", stream);
}

static utils_hash_t _vector_recalc_hashsum(vec_t* vec)
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
