#include "difftree_math.h"
#include "difftree.h"
#include "logutils.h"
#include "types.h"
#include "operators.h"

#define LOG_CTG_DMATH "DIFFTREE_MATH"

static DiffTreeNode* diff_tree_differentiate_op_(DiffTree* dtree, DiffTreeNode* node, Variable* var);
static DiffTreeNode* diff_tree_differentiate_var_(DiffTree* dtree, DiffTreeNode* node, Variable* var);
static DiffTreeNode* diff_tree_differentiate_num_(DiffTree* dtree, DiffTreeNode* node, Variable* var);

static DiffTreeNode* diff_tree_evaluate_op_(DiffTree* dtree, DiffTreeNode* node);

/* And here goes our DSL */
#define cL diff_tree_copy_subtree(dtree, node->left, NULL)
#define cR diff_tree_copy_subtree(dtree, node->right, NULL)
#define dL diff_tree_differentiate(dtree, cL, var)
#define dR diff_tree_differentiate(dtree, cR, var)

#define ADD_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_ADD }, left, right)

#define SUB_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_SUB }, left, right)

#define MUL_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_MUL }, left, right)

#define DIV_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_DIV }, left, right)

#define CONST_(num_) \
    diff_tree_new_node(NODE_TYPE_NUM, NodeValue { .num = num_ }, NULL, NULL)

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
            return DIV_(ADD_(MUL_(dL, cR), MUL_(dL, cR)), MUL_(cR, cR));
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
        return CONST_(1);
    else 
        return CONST_(0);
}

static DiffTreeNode* diff_tree_differentiate_num_(ATTR_UNUSED DiffTree* dtree, ATTR_UNUSED DiffTreeNode* node, ATTR_UNUSED Variable* var)
{
    return CONST_(0);
}

#undef dR
#undef dL
#undef cR
#undef cL
#undef ADD_
#undef SUB_
#undef MUL_
#undef DIV_

DiffTreeNode* diff_tree_evaluate(DiffTree* dtree, DiffTreeNode* node)
{
    switch(node->type) {
        case NODE_TYPE_OP:
            return diff_tree_evaluate_op_(dtree, node);

        case NODE_TYPE_VAR:
            return CONST_(diff_tree_find_variable(dtree, node->value.var_hash)->val);

        case NODE_TYPE_NUM:
            return node;

        default:
            UTILS_LOGE(LOG_CTG_DMATH, "unknown node type %d", node->type);
            return NULL;
    }
}

DiffTreeNode* diff_tree_evaluate_op_(DiffTree* dtree, DiffTreeNode* node)
{
    utils_assert(node);
    utils_assert(node->type == NODE_TYPE_OP);

    DiffTreeNode *left = NULL, *right = NULL;

    if(node->left)
       left = diff_tree_evaluate(dtree, node->left);

    if(node->right)
       right = diff_tree_evaluate(dtree, node->right);


    switch(node->value.op_type) {
        case OPERATOR_TYPE_ADD:
            return CONST_(left->value.num + right->value.num);
        case OPERATOR_TYPE_SUB:
            return CONST_(left->value.num - right->value.num);
        case OPERATOR_TYPE_DIV:
            return CONST_(left->value.num / right->value.num);
        case OPERATOR_TYPE_MUL:
            return CONST_(left->value.num * right->value.num);
        default:
            UTILS_LOGE(LOG_CTG_DMATH, "unknown operator %d", node->value.op_type);
            return NULL;
    }
}

