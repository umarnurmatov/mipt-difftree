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

    MAKE_OPERATOR("/", "\\frac{", "}{", "}", OPERATOR_TYPE_DIV),

    MAKE_OPERATOR("^", ""       , "^",  "" , OPERATOR_TYPE_POW),

    MAKE_OPERATOR("sqrt", "\\sqrt\\left(" , "",  "\\right)" , OPERATOR_TYPE_POW),

    MAKE_OPERATOR("log", "\\log\\left(" , ""  , "\\right)", OPERATOR_TYPE_LOG),

    MAKE_OPERATOR("sin", "\\sin\\left(", "", "\\right)", OPERATOR_TYPE_SIN),

    MAKE_OPERATOR("cos", "\\cos\\left(", "", "\\right)", OPERATOR_TYPE_COS),

    MAKE_OPERATOR("tan", "\\tg\\left(", "", "\\right)", OPERATOR_TYPE_TAN),

    MAKE_OPERATOR("ctg", "\\ctg\\left(", "", "\\right)", OPERATOR_TYPE_CTG),

    MAKE_OPERATOR("sh", "\\sh\\left(", "", "\\right)", OPERATOR_TYPE_SH),

    MAKE_OPERATOR("ch", "\\ch\\left(", "", "\\right)", OPERATOR_TYPE_CH),

    MAKE_OPERATOR("th", "\\th\\left(", "", "\\right)", OPERATOR_TYPE_TH),

    MAKE_OPERATOR("arcsin", "\\arcsin\\left(", "", "\\right)", OPERATOR_TYPE_ASIN),

    MAKE_OPERATOR("arccos", "\\arccos\\left(", "", "\\right)", OPERATOR_TYPE_ACOS),

    MAKE_OPERATOR("arctg", "\\arctan\\left(", "", "\\right)", OPERATOR_TYPE_ATAN),

};

// int operators_prepare_op_arr();
