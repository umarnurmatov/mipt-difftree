#include "difftree_math.h"
#include "difftree.h"
#include "logutils.h"
#include "types.h"
#include "operators.h"

#define LOG_CTG_DMATH "DIFFTREE_MATH"

static DiffTreeNode* diff_tree_differentiate_op_(DiffTree* dtree, DiffTreeNode* node, Variable* var);
static DiffTreeNode* diff_tree_differentiate_var_(DiffTree* dtree, DiffTreeNode* node, Variable* var);
static DiffTreeNode* diff_tree_differentiate_num_(DiffTree* dtree, DiffTreeNode* node, Variable* var);

/* And here goes our DSL */
#define dL diff_tree_differentiate(dtree, node->left, var)
#define dR diff_tree_differentiate(dtree, node->right, var)
#define cL diff_tree_copy_subtree(dtree, node->left)
#define cR diff_tree_copy_subtree(dtree, node->right)
#define ADD_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_ADD }, left, right)
#define SUB_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_SUB }, left, right)
#define MUL_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_MUL }, left, right)
#define DIV_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_DIV }, left, right)

DiffTreeNode* diff_tree_differentiate(DiffTree* dtree, DiffTreeNode* node, Variable* var)
{
    diff_tree_dump_latex(dtree);

    switch(node->type) {
        case NODE_TYPE_OP:
            return diff_tree_differentiate_op_(dtree, node, var);

        case NODE_TYPE_VAR:
            return diff_tree_differentiate_var_(dtree, node, var);

        case NODE_TYPE_NUM:
            return diff_tree_differentiate_num_(dtree, node, var);

        default:
            UTILS_LOGE(LOG_CTG_DMATH, "unknown node type %d", node->type);
            return NULL;
    }

}

static DiffTreeNode* diff_tree_differentiate_op_(DiffTree* dtree, DiffTreeNode* node, Variable* var)
{
    utils_assert(node);
    utils_assert(node->type == NODE_TYPE_OP);

    switch(node->value.op_type) {
        case OPERATOR_TYPE_ADD:
            return ADD_(dL, dR);
        case OPERATOR_TYPE_SUB:
            return SUB_(dL, dR);
        case OPERATOR_TYPE_DIV:
        case OPERATOR_TYPE_MUL:
            return ADD_(MUL_(dL, cR), MUL_(cL, dR));
        default:
            UTILS_LOGE(LOG_CTG_DMATH, "unknown operator %d", node->value.op_type);
            return NULL;
    }
}

static DiffTreeNode* diff_tree_differentiate_var_(ATTR_UNUSED DiffTree* dtree, DiffTreeNode* node, Variable* var)
{
    utils_assert(node);
    utils_assert(node->type == NODE_TYPE_VAR);

    if(node->value.var_hash == var->hash)
        return diff_tree_new_node(NODE_TYPE_NUM, NodeValue { 1 }, NULL, NULL);
    else 
        return diff_tree_new_node(NODE_TYPE_NUM, NodeValue { 0 }, NULL, NULL);
}

static DiffTreeNode* diff_tree_differentiate_num_(ATTR_UNUSED DiffTree* dtree, ATTR_UNUSED DiffTreeNode* node, ATTR_UNUSED Variable* var)
{
    return diff_tree_new_node(NODE_TYPE_NUM, NodeValue { 0 }, NULL, NULL);
}

#undef dR
#undef dL
#undef ADD_
#undef SUB_
#undef MUL_
#undef DIV_
