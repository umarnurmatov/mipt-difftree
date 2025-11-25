#include "difftree_math.h"

#include <cfenv>
#include <math.h>
#include <fenv.h>

#include "difftree.h"
#include "logutils.h"
#include "types.h"
#include "operators.h"

#pragma STDC FENV_ACCESS ON

#define LOG_CTG_DMATH "DIFFTREE_MATH"

static bool IS_FE_EXCEPTION_SET = false;

static DiffTreeNode* diff_tree_differentiate_op_(DiffTree* dtree, DiffTreeNode* node, Variable* var);
static DiffTreeNode* diff_tree_differentiate_var_(DiffTree* dtree, DiffTreeNode* node, Variable* var);
static DiffTreeNode* diff_tree_differentiate_num_(DiffTree* dtree, DiffTreeNode* node, Variable* var);

static const char* diff_tree_get_fe_exception_str(void);

static void diff_tree_check_math_errors(DiffTreeNode* node, double left, double right);

DiffTreeErr diff_tree_differentiate_tree(DiffTree* dtree, Variable* var)
{
    dtree->root->left = diff_tree_differentiate(dtree, dtree->root->left, var);

    return DIFF_TREE_ERR_NONE;
}

/* And here goes our DSL */
#define cL diff_tree_copy_subtree(dtree, node->left, node)
#define cR diff_tree_copy_subtree(dtree, node->right, node)
#define dL diff_tree_differentiate(dtree, cL, var)
#define dR diff_tree_differentiate(dtree, cR, var)

#define ADD_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_ADD }, left, right, node->parent)

#define SUB_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_SUB }, left, right, node->parent)

#define MUL_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_MUL }, left, right, NULL)

#define DIV_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_DIV }, left, right, NULL)

#define SIN_(left) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_SIN }, left, NULL, NULL)

#define COS_(left) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_COS }, left, NULL, NULL)

#define SH_(left) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_SH }, left, NULL, NULL)

#define CH_(left) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_CH }, left, NULL, NULL)

#define LOG_(left) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_LOG }, left, NULL, NULL)

#define EXP_(left) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_EXP }, left, NULL, NULL)

#define POW_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_POW }, left, right, NULL)

#define SQRT_(left) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_SQRT }, left, NULL, NULL)

#define CONST_(num_) \
    diff_tree_new_node(NODE_TYPE_NUM, NodeValue { .num = num_ }, NULL, NULL, NULL)

