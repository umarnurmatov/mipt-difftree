#pragma once

#include <stdlib.h>

#include "stringutils.h"
#include "stack.h"
#include "types.h"

#define DIFF_TREE_INIT_LIST \
    {                       \
        .root = NULL,       \
        .size = 0,          \
        .buf = {            \
            .ptr = NULL,    \
            .len = 0,       \
            .pos = 0        \
        }                   \
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
    } buf;

} DiffTree;

DiffTreeErr diff_tree_ctor(DiffTree* diff_tree);

void diff_tree_dtor(DiffTree* diff_tree);

DiffTreeErr diff_tree_insert(DiffTree* diff_tree, DiffTreeNode* node, DiffTreeNode** ret);

const char* diff_tree_strerr(DiffTreeErr err);

DiffTreeErr diff_tree_fwrite(DiffTree* diff_tree, const char* filename);

DiffTreeErr diff_tree_fread(DiffTree* diff_tree, const char* filename);

void diff_tree_dump(DiffTree* diff_tree, DiffTreeErr err, const char* msg, const char* file, int line, const char* funcname);

#define DIFF_TREE_DUMP(diff_tree, err) \
    diff_tree_dump(diff_tree, err, NULL, __FILE__, __LINE__, __func__); 

#define DIFF_TREE_DUMP_MSG(diff_tree, err, msg) \
    diff_tree_dump(diff_tree, err, msg, __FILE__, __LINE__, __func__); 
