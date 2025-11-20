#include "difftree_math.h"
#include "difftree.h"
#include "logutils.h"
#include "types.h"
#include "operators.h"

static DiffTreeNode* diff_tree_differentiate_op_(DiffTreeNode* node);

/* And here goes our DSL */
#define dL diff_tree_differentiate(node->left)
#define dR diff_tree_differentiate(node->right)
#define ADD_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_ADD }, left, right)
#define SUB_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_SUB }, left, right)
#define MUL_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_MUL }, left, right)
#define DIV_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_DIV }, left, right)

DiffTreeNode* diff_tree_differentiate(DiffTreeNode* node)
{
    switch(node->type) {
        case NODE_TYPE_OP:
            return diff_tree_differentiate_op_(node);

        case NODE_TYPE_VAR:
            break;

        case NODE_TYPE_NUM:
            break;
    }
}

static DiffTreeNode* diff_tree_differentiate_op_(DiffTreeNode* node)
{
    switch(node->value.op_type) {
        case OPERATOR_TYPE_ADD:
            return ADD_(dL, dR);
        case OPERATOR_TYPE_SUB:
        case OPERATOR_TYPE_DIV:
        case OPERATOR_TYPE_MUL:
            break;
    }
}

#undef dR
#undef dL
#undef ADD_
#undef SUB_
#undef MUL_
#undef DIV_
