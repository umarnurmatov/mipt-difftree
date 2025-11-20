#include "difftree.h"

DiffTreeNode* diff_tree_differentiate(DiffTree* dtree, DiffTreeNode* node, Variable* var);

/// @brief all variables must be set before evaluating
DiffTreeNode* diff_tree_evaluate(DiffTree* dtree, DiffTreeNode* node);
