#pragma once

#include <stdlib.h>
#include <stdio.h>

#include "vector.h"
#include "types.h"
#include "variable.h"

#define DIFF_TREE_INIT_LIST           \
    {                                 \
        .root = NULL,                 \
        .size = 0,                    \
        .buf = {                      \
            .ptr = NULL,              \
            .len = 0,                 \
            .pos = 0,                 \
            .filename = NULL          \
        },                            \
        .vars = VECTOR_INITLIST,      \
        .to_delete = VECTOR_INITLIST  \
    };                      

typedef enum DiffTreeErr
{
    DIFF_TREE_ERR_NONE,
    DIFF_TREE_INVALID_BUFPOS,
    DIFF_TREE_NULLPTR,
    DIFF_TREE_ALLOC_FAIL,
    DIFF_TREE_IO_ERR,
    DIFF_TREE_SYNTAX_ERR

} DiffTreeErr;

typedef struct DiffTreeNode
{
    DiffTreeNode* left;
    DiffTreeNode* right;
    DiffTreeNode* parent;
    
    NodeType type;
    NodeValue value;

} DiffTreeNode;

typedef struct DiffTree
{
    DiffTreeNode* root;
    size_t size;

    struct {
        char* ptr;
        ssize_t len;
        ssize_t pos;
        const char* filename;
        Vector nodes;
    } buf;

    Vector vars;

    Vector to_delete;

} DiffTree;

DiffTreeErr diff_tree_ctor(DiffTree* diff_tree, const char* latex_filename);

void diff_tree_dtor(DiffTree* diff_tree);

const char* diff_tree_strerr(DiffTreeErr err);

void diff_tree_node_print(FILE* stream, void* node);

DiffTreeErr diff_tree_fwrite(DiffTree* diff_tree, const char* filename);

DiffTreeErr diff_tree_fread(DiffTree* diff_tree, const char* filename);

DiffTreeNode* diff_tree_new_node(NodeType node_type, NodeValue node_value, DiffTreeNode *left, DiffTreeNode *right, DiffTreeNode *parent);

DiffTreeNode* diff_tree_copy_subtree(DiffTree* dtree, DiffTreeNode* node, DiffTreeNode* parent);

void diff_tree_free_subtree(DiffTreeNode* node);

Variable* diff_tree_find_variable(DiffTree* dtree, utils_hash_t hash);

void diff_tree_mark_to_delete(DiffTree* dtree, DiffTreeNode* node);

void diff_tree_dump_latex(const char* fmt, ...)
    __attribute__((format(printf, 1, 2)));

void diff_tree_dump_randphrase_latex();

void diff_tree_dump_node_latex(DiffTree* dtree, DiffTreeNode* node);

void diff_tree_dump_begin_math();

void diff_tree_dump_end_math();

void diff_tree_dump_graph_latex(DiffTree* dtree, double x_begin, double x_end, double x_step);

#ifdef _DEBUG 

void diff_tree_dump(DiffTree* diff_tree, DiffTreeNode* node, DiffTreeErr err, const char* msg, const char* file, int line, const char* funcname);


#define DIFF_TREE_DUMP_NODE(diff_tree, node, err) \
    diff_tree_dump(diff_tree, node, err, NULL, __FILE__, __LINE__, __func__); 

#define DIFF_TREE_DUMP(diff_tree, err) \
    diff_tree_dump(diff_tree, (diff_tree)->root, err, NULL, __FILE__, __LINE__, __func__); 

#define DIFF_TREE_DUMP_MSG(diff_tree, err, msg) \
    diff_tree_dump(diff_tree, (diff_tree)->root, err, msg, __FILE__, __LINE__, __func__); 

#endif // _DEBUG
