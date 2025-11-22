#pragma once
#include "hashutils.h"
#include "types.h"
#include "utils.h"

typedef struct Operator
{
    OperatorType type;
    const char* str;

    const char* latex_str_pref;
    const char* latex_str_inf;
    const char* latex_str_post;

    utils_hash_t hash;

} Operator;

#define MAKE_OPERATOR(str, pref, inf, post, type) \
    { type, str, pref, inf, post, utils_djb2_hash(str, SIZEOF(str)) }

static Operator op_arr[] = 
{
    MAKE_OPERATOR("+", "",        "+",  "",  OPERATOR_TYPE_ADD),
    MAKE_OPERATOR("-", "",        "-",  "",  OPERATOR_TYPE_SUB),
    MAKE_OPERATOR("*", "",        "*",  "",  OPERATOR_TYPE_MUL),
    MAKE_OPERATOR("/", "\\frac{", "}{", "}", OPERATOR_TYPE_DIV)
};

// int operators_prepare_op_arr();
