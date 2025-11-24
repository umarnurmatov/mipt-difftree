#include "types.h"

const char* node_type_str(NodeType node_type)
{
    switch(node_type) {
        case NODE_TYPE_OP:  return "OP";
        case NODE_TYPE_VAR: return "VAR";
        case NODE_TYPE_NUM: return "NUM";
        case NODE_TYPE_FAKE:return "FAKE";
        default:            return "???";
    }
}

const char* node_op_type_str(OperatorType op_type)
{
    switch(op_type) {
        case OPERATOR_TYPE_ADD:   return "ADD";
        case OPERATOR_TYPE_SUB:   return "SUB";
        case OPERATOR_TYPE_MUL:   return "MUL";
        case OPERATOR_TYPE_DIV:   return "DIV";
        case OPERATOR_TYPE_POW:   return "POW";
        case OPERATOR_TYPE_EXP:   return "EXP";
        case OPERATOR_TYPE_SQRT:  return "SQRT";
        case OPERATOR_TYPE_LOG:   return "LOG";
        case OPERATOR_TYPE_SIN:   return "SIN";
        case OPERATOR_TYPE_COS:   return "COS";
        case OPERATOR_TYPE_TAN:   return "TAN";
        case OPERATOR_TYPE_CTG:   return "CTG";
        case OPERATOR_TYPE_SH:    return "SH";
        case OPERATOR_TYPE_CH:    return "CH";
        case OPERATOR_TYPE_TH:    return "TH";
        case OPERATOR_TYPE_ASIN:  return "ARCSIN";
        case OPERATOR_TYPE_ACOS:  return "ARCCOS";
        case OPERATOR_TYPE_ATAN:  return "ARCTG";
        case OPERATOR_TYPE_ACTG:  return "ARCCTG";
        case OPERATOR_TYPE_NONE:  return "NONE";
        default:                  return "???";
    }
}
