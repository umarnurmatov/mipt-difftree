#include "difftree_optimize.h"

#include "difftree_math.h"
#include "assertutils.h"
#include "difftree.h"
#include "floatutils.h"
#include "types.h"
#include "utils.h"

ATTR_UNUSED static const char* LOG_CTG_DIFF_OPT = "DIFFTREE OPTIMIZE";

static bool treeChanged = false;

static DiffTreeNode* diff_tree_const_fold_(DiffTree* dtree, DiffTreeNode* node);

static DiffTreeNode* diff_tree_eliminate_neutral_(DiffTree* dtree, DiffTreeNode* node);

static DiffTreeNode* diff_tree_eliminate_neutral_mul_(DiffTree* dtree, DiffTreeNode* node, DiffTreeNode* left, DiffTreeNode* right);

static DiffTreeNode* diff_tree_eliminate_neutral_add_(DiffTree* dtree, DiffTreeNode* node, DiffTreeNode* left, DiffTreeNode* right);

static DiffTreeNode* diff_tree_eliminate_neutral_pow_(DiffTree* dtree, DiffTreeNode* node, DiffTreeNode* left, DiffTreeNode* right);

void diff_tree_optimize(DiffTree *dtree)
{
    do {

        treeChanged = false;
        diff_tree_const_fold_(dtree, dtree->root->left);
        diff_tree_eliminate_neutral_(dtree, dtree->root->left);

    } while(treeChanged);
}

#define CONST_(num_) \
    diff_tree_new_node(NODE_TYPE_NUM, NodeValue { .num = num_ }, NULL, NULL, node->parent)

static DiffTreeNode* diff_tree_const_fold_(DiffTree* dtree, DiffTreeNode* node)
{
    DiffTreeNode *left = NULL, *right = NULL;

    if(node->left)
        left = diff_tree_const_fold_(dtree, node->left);

    if(node->right)
        right = diff_tree_const_fold_(dtree, node->right);

    bool left_has_var = true, right_has_val = true; 
    if(left)  left_has_var  = diff_tree_subtree_holds_var(left);
    if(right) right_has_val = diff_tree_subtree_holds_var(right);

    if(!left_has_var && !right_has_val) {
        DiffTreeNode* new_node 
            = CONST_(diff_tree_evaluate_op(dtree, node));
        
        if(node->parent->left == node)
            node->parent->left = new_node;

        if(node->parent->right == node)
            node->parent->right = new_node;

        diff_tree_mark_to_delete(dtree, node);

        treeChanged = true;

        return new_node;
    }

    return node;
}

#define IS_VALUE_(node, val) \
    ((node->type == NODE_TYPE_NUM) && (utils_equal_with_precision(node->value.num, val)))

#define cL diff_tree_copy_subtree(dtree, node->left, node)
#define cR diff_tree_copy_subtree(dtree, node->right, node)

static DiffTreeNode* diff_tree_eliminate_neutral_(DiffTree* dtree, DiffTreeNode* node)
{
    utils_assert(dtree);
    utils_assert(node);

    DiffTreeNode *left = NULL, *right = NULL, *new_node = node;
    
    if(node->type != NODE_TYPE_OP)
        return node;

    if(node->left)
        left = diff_tree_eliminate_neutral_(dtree, node->left);

    if(node->right)
        right = diff_tree_eliminate_neutral_(dtree, node->right);

    switch(node->value.op_type)
    {
        case OPERATOR_TYPE_MUL:
            new_node = 
                diff_tree_eliminate_neutral_mul_(dtree, node, left, right);
            break;
        case OPERATOR_TYPE_ADD:
            new_node = 
                diff_tree_eliminate_neutral_add_(dtree, node, left, right);
            break;
        case OPERATOR_TYPE_POW:
            new_node = 
                diff_tree_eliminate_neutral_pow_(dtree, node, left, right);
            break;
        default:
            break;
    }

    if(new_node != node) {
        if(node->parent->left == node)
            node->parent->left = new_node;

        else if(node->parent->right == node)
            node->parent->right = new_node;

        treeChanged = true;

        diff_tree_mark_to_delete(dtree, node);
    }

    return new_node;
}

static DiffTreeNode* diff_tree_eliminate_neutral_mul_(DiffTree* dtree, DiffTreeNode* node, DiffTreeNode* left, DiffTreeNode* right)
{
    utils_assert(dtree);
    utils_assert(node);
    utils_assert(left);
    utils_assert(right);

    utils_assert(node->type == NODE_TYPE_OP);
    utils_assert(node->value.op_type == OPERATOR_TYPE_MUL);

    DiffTreeNode* new_node = node;

    if     (IS_VALUE_(left, 0.f))  new_node = CONST_(0.f);
    else if(IS_VALUE_(left, 1.f))  new_node = cR;
    else if(IS_VALUE_(right, 0.f)) new_node = CONST_(0.f);
    else if(IS_VALUE_(right, 1.f)) new_node = cL;

    return new_node;
}


static DiffTreeNode* diff_tree_eliminate_neutral_add_(DiffTree* dtree, DiffTreeNode* node, DiffTreeNode* left, DiffTreeNode* right)
{
    utils_assert(dtree);
    utils_assert(node);
    utils_assert(left);
    utils_assert(right);

    utils_assert(node->type == NODE_TYPE_OP);
    utils_assert(node->value.op_type == OPERATOR_TYPE_ADD);

    DiffTreeNode* new_node = node;

    if     (IS_VALUE_(left, 0.f))  new_node = cR;
    else if(IS_VALUE_(right, 0.f)) new_node = cL;
    
    return new_node;
}

static DiffTreeNode* diff_tree_eliminate_neutral_pow_(DiffTree* dtree, DiffTreeNode* node, DiffTreeNode* left, DiffTreeNode* right)
{
    utils_assert(dtree);
    utils_assert(node);
    utils_assert(left);
    utils_assert(right);

    utils_assert(node->type == NODE_TYPE_OP);
    utils_assert(node->value.op_type == OPERATOR_TYPE_POW);

    DiffTreeNode* new_node = node;

    if      (IS_VALUE_(left,  0.f)) new_node = CONST_(0.f); // 0 ^ x = 0
    else if (IS_VALUE_(left,  1.f)) new_node = CONST_(1.f); // 1 ^ x = 1
    else if (IS_VALUE_(right, 0.f)) new_node = CONST_(1.f); // x ^ 0 = 1
    else if (IS_VALUE_(right, 1.f)) new_node = cL;          // x ^ 1 = x

    return new_node;
}

#undef CONST_
#undef IS_VALUE_
#undef cL
#undef cR