DiffTreeNode* diff_tree_differentiate(DiffTree* dtree, DiffTreeNode* node, Variable* var)
{
    utils_assert(dtree);
    utils_assert(node);
    utils_assert(var);

    DiffTreeNode* new_node = NULL;

    switch(node->type) {
        case NODE_TYPE_OP:
            new_node = diff_tree_differentiate_op_(dtree, node, var);
            break;

        case NODE_TYPE_VAR:
            new_node = diff_tree_differentiate_var_(dtree, node, var);
            break;

        case NODE_TYPE_NUM:
            new_node = diff_tree_differentiate_num_(dtree, node, var);
            break;

        default:
            UTILS_LOGE(LOG_CTG_DMATH, "unknown node type %d", node->type);
            return NULL;
    }

    if(node->parent) {
        if(node == node->parent->left)
            node->parent->left = new_node;
        else if(node == node->parent->right)
            node->parent->right = new_node;
    }

    diff_tree_dump_latex(dtree, new_node);
    DIFF_TREE_DUMP_NODE(dtree, new_node, DIFF_TREE_ERR_NONE);

    diff_tree_mark_to_delete(dtree, node);

    return new_node;
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
            return DIV_(ADD_(MUL_(dL, cR), MUL_(cL, dR)), POW_(cR, CONST_(2)));
        case OPERATOR_TYPE_MUL:
            return ADD_(MUL_(dL, cR), MUL_(cL, dR));
        case OPERATOR_TYPE_POW:
        {
            bool left = diff_tree_subtree_holds_var(node->left);
            bool right = diff_tree_subtree_holds_var(node->right);

            if(left && right) {
                DiffTreeNode* exp_f_1 = MUL_(cR, LOG_(cL));
                DiffTreeNode* exp_f_2 = MUL_(cR, LOG_(cL));
                return MUL_(EXP_(exp_f_1), diff_tree_differentiate(dtree, exp_f_2, var));
            }
            else if(left)
                return MUL_(POW_(cL, SUB_(cR, CONST_(1))), dL);
            else if(right)
                return MUL_(MUL_(POW_(cL, cR), LOG_(cL)), dR);
            else {
                UTILS_LOGE(LOG_CTG_DMATH, "constant folding was not run");
                return NULL;
            }
        }
        case OPERATOR_TYPE_SQRT:
            return DIV_(dL, MUL_(CONST_(2), SQRT_(cL)));
        case OPERATOR_TYPE_LOG:
            return DIV_(dL, cL);
        case OPERATOR_TYPE_SIN:
            return MUL_(COS_(cL), dL);
        case OPERATOR_TYPE_COS:
            return MUL_(CONST_(-1), MUL_(COS_(cL), dL));
        case OPERATOR_TYPE_TAN:
            return DIV_(dL, POW_(COS_(cL), CONST_(2))); 
        case OPERATOR_TYPE_CTG:
            return MUL_(CONST_(-1), DIV_(dL, POW_(SIN_(cL), CONST_(2))));
        case OPERATOR_TYPE_SH:
            return MUL_(CH_(cL), dL);
        case OPERATOR_TYPE_CH:
            return MUL_(SH_(cL), dL);
        case OPERATOR_TYPE_TH:
            return DIV_(dL, POW_(CH_(cL), CONST_(2))); 
        case OPERATOR_TYPE_ASIN:
            return DIV_(dL, SQRT_(SUB_(CONST_(1), POW_(cL, CONST_(2)))));
        case OPERATOR_TYPE_ACOS:
            return MUL_(CONST_(-1), DIV_(dL, SQRT_(SUB_(CONST_(1), POW_(cL, CONST_(2))))));
        case OPERATOR_TYPE_ATAN:
            return DIV_(dL, ADD_(CONST_(1), POW_(cL, CONST_(2))));
        case OPERATOR_TYPE_ACTG:
            return MUL_(CONST_(-1), DIV_(dL, ADD_(CONST_(1), POW_(cL, CONST_(2)))));
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
    utils_assert(node);
    utils_assert(node->type == NODE_TYPE_NUM);

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

double diff_tree_evaluate_tree(DiffTree* dtree)
{
    feclearexcept(FE_ALL_EXCEPT);
    return diff_tree_evaluate(dtree, dtree->root->left);
}

double diff_tree_evaluate(DiffTree* dtree, DiffTreeNode* node)
{
    utils_assert(dtree);
    utils_assert(node);

    double res = NAN;

    switch(node->type) {
        case NODE_TYPE_OP:
            res = diff_tree_evaluate_op(dtree, node);
            break;

        case NODE_TYPE_VAR:
            res = diff_tree_find_variable(dtree, node->value.var_hash)->val;
            break;

        case NODE_TYPE_NUM:
            res = node->value.num;
            break;

        default:
            UTILS_LOGE(LOG_CTG_DMATH, "unknown node type %d", node->type);
            break;
    }

    return res;
}

#define left_ left
#define right_ right

#define CHECK_MATH_ERR                              \
    diff_tree_check_math_errors(node, left, right);

#define CHECK_MATH_ERR_AND_RET                      \
    diff_tree_check_math_errors(node, left, right); \
    return res;

double diff_tree_evaluate_op(DiffTree* dtree, DiffTreeNode* node)
{
    utils_assert(node);
    utils_assert(node->type == NODE_TYPE_OP);

    double left = NAN, right = NAN, res = NAN;

    if(node->left)
       left = diff_tree_evaluate(dtree, node->left);

    if(IS_FE_EXCEPTION_SET) return res;

    if(node->right)
       right = diff_tree_evaluate(dtree, node->right);

    if(IS_FE_EXCEPTION_SET) return res;

    // FIXME
    // if(get_operator(node->value.op_type)->argnum == 2) {
    // }

    switch(node->value.op_type) {
        case OPERATOR_TYPE_ADD:
            res = left_ + right_;
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_SUB:
            res = left_ - right_;
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_DIV:
            res = left_ / right_;
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_MUL:
            res = left_ * right_;
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_POW:
            res = pow(left_, right_);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_EXP:
            res = exp(left_);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_SQRT:
            res = sqrt(left_);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_LOG:
            res = log2(left_); 
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_SIN:
            res = sin(left_);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_COS:
            res = cos(res);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_TAN:
            res = tan(left_);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_CTG:
            res = tan(left_);
            CHECK_MATH_ERR;
            res = 1.f / res;
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_SH:
            res = sinh(left_);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_CH:
            res = cosh(left_);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_TH:
            res = tanh(left_);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_ASIN:
            res = asin(left_);
            CHECK_MATH_ERR_AND_RET;
            return res;
        case OPERATOR_TYPE_ACOS:
            res = acos(left_);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_ATAN:
            res = atan(left_);
            CHECK_MATH_ERR_AND_RET;
        case OPERATOR_TYPE_ACTG:
            res = atan(left_);
            CHECK_MATH_ERR;
            res = 1.f / res;
            CHECK_MATH_ERR_AND_RET;
        default:
            UTILS_LOGE(LOG_CTG_DMATH, "unknown operator %d", node->value.op_type);
            return NAN;
    }
}

#undef left_
#undef right_
#undef CHECK_MATH_ERR
#undef CHECK_MATH_ERR_AND_RET


bool diff_tree_subtree_holds_var(DiffTreeNode* node)
{
    utils_assert(node);
    bool ret = false;

    if(node->right) {
        ret = diff_tree_subtree_holds_var(node->right);
        if(ret) return true;
    }

    if(node->left) {
        ret = diff_tree_subtree_holds_var(node->left);
        if(ret) return true;
    }

    if(node->type == NODE_TYPE_VAR)
        return true;
    
    return false;
}

static const char* diff_tree_get_fe_exception_str(void)
{
    IS_FE_EXCEPTION_SET = true;

    if(fetestexcept(FE_DIVBYZERO)) return "division by zero";
    if(fetestexcept(FE_INVALID))   return "domain error";
    if(fetestexcept(FE_OVERFLOW))  return "overflow";
    if(fetestexcept(FE_UNDERFLOW)) return "underflow";

    IS_FE_EXCEPTION_SET = false;

    return NULL;
}

static void diff_tree_check_math_errors(DiffTreeNode* node, double left, double right)
{
    utils_assert(node);

    const char* errstr = diff_tree_get_fe_exception_str();
    const Operator* op = get_operator(node->value.op_type);
    if(errstr) {
        if(op->argnum == 1)
            UTILS_LOGE(LOG_CTG_DMATH, 
                       "%s(%f): %s", op->str, left, errstr);
        else if(op->argnum == 2)
            UTILS_LOGE(LOG_CTG_DMATH, 
                       "%f %s %f: %s", left, 
                       op->str, right, errstr);
    }
}


