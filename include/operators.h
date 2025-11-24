#pragma once
#include "hashutils.h"
#include "types.h"
#include "utils.h"

typedef struct Operator
{
    OperatorType type;
    unsigned int argnum;
    const char* str;

    const char* latex_str_pref;
    const char* latex_str_inf;
    const char* latex_str_post;

    utils_hash_t hash;

} Operator;

#define MAKE_OPERATOR(str, pref, inf, post, type, argnum) \
    { type, argnum, str, pref, inf, post, utils_djb2_hash(str, SIZEOF(str)) }

static Operator op_arr[] = 
{
    MAKE_OPERATOR("+", "",        "+",  "",  OPERATOR_TYPE_ADD, 2),

    MAKE_OPERATOR("-", "",        "-",  "",  OPERATOR_TYPE_SUB, 2),

    MAKE_OPERATOR("*", "",        "\\cdot",  "",  OPERATOR_TYPE_MUL, 2),

    MAKE_OPERATOR("/", "\\frac{", "}{", "}", OPERATOR_TYPE_DIV, 2),

    MAKE_OPERATOR("^", ""       , "^",  "" , OPERATOR_TYPE_POW, 2),

    MAKE_OPERATOR("exp", "e^{"       , "",  "}" , OPERATOR_TYPE_EXP, 1),

    MAKE_OPERATOR("sqrt", "\\sqrt{" , "",  "}" , OPERATOR_TYPE_POW, 1),

    MAKE_OPERATOR("log", "\\log\\left (" , ""  , "\\right )", OPERATOR_TYPE_LOG, 1),

    MAKE_OPERATOR("sin", "\\sin\\left(", "", "\\right)", OPERATOR_TYPE_SIN, 1),

    MAKE_OPERATOR("cos", "\\cos\\left(", "", "\\right)", OPERATOR_TYPE_COS, 1),

    MAKE_OPERATOR("tan", "\\tg\\left(", "", "\\right)", OPERATOR_TYPE_TAN, 1),

    MAKE_OPERATOR("ctg", "\\ctg\\left(", "", "\\right)", OPERATOR_TYPE_CTG, 1),

    MAKE_OPERATOR("sh", "\\sh\\left(", "", "\\right)", OPERATOR_TYPE_SH, 1),

    MAKE_OPERATOR("ch", "\\ch\\left(", "", "\\right)", OPERATOR_TYPE_CH, 1),

    MAKE_OPERATOR("th", "\\th\\left(", "", "\\right)", OPERATOR_TYPE_TH, 1),

    MAKE_OPERATOR("arcsin", "\\arcsin\\left(", "", "\\right)", OPERATOR_TYPE_ASIN, 1),

    MAKE_OPERATOR("arccos", "\\arccos\\left(", "", "\\right)", OPERATOR_TYPE_ACOS, 1),

    MAKE_OPERATOR("arctg", "\\arctan\\left(", "", "\\right)", OPERATOR_TYPE_ATAN, 1),

};

// int operators_prepare_op_arr();

const Operator* get_operator(OperatorType op_type);
