#include "difftree.h"

DiffTreeErr diff_tree_differentiate_tree(DiffTree* dtree, Variable* var);

DiffTreeNode* diff_tree_differentiate(DiffTree* dtree, DiffTreeNode* node, Variable* var);

/// @brief all variables must be set before evaluating
DiffTreeErr diff_tree_evaluate_tree(DiffTree* dtree);

/// @brief all variables must be set before evaluating
DiffTreeNode* diff_tree_evaluate(DiffTree* dtree, DiffTreeNode* node);
