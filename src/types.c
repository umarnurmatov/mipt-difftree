#include "types.h"

const char* node_type_str(NodeType node_type)
{
    switch(node_type) {
        case NODE_TYPE_OP:
            return "OP";
        case NODE_TYPE_VAR:
            return "VAR";
        case NODE_TYPE_NUM:
            return "NUM";
        default:
            return "???";
    }
}

const char* node_op_type_str(OperatorType op_type)
{
    switch(op_type) {
        case OPERATOR_TYPE_ADD:
            return "ADD";
        case OPERATOR_TYPE_SUB:
            return "SUB";
        case OPERATOR_TYPE_MUL:
            return "MUL";
        case OPERATOR_TYPE_DIV:
            return "DIV";
        case OPERATOR_TYPE_NONE:
        default:
            return "???";
    }
}
