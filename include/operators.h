#pragma once
#include "hashutils.h"
#include "types.h"
#include "utils.h"

const size_t MAX_OP_NAME_LEN = 10;

typedef enum OperatorArgnum
{
    OPERATOR_ARGNUM_NONE = 0x00,
    OPERATOR_ARGNUM_1    = 0x01,
    OPERATOR_ARGNUM_2    = 0x02,

} OperatorArgnum;

typedef enum OperatorPrecedance
{
    OPERATOR_PRECEDANCE_0 = 0x00,
    OPERATOR_PRECEDANCE_1 = 0x01,
    OPERATOR_PRECEDANCE_2 = 0x02,
    OPERATOR_PRECEDANCE_3 = 0x03,
    OPERATOR_PRECEDANCE_4 = 0x04,

} OperatorPrecedance;

typedef struct Operator
{
    OperatorType type;
    OperatorArgnum argnum;
    OperatorPrecedance precedance;
    const char* str;

    const char* latex_str_pref;
    const char* latex_str_inf;
    const char* latex_str_post;

    utils_hash_t hash;

} Operator;

#define MAKE_OPERATOR(str, pref, inf, post, type, argnum, precedance) \
    { type, argnum, precedance, str, pref, inf, post, utils_djb2_hash(str, SIZEOF(str)) }

static Operator op_arr[] = 
{
    MAKE_OPERATOR("+"     , ""          , "+"         , ""   , OPERATOR_TYPE_ADD  , OPERATOR_ARGNUM_2 , OPERATOR_PRECEDANCE_1),
    MAKE_OPERATOR("-"     , ""          , "-"         , ""   , OPERATOR_TYPE_SUB  , OPERATOR_ARGNUM_2 , OPERATOR_PRECEDANCE_1),
    MAKE_OPERATOR("*"     , "{"         , "}\\cdot{"  , "}"  , OPERATOR_TYPE_MUL  , OPERATOR_ARGNUM_2 , OPERATOR_PRECEDANCE_2),
    MAKE_OPERATOR("/"     , "\\frac{"   , "}{"        , "}"  , OPERATOR_TYPE_DIV  , OPERATOR_ARGNUM_2 , OPERATOR_PRECEDANCE_2),
    MAKE_OPERATOR("^"     , "{"         , "}^{"       , "}"  , OPERATOR_TYPE_POW  , OPERATOR_ARGNUM_2 , OPERATOR_PRECEDANCE_3),
    MAKE_OPERATOR("exp"   , "e^{"       , ""          , "}"  , OPERATOR_TYPE_EXP  , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("sqrt"  , "\\sqrt{"   , ""          , "}"  , OPERATOR_TYPE_SQRT , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("ln"    , "\\ln{"     , ""          , "}"  , OPERATOR_TYPE_LOG  , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("sin"   , "\\sin{"    , ""          , "}"  , OPERATOR_TYPE_SIN  , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("cos"   , "\\cos{"    , ""          , "}"  , OPERATOR_TYPE_COS  , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("tan"   , "\\tg{"     , ""          , "}"  , OPERATOR_TYPE_TAN  , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("ctg"   , "\\ctg{"    , ""          , "}"  , OPERATOR_TYPE_CTG  , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("sh"    , "\\sh{"     , ""          , "}"  , OPERATOR_TYPE_SH   , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("ch"    , "\\ch{"     , ""          , "}"  , OPERATOR_TYPE_CH   , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("th"    , "\\th{"     , ""          , "}"  , OPERATOR_TYPE_TH   , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("arcsin", "\\arcsin{" , ""          , "}"  , OPERATOR_TYPE_ASIN , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("arccos", "\\arccos{" , ""          , "}"  , OPERATOR_TYPE_ACOS , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
    MAKE_OPERATOR("arctg" , "\\arctan{" , ""          , "}"  , OPERATOR_TYPE_ATAN , OPERATOR_ARGNUM_1 , OPERATOR_PRECEDANCE_4),
};

// int operators_prepare_op_arr();

const Operator* get_operator(OperatorType op_type);

const Operator* match_function(char* str);
