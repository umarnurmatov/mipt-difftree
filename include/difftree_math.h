#pragma once
#include "difftree.h"

DiffTreeErr diff_tree_differentiate_tree_n(DiffTree* dtree, Variable* var, size_t n);

DiffTreeNode* diff_tree_differentiate(DiffTree* dtree, DiffTreeNode* node, Variable* var);

/// @brief all variables must be set before evaluating
double diff_tree_evaluate_tree(DiffTree* dtree);

/// @brief all variables must be set before evaluating
double diff_tree_evaluate(DiffTree* dtree, DiffTreeNode* node);

double diff_tree_evaluate_op(DiffTree* dtree, DiffTreeNode* node);


bool diff_tree_subtree_holds_var(DiffTreeNode* node);

DiffTreeErr diff_tree_taylor_expansion(DiffTree* dtree, Variable* var, double x0, size_t n);
