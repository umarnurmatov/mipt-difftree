#pragma once
#include "hashutils.h"
#include "types.h"
#include "utils.h"

typedef struct Operator
{
    OperatorType type;
    const char* str;
    utils_hash_t hash;
} 
Operator;

#define MAKE_OPERATOR(str, type) \
    { type, str, utils_djb2_hash(str, SIZEOF(str)) }

static Operator op_arr[] = 
{
    MAKE_OPERATOR("+", OPERATOR_TYPE_ADD),
    MAKE_OPERATOR("-", OPERATOR_TYPE_SUB),
    MAKE_OPERATOR("*", OPERATOR_TYPE_MUL),
    MAKE_OPERATOR("/", OPERATOR_TYPE_DIV)
};

// int operators_prepare_op_arr();
