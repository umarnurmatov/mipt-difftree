#pragma once
#include "hashutils.h"

typedef enum NodeType
{
    NODE_TYPE_OP,
    NODE_TYPE_VAR,
    NODE_TYPE_NUM
}
NodeType;

typedef enum OperatorType
{
    OPERATOR_TYPE_ADD,
    OPERATOR_TYPE_SUB,
    OPERATOR_TYPE_MUL,
    OPERATOR_TYPE_DIV,
    OPERATOR_TYPE_POW,
    OPERATOR_TYPE_SQRT,
    OPERATOR_TYPE_LOG,
    OPERATOR_TYPE_SIN,
    OPERATOR_TYPE_COS,
    OPERATOR_TYPE_TAN,
    OPERATOR_TYPE_CTG,
    OPERATOR_TYPE_SH,
    OPERATOR_TYPE_CH,
    OPERATOR_TYPE_TH,
    OPERATOR_TYPE_ASIN,
    OPERATOR_TYPE_ACOS,
    OPERATOR_TYPE_ATAN,
    OPERATOR_TYPE_ACTG,
    OPERATOR_TYPE_NONE
} 
OperatorType;

typedef union NodeValue
{
    utils_hash_t var_hash;
    OperatorType op_type;
    double num;
}
NodeValue;

const char* node_type_str(NodeType node_type);

const char* node_op_type_str(OperatorType op_type);


