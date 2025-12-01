#include "difftree.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>

#include "difftree_math.h"
#include "hashutils.h"
#include "logutils.h"
#include "memutils.h"
#include "ioutils.h"
#include "assertutils.h"
#include "logutils.h"

#include "operators.h"
#include "types.h"
#include "variable.h"
#include "vector.h"

#define LOG_CTG_DIFF_TREE "DIFFTREE"
#define NIL_STR "nil"

static FILE* file_tex = NULL;

#ifdef _DEBUG

#define DIFF_TREE_ASSERT_OK_(diff_tree)                      \
    {                                                        \
        DiffTreeErr err = diff_tree_verify_(diff_tree);  \
        if(err != DIFF_TREE_ERR_NONE) {                      \
            DIFF_TREE_DUMP(diff_tree, err);                  \
            utils_assert(err == DIFF_TREE_ERR_NONE);         \
        }                                                    \
    }

#else // _DEBUG

#define DIFF_TREE_ASSERT_OK_(diff_tree) 

#endif // _DEBUG

static DiffTreeErr diff_tree_fwrite_node_(DiffTreeNode* node, FILE* file);

static void diff_tree_add_variable_(DiffTree* dtree, Variable new_var);


static char* diff_tree_node_value_str_(DiffTree* dtree, NodeType node_type, NodeValue val);


static DiffTreeErr diff_tree_scan_node_name_(DiffTree* dtree);

static int diff_tree_advance_buf_pos_(DiffTree* dtree);

static void diff_tree_skip_spaces_(DiffTree* dtree);

ATTR_UNUSED static void diff_tree_print_node_ptr_(FILE* file, void* ptr);

static bool diff_tree_node_need_parentheses_(DiffTreeNode* node);

static DiffTreeErr diff_tree_init_latex_file_(const char* filename);

static void diff_tree_end_latex_file_();

// PARSING //

DiffTreeNode* diff_tree_parse_get_general_(DiffTree* dtree);

DiffTreeNode* diff_tree_parse_get_expr_(DiffTree* dtree);

DiffTreeNode* diff_tree_parse_get_mul_div_(DiffTree* dtree);

DiffTreeNode* diff_tree_parse_get_pow_(DiffTree* dtree);

DiffTreeNode* diff_tree_parse_get_primary_(DiffTree* dtree);

DiffTreeNode* diff_tree_parse_get_number_(DiffTree* dtree);

DiffTreeNode* diff_tree_parse_get_var_(DiffTree* dtree);

DiffTreeNode* diff_tree_parse_get_func_(DiffTree* dtree);


#ifdef _DEBUG

char* diff_tree_dump_graphviz_(DiffTree* diff_tree, DiffTreeNode* node);

void diff_tree_dump_node_graphviz_(DiffTree* dtree, FILE* file, DiffTreeNode* node, int rank);

DiffTreeErr diff_tree_verify_(DiffTree* diff_tree);

#endif // _DEBUG

#define DEFAULT_VAR_VECTOR_CAPACITY       10
#define DEFAULT_TO_DELETE_VECTOR_CAPACITY 10
#define DEFAULT_NODES_VECTOR_CAPACITY     10

DiffTreeErr diff_tree_ctor(DiffTree* diff_tree, const char* latex_filename)
{
    utils_assert(diff_tree);

    DiffTreeErr err = DIFF_TREE_ERR_NONE;

    diff_tree->size = 0;
    
    err = diff_tree_init_latex_file_(latex_filename);
    err == DIFF_TREE_ERR_NONE verified(return err);

    vector_ctor(&diff_tree->vars, DEFAULT_VAR_VECTOR_CAPACITY, sizeof(Variable));
    vector_ctor(&diff_tree->to_delete, DEFAULT_TO_DELETE_VECTOR_CAPACITY, sizeof(DiffTreeNode*));

    return DIFF_TREE_ERR_NONE;
}

void diff_tree_dtor(DiffTree* diff_tree)
{
    utils_assert(diff_tree);

    diff_tree_end_latex_file_();

    diff_tree_free_subtree(diff_tree->root); 

    diff_tree->size = 0;
    diff_tree->root = NULL;

    NFREE(diff_tree->buf.ptr);
    diff_tree->buf.pos = 0;
    diff_tree->buf.len = 0;

    vector_dtor(&diff_tree->vars);

    for(size_t i = 0; i < diff_tree->to_delete.size; ++i)
        diff_tree_free_subtree(*(DiffTreeNode**)vector_at(&diff_tree->to_delete, i));

    vector_dtor(&diff_tree->to_delete);
}

void diff_tree_free_subtree(DiffTreeNode* node)
{
    if(!node) return;

    if(node->left)
        diff_tree_free_subtree(node->left);
    if(node->right)
        diff_tree_free_subtree(node->right);

    NFREE(node);
}

void diff_tree_mark_to_delete(DiffTree* dtree, DiffTreeNode* node)
{
    vector_push(&dtree->to_delete, &node);
}

DiffTreeErr diff_tree_fwrite(DiffTree* diff_tree, const char* filename)
{
    utils_assert(filename);

    FILE* file = open_file(filename, "w");
    file verified(return DIFF_TREE_IO_ERR);

    DiffTreeErr err = diff_tree_fwrite_node_(diff_tree->root, file);

    UTILS_LOGD(LOG_CTG_DIFF_TREE, "Writing done");

    fclose(file);

    return err;
}

DiffTreeErr diff_tree_fwrite_node_(DiffTreeNode* node, FILE* file)
{
    utils_assert(node);
    utils_assert(file);

    DiffTreeErr err = DIFF_TREE_ERR_NONE;
    int io_err = 0;

    io_err = fprintf(file, "(");
    io_err >= 0 verified(return DIFF_TREE_IO_ERR);

    if(node->left)
        err = diff_tree_fwrite_node_(node->left, file);
    else
        fprintf(file, NIL_STR);

    if(node->right)
        err = diff_tree_fwrite_node_(node->right, file);
    else
        fprintf(file, " " NIL_STR " ");

    io_err = fprintf(file, ")");
    io_err >= 0 verified(return DIFF_TREE_IO_ERR);

    return err;
}

int diff_tree_advance_buf_pos_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    if(dtree->buf.pos < dtree->buf.len - 1) {
        dtree->buf.pos++;
        return 0;
    }
    else
        return 1;
}

void diff_tree_skip_spaces_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    while(isspace(dtree->buf.ptr[dtree->buf.pos])) {
        if(diff_tree_advance_buf_pos_(dtree))
            break;
    }
}

static void diff_tree_print_node_ptr_(FILE* file, void* ptr)
{
    fprintf(file, "%p", *(DiffTreeNode**)ptr);
}

DiffTreeErr diff_tree_scan_node_name_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    int name_len = 0;
    sscanf(dtree->buf.ptr + dtree->buf.pos, " %*[^ ]%n", &name_len);

    dtree->buf.pos += (ssize_t) name_len;

    dtree->buf.ptr[dtree->buf.pos] = '\0';

    return DIFF_TREE_ERR_NONE;
}

static void diff_tree_add_variable_(DiffTree* dtree, Variable new_var)
{
    for(size_t i = 0; i < dtree->vars.size; ++i)
        if(((Variable*)vector_at(&dtree->vars, i))->c == new_var.c)
            return;

    vector_push(&dtree->vars, &new_var);
    VECTOR_DUMP(&dtree->vars, VECTOR_ERR_NONE, NULL, variable_print_callback);
}

Variable* diff_tree_find_variable(DiffTree* dtree, utils_hash_t hash) 
{
    // FIXME use binsearch

    Vector* vars = &dtree->vars;
    for(size_t i = 0; i < vars->size; ++i) {
        Variable* var = (Variable*)vector_at(vars, i);
        if(hash == var->hash)
            return var;
    }

    return NULL;
}

#define LOG_SYNTAX_ERR_(msg, ...)           \
    UTILS_LOGE(                             \
        LOG_CTG_DIFF_TREE,                  \
        "%s:1:%ld: syntax error: " msg,     \
        dtree->buf.filename,                \
        dtree->buf.pos,                     \
        __VA_ARGS__ );

#define BUF_AT_POS_ dtree->buf.ptr[dtree->buf.pos]

#define BUF_AT_PREV_POS_ dtree->buf.ptr[pos_prev]

#define POS_ dtree->buf.pos

#define LEN_ dtree->buf.len

#define INCREMENT_POS_                 \
    diff_tree_advance_buf_pos_(dtree); \
    diff_tree_skip_spaces_(dtree);     

#define ADD_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_ADD }, left, right, NULL)

#define SUB_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_SUB }, left, right, NULL)

#define MUL_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_MUL }, left, right, NULL)

#define DIV_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_DIV }, left, right, NULL)

#define POW_(left, right) \
    diff_tree_new_node(NODE_TYPE_OP, NodeValue { OPERATOR_TYPE_POW }, left, right, NULL)

#define CONST_(num_) \
    diff_tree_new_node(NODE_TYPE_NUM, NodeValue { .num = num_ }, NULL, NULL, NULL)

#define VAR_(var) \
    diff_tree_new_node(NODE_TYPE_VAR, NodeValue { .var_hash = var->hash }, NULL, NULL, NULL)


DiffTreeNode* diff_tree_parse_get_number_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    int val = 0;
    ssize_t pos_prev = dtree->buf.pos;
    DiffTreeNode* node = NULL;

    while('0' <= BUF_AT_POS_ && BUF_AT_POS_ < '9') {
        val = (BUF_AT_POS_ - '0') + val * 10;
        diff_tree_advance_buf_pos_(dtree);
    }

    if(pos_prev != POS_) {
        node = CONST_(val);
        diff_tree_mark_to_delete(dtree, node);
    }

    return node;
}

DiffTreeNode* diff_tree_parse_get_var_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    ssize_t pos_prev = dtree->buf.pos;
    Variable var = { .c = ' ', .hash = 0, .val = 0 };
    DiffTreeNode* node = NULL;

    if(isalpha(BUF_AT_POS_)) {
        var.c = BUF_AT_POS_;
        var.hash = utils_djb2_hash(dtree->buf.ptr + dtree->buf.pos, sizeof(char));

        diff_tree_add_variable_(dtree, var);

        INCREMENT_POS_;

        node = diff_tree_new_node(
                NODE_TYPE_VAR, 
                NodeValue { .var_hash = var.hash },
                NULL,
                NULL,
                NULL );
        diff_tree_mark_to_delete(dtree, node);
    }

    return node;
}

DiffTreeNode* diff_tree_parse_get_expr_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    DiffTreeNode* node = diff_tree_parse_get_mul_div_(dtree);

    while(BUF_AT_POS_ == '+' || BUF_AT_POS_ == '-') {
        ssize_t pos_prev = POS_;
        INCREMENT_POS_;

        DiffTreeNode* node_new = diff_tree_parse_get_mul_div_(dtree);

        if(BUF_AT_PREV_POS_ == '+')
            node = ADD_(node, node_new);
        else
            node = SUB_(node, node_new);

        diff_tree_mark_to_delete(dtree, node);
    }

    // if(pos_prev == POS_) {
    //     UTILS_LOGE(LOG_CTG_DIFF_TREE, "syntax err on %ld", POS_);
    //     return NULL;
    // }

    return node;
}

DiffTreeNode* diff_tree_parse_get_mul_div_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    DiffTreeNode* node = diff_tree_parse_get_pow_(dtree);

    while(BUF_AT_POS_ == '*' || BUF_AT_POS_ == '/') {
        ssize_t pos_prev = POS_;
        INCREMENT_POS_;
        DiffTreeNode* node_right = diff_tree_parse_get_pow_(dtree);

        if(BUF_AT_PREV_POS_ == '*')
            node = MUL_(node, node_right);
        else
            node = DIV_(node, node_right);

        diff_tree_mark_to_delete(dtree, node);
    }
    
    return node;
}

DiffTreeNode* diff_tree_parse_get_pow_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    DiffTreeNode* node = diff_tree_parse_get_primary_(dtree);
    
    ssize_t pos_prev = POS_;
    while(BUF_AT_POS_ == '^') {

        INCREMENT_POS_;
        DiffTreeNode* node_new = diff_tree_parse_get_primary_(dtree);

        node = POW_(node, node_new);
        diff_tree_mark_to_delete(dtree, node);
    }
    
    // if(pos_prev == POS_) {
    //     UTILS_LOGE(LOG_CTG_DIFF_TREE, "syntax err on %ld", POS_);
    //     return NULL;
    // }

    return node;
}

DiffTreeNode* diff_tree_parse_get_func_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    ssize_t bufpos = 0;
    char buf[MAX_OP_NAME_LEN] = "";

    ssize_t pos_prev = POS_;
    DiffTreeNode* node = NULL;

    while(POS_ < LEN_ && isalpha(BUF_AT_POS_)) {
        buf[bufpos++] = BUF_AT_POS_;
        INCREMENT_POS_;
    }
    
    const Operator* op = match_function(buf);
    if(!op) {
        POS_ = pos_prev;
        return NULL;
    }

    if(BUF_AT_POS_ == '(') {
        INCREMENT_POS_;

        node = diff_tree_parse_get_expr_(dtree);

        INCREMENT_POS_;

        node = diff_tree_new_node(NODE_TYPE_OP, NodeValue { .op_type = op->type }, node, NULL, NULL);
        diff_tree_mark_to_delete(dtree, node);
    }
    
    return node;
}

DiffTreeNode* diff_tree_parse_get_primary_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    DiffTreeNode* node = NULL;

    if(BUF_AT_POS_ == '(') {
        INCREMENT_POS_;

        node = diff_tree_parse_get_expr_(dtree);

        INCREMENT_POS_;

        return node;
    }

    node = diff_tree_parse_get_number_(dtree);
    if(node) return node;

    node = diff_tree_parse_get_func_(dtree);
    if(node) return node;

    node = diff_tree_parse_get_var_(dtree);

    return node;
}

DiffTreeNode* diff_tree_parse_get_general_(DiffTree* dtree)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    DiffTreeNode* node = diff_tree_parse_get_expr_(dtree);

    if(dtree->buf.ptr[dtree->buf.pos] != '\n') {
        LOG_SYNTAX_ERR_("expected: \\n, got: (ASCII) %d", (int)BUF_AT_POS_);
        return NULL;
    }
    INCREMENT_POS_;

    return node;
}

#undef LOG_SYNTAX_ERR_
#undef BUF_AT_POS_
#undef BUF_AT_PREV_POS_
#undef POS_
#undef LEN_
#undef INCREMENT_POS_
#undef ADD_
#undef SUB_
#undef MUL_
#undef DIV_
#undef POW_
#undef CONST_
#undef VAR_

DiffTreeErr diff_tree_fread(DiffTree* dtree, const char* filename)
{
    utils_assert(dtree);
    utils_assert(filename);

    FILE* file = open_file(filename, "r");
    file verified(return DIFF_TREE_IO_ERR);
    
    size_t fsize = get_file_size(file);

    dtree->buf.ptr = TYPED_CALLOC(fsize, char);
    dtree->buf.ptr verified(return DIFF_TREE_ALLOC_FAIL);

    dtree->buf.filename = filename;

    size_t bytes_transferred = fread(dtree->buf.ptr, sizeof(dtree->buf.ptr[0]), fsize, file);
    fclose(file);
    // TODO check for errors
    dtree->buf.len = (unsigned) bytes_transferred;
    
    DiffTreeErr err = DIFF_TREE_ERR_NONE;

    dtree->root = diff_tree_parse_get_general_(dtree);

    if(!dtree->root) {
        err = DIFF_TREE_SYNTAX_ERR;
        DIFF_TREE_DUMP(dtree, err);
        
        for(size_t i = 0; i < dtree->to_delete.size; ++i)
            NFREE(*(DiffTreeNode**)vector_at(&dtree->to_delete, i));

        return err;
    }

    vector_free(&dtree->to_delete);

    dtree->root = diff_tree_new_node(NODE_TYPE_FAKE, NodeValue { .num = NAN }, 
                                     dtree->root, NULL, NULL);

    DIFF_TREE_DUMP(dtree, err);

    return DIFF_TREE_ERR_NONE;
}

const char* diff_tree_strerr(DiffTreeErr err)
{
    switch(err) {
        case DIFF_TREE_ERR_NONE:
            return "none";
        case DIFF_TREE_NULLPTR:
            return "passed a nullptr";
        case DIFF_TREE_INVALID_BUFPOS:
            return "buffer position invalid";
        case DIFF_TREE_ALLOC_FAIL:
            return "memory allocation failed";
        case DIFF_TREE_IO_ERR:
            return "io error";
        case DIFF_TREE_SYNTAX_ERR:
            return "syntax error";
        default:
            return "unknown";
    }
}

void diff_tree_node_print(FILE* stream, void* node)
{
    utils_assert(stream);
    utils_assert(node);

    DiffTreeNode* node_ = (DiffTreeNode*) node;
    fprintf(stream, "[%p; l: %p; r: %p; p: %p]", node_, node_->left, node_->right, node_->parent);
}

DiffTreeNode* diff_tree_new_node(NodeType node_type, NodeValue node_value, DiffTreeNode *left, DiffTreeNode *right, DiffTreeNode *parent)
{
    DiffTreeNode* node = TYPED_CALLOC(1, DiffTreeNode);

    if(!node) return NULL;

    *node = {
        .left = left,
        .right = right,
        .parent = parent,
        .type = node_type,
        .value = node_value,
    };

    if(right)
        node->right->parent = node;

    if(left)
        node->left->parent = node;

    return node;
}

DiffTreeNode* diff_tree_copy_subtree(DiffTree* dtree, DiffTreeNode* node, DiffTreeNode* parent)
{
    DiffTreeNode *new_node = diff_tree_new_node(node->type, node->value, NULL, NULL, NULL);

    if(node->left)
        new_node->left = diff_tree_copy_subtree(dtree, node->left, new_node);

    if(node->right)
        new_node->right = diff_tree_copy_subtree(dtree, node->right, new_node);

    new_node->parent = parent;

    return new_node;
}

#define BUF_LEN_ 100
static char* diff_tree_node_value_str_(DiffTree* dtree, NodeType node_type, NodeValue val)
{
    static char buffer[BUF_LEN_] = "";

    switch(node_type) {
        case NODE_TYPE_OP:
            return const_cast<char*>(node_op_type_str(val.op_type));
        case NODE_TYPE_VAR: 
        {
            Variable* var = diff_tree_find_variable(dtree, val.var_hash);
            if(var) {
                sprintf(buffer, "%c", var->c);
                return buffer;
            }
            break;
        }
        case NODE_TYPE_NUM:
            strfromd(buffer, BUF_LEN_, "%f", val.num);
            return buffer;
        case NODE_TYPE_FAKE:
            return const_cast<char*>("fakeval");
        default:
            return const_cast<char*>("???");
    }

    return NULL;
}
#undef BUF_LEN_

static const char* phrases[] = {
    "Формула красивая, но бесполезная",
    "Производную пока написал без пределов, но этот беспредел мы скоро устраним",
    "Я все делал по науке, и у меня не получилось... Ну да плевать",
    "И вот если мы берём симплициальную резольвенту у группы G, то у каждой симплициальной группы есть гомотопические группы",
    "Очевидно, что",
    "Это несложное доказательство оставим любопытному читателю в качестве несложного упражнения",
    "Это утверждение предствляет собой переформулировку предложения 5.3.18, которое непосредтвенно вытекает из теоремы 7.2.134"
};

static DiffTreeErr diff_tree_init_latex_file_(const char* filename)
{
    file_tex = open_file(filename, "w");
    file_tex verified(return DIFF_TREE_IO_ERR);
    
    fprintf(
        file_tex, 
        "\\documentclass[a4paper,12pt]{article}\n"
        "\\usepackage[a4paper,top=1.3cm,bottom=2cm,left=1.5cm,right=1.5cm]{geometry}\n"
        "\\usepackage[T2A, T1]{fontenc}\n"
        "\\usepackage[english,russian]{babel}\n"
        "\\usepackage{amsmath,amsfonts,amssymb,amsthm,mathtools}\n"
        "\\usepackage{breqn}\n"
        "\\usepackage{hyperref}\n"
        "\\usepackage{pgfplots}\n"
        "\\pgfplotsset{compat=1.18}\n"
        "\\breqnsetup{breakdepth={1}}\n"
        "\\title{Дифференциальное исчисление на простейших примерах}\n"
        "\\author{Нурматов Умархон Акмалович, д.т.н, д.ф.-м.н.}\n"
        "\\begin{document}\n\n"
        "\\maketitle\n"
        "\\tableofcontents\n"
    );

    return DIFF_TREE_ERR_NONE;
}

static void diff_tree_end_latex_file_()
{
    utils_assert(file_tex);

    fprintf(file_tex,
            "\n\\end{document}\n");

    fclose(file_tex);
}


static bool diff_tree_node_need_parentheses_(DiffTreeNode* node)
{
    utils_assert(node);

    if (!node->parent || node->parent->type == NODE_TYPE_FAKE)
        return false;

    if (node->type != NODE_TYPE_OP || node->parent->type != NODE_TYPE_OP)
        return false;

    if(node->parent->value.op_type == OPERATOR_TYPE_DIV ||
       node->parent->value.op_type == OPERATOR_TYPE_SQRT)
        return false;

    const Operator* op_node = get_operator(node->value.op_type);
    const Operator* op_parent = get_operator(node->parent->value.op_type);

    if(op_node->argnum == OPERATOR_ARGNUM_1 && op_parent->type == OPERATOR_TYPE_POW)
        return true;

    if(op_parent->precedance > op_node->precedance)
        return true;

    if(op_parent->precedance == op_node->precedance)
        if((node->parent->value.op_type == OPERATOR_TYPE_SUB ||
           node->parent->value.op_type == OPERATOR_TYPE_POW) &&
           node == node->parent->right)
            return true;

    return false;
}

void diff_tree_dump_node_latex(DiffTree* dtree, DiffTreeNode* node)
{
    DIFF_TREE_ASSERT_OK_(dtree);
    utils_assert(node);
    utils_assert(file_tex);

    if(node->type == NODE_TYPE_OP) {
        if(diff_tree_node_need_parentheses_(node)) fprintf(file_tex, "\\left (");
        fprintf(file_tex, "%s", get_operator(node->value.op_type)->latex_str_pref);
    }

    if(node->left)
        diff_tree_dump_node_latex(dtree, node->left);

    switch(node->type) {
        case NODE_TYPE_VAR:
            fprintf(file_tex, " %c ", diff_tree_find_variable(dtree, node->value.var_hash)->c);
            break;
        case NODE_TYPE_OP:
            fprintf(file_tex, " %s ", get_operator(node->value.op_type)->latex_str_inf);
            break;
        case NODE_TYPE_NUM:
            fprintf(file_tex, " %g ", node->value.num);
        case NODE_TYPE_FAKE:
            UTILS_LOGW(LOG_CTG_DIFF_TREE, "fake node occured");
            break;
        default:
            UTILS_LOGW(LOG_CTG_DIFF_TREE, "unrecognized node type");
            break;
    }

    if(node->right)
        diff_tree_dump_node_latex(dtree, node->right);

    if(node->type == NODE_TYPE_OP) {
        fprintf(file_tex, "%s", get_operator(node->value.op_type)->latex_str_post);
        if(diff_tree_node_need_parentheses_(node)) fprintf(file_tex, "\\right )");
    }
}

void diff_tree_dump_randphrase_latex()
{
    fprintf(file_tex, "%s\n", phrases[(unsigned) rand() % SIZEOF(phrases)]);
}

void diff_tree_dump_latex(const char* fmt, ...)
{
    va_list va_arg_list;
    va_start(va_arg_list, fmt);
    vfprintf(file_tex, fmt, va_arg_list);
    va_end(va_arg_list);
}

void diff_tree_dump_begin_math()
{
    diff_tree_dump_latex("\\begin{dmath}\n");
}

void diff_tree_dump_end_math()
{
    diff_tree_dump_latex("\\end{dmath}\n\n");
}

void diff_tree_dump_graph_latex(DiffTree* dtree, double x_begin, double x_end, double x_step)
{
    fprintf(file_tex,
        "\\begin{tikzpicture}\n"
        "\\begin{axis}[\n"
            "xlabel={X},\n"
            "ylabel={Y},\n"
            "grid=major,\n"
            "legend pos=north west\n"
        "]\n\n"
        "\\addplot[\n"
        "    mark size=0pt,\n"
        "    color=blue\n"
        "] coordinates {\n");

    for(double x = x_begin; x < x_end; x += x_step) {
        ((Variable*)vector_at(&dtree->vars, 0))->val = x;
        double val = diff_tree_evaluate_tree(dtree);
        fprintf(file_tex, "(%f,%f)\n", x, val);
    }

    fprintf(file_tex, 
        "};\n"
        "\\end{axis}\n"
        "\\end{tikzpicture}\n");

}

#ifdef _DEBUG

#define GRAPHVIZ_FNAME_ "graphviz"
#define GRAPHVIZ_CMD_LEN_ 100

#define CLR_RED_LIGHT_   "\"#FFB0B0\""
#define CLR_GREEN_LIGHT_ "\"#B0FFB0\""
#define CLR_BLUE_LIGHT_  "\"#B0B0FF\""

#define CLR_RED_BOLD_    "\"#FF0000\""
#define CLR_GREEN_BOLD_  "\"#03C03C\""
#define CLR_BLUE_BOLD_   "\"#0000FF\""

void diff_tree_dump(DiffTree* diff_tree, DiffTreeNode* node, DiffTreeErr err, const char* msg, const char* filename, int line, const char* funcname)
{
    utils_log_fprintf(
        "<style>"
        "table {"
          "border-collapse: collapse;"
          "border: 1px solid;"
          "font-size: 0.9em;"
        "}"
        "th,"
        "td {"
          "border: 1px solid rgb(160 160 160);"
          "padding: 8px 10px;"
        "}"
        "</style>\n"
    );

    utils_log_fprintf("<pre>\n"); 

    time_t cur_time = time(NULL);
    struct tm* iso_time = localtime(&cur_time);
    char time_buff[100];
    strftime(time_buff, sizeof(time_buff), "%F %T", iso_time);

    if(err != DIFF_TREE_ERR_NONE) {
        utils_log_fprintf("<h3 style=\"color:red;\">[ERROR] [%s] from %s:%d: %s() </h3>", time_buff, filename, line, funcname);
        utils_log_fprintf("<h4><font color=\"red\">err: %s </font></h4>", diff_tree_strerr(err));
    }
    else
        utils_log_fprintf("<h3>[DEBUG] [%s] from %s:%d: %s() </h3>\n", time_buff, filename, line, funcname);

    if(msg)
        utils_log_fprintf("what: %s\n", msg);


    BEGIN {
        if(!diff_tree->buf.ptr) GOTO_END;

        utils_log_fprintf("buf.pos = %ld\n", diff_tree->buf.pos); 
        utils_log_fprintf("buf.len = %ld\n", diff_tree->buf.len); 
        utils_log_fprintf("buf.ptr[%p] = ", diff_tree->buf.ptr); 

        if(err == DIFF_TREE_INVALID_BUFPOS) {
            for(ssize_t i = 0; i < diff_tree->buf.len; ++i)
                utils_log_fprintf("%c", diff_tree->buf.ptr[i]);
            utils_log_fprintf("\n");
            GOTO_END;
        }

#define CLR_PREV "#09AB00"
#define CLR_CUR  "#C71022"
#define CLR_NEXT "#1022C7"

#define LOG_PRINTF_CHAR(ch, col) \
    if(ch == '\0') \
        utils_log_fprintf("<span style=\"border: 1px solid" col ";\">0</span>"); \
    else if(isspace(ch)) \
        utils_log_fprintf("<span style=\"border: 1px solid" col ";\"> </span>"); \
    else \
        utils_log_fprintf("%c", ch);

        utils_log_fprintf("<font color=\"" CLR_PREV "\">"); 
        for(ssize_t i = 0; i < diff_tree->buf.pos; ++i) {
            LOG_PRINTF_CHAR(diff_tree->buf.ptr[i], CLR_PREV);
        }
        utils_log_fprintf("</font>");


        utils_log_fprintf("<font color=\"" CLR_CUR "\"><b>");
        LOG_PRINTF_CHAR(diff_tree->buf.ptr[diff_tree->buf.pos], CLR_CUR);
        utils_log_fprintf("</b></font>");


        utils_log_fprintf("<font color=\"" CLR_NEXT "\">"); 
        for(ssize_t i = diff_tree->buf.pos + 1; i < diff_tree->buf.len; ++i) {
            LOG_PRINTF_CHAR(diff_tree->buf.ptr[i], CLR_NEXT);
        }
        utils_log_fprintf("</font>\n");

    } END;

#undef CLR_PREV
#undef CLR_CUR
#undef CLR_NEXT
#undef LOG_PRINTF_CHAR

    char* img_pref = diff_tree_dump_graphviz_(diff_tree, node);

    utils_log_fprintf(
        "\n<img src=" IMG_DIR "/%s.svg\n", 
        strrchr(img_pref, '/') + 1
    );

    utils_log_fprintf("</pre>\n"); 

    utils_log_fprintf("<hr color=\"black\" />\n");

    NFREE(img_pref);
}

char* diff_tree_dump_graphviz_(DiffTree* diff_tree, DiffTreeNode* node)
{
    utils_assert(diff_tree);

    FILE* file = open_file(LOG_DIR "/" GRAPHVIZ_FNAME_ ".txt", "w");

    if(!file)
        exit(EXIT_FAILURE);

    fprintf(file, "digraph {\n rankdir=TB;\n"); 
    fprintf(file, "nodesep=0.9;\nranksep=0.75;\n");

    diff_tree_dump_node_graphviz_(diff_tree, file, node, 1);

    fprintf(file, "};");

    fclose(file);
    
    create_dir(LOG_DIR "/" IMG_DIR);
    char* img_tmpnam = tempnam(LOG_DIR "/" IMG_DIR, "img-");
    utils_assert(img_tmpnam);

    static char strbuf[GRAPHVIZ_CMD_LEN_]= "";

    snprintf(
        strbuf, 
        GRAPHVIZ_CMD_LEN_, 
        "dot -T svg -o %s.svg " LOG_DIR "/" GRAPHVIZ_FNAME_ ".txt", 
        img_tmpnam
    );

    system(strbuf);

    return img_tmpnam;
}

void diff_tree_dump_node_graphviz_(DiffTree* dtree, FILE* file, DiffTreeNode* node, int rank)
{
    utils_assert(file);

    if(!node) return;

    if(node->left)
        diff_tree_dump_node_graphviz_(dtree, file, node->left, rank + 1); 
    if(node->right) 
        diff_tree_dump_node_graphviz_(dtree, file, node->right, rank + 1);

    if(!node->left && !node->right)
        fprintf(
            file, 
            "node_%p["
            "shape=record,"
            "label=\" { parent: %p | addr: %p | type: %s | val: %s | { L: %p | R: %p } } \","
            "style=\"filled\","
            "color=" CLR_GREEN_BOLD_ ","
            "fillcolor=" CLR_GREEN_LIGHT_ ","
            "rank=%d"
            "];\n",
            node,
            node->parent,
            node,
            node_type_str(node->type),
            diff_tree_node_value_str_(dtree, node->type, node->value),
            node->left,
            node->right,
            rank
        );
    else
        fprintf(
            file, 
            "node_%p["
            "shape=none,"
            "label=<"
            "<table cellspacing=\"0\" border=\"0\" cellborder=\"1\">"
              "<tr>"
                "<td colspan=\"2\">parent %p</td>"
              "</tr>"
              "<tr>"
                "<td colspan=\"2\">addr: %p</td>"
              "</tr>"
              "<tr>"
                "<td colspan=\"2\">type: %s</td>"
              "</tr>"
              "<tr>"
                "<td colspan=\"2\">val: %s</td>"
              "</tr>"
              "<tr>"
                "<td bgcolor=" CLR_RED_LIGHT_ ">L: %p</td>"
                "<td bgcolor=" CLR_BLUE_LIGHT_">R: %p</td>"
              "</tr>"
            "</table>>,"
            "rank=%d,"
            "];\n",
            node,
            node->parent,
            node,
            node_type_str(node->type),
            diff_tree_node_value_str_(dtree, node->type, node->value),
            node->left,
            node->right,
            rank
        );

    if(node->left) {
        if(node->left->parent == node)
            fprintf(
                file,
                "node_%p -> node_%p ["
                "dir=both," 
                "color=" CLR_RED_BOLD_ ","
                "fontcolor=" CLR_RED_BOLD_ ","
                "];\n",
                node, 
                node->left
            );
        else
            fprintf(
                file,
                "node_%p -> node_%p ["
                "color=" CLR_RED_BOLD_ ","
                "fontcolor=" CLR_RED_BOLD_ ","
                "];\n",
                node, 
                node->left
            );
    }

    if(node->right) {
        if(node->right->parent == node)
            fprintf(
                file,
                "node_%p -> node_%p ["
                "dir=both," 
                "color=" CLR_BLUE_BOLD_ ","
                "fontcolor=" CLR_BLUE_BOLD_ ","
                "];\n",
                node, 
                node->right
            );
        else
            fprintf(
                file,
                "node_%p -> node_%p ["
                "color=" CLR_BLUE_BOLD_ ","
                "fontcolor=" CLR_BLUE_BOLD_ ","
                "];\n",
                node, 
                node->right
            );
    }
}

DiffTreeErr diff_tree_verify_(DiffTree* diff_tree)
{
    if(!diff_tree)
        return DIFF_TREE_NULLPTR;

    if(diff_tree->buf.pos > diff_tree->buf.len)
        return DIFF_TREE_INVALID_BUFPOS;
    
    return DIFF_TREE_ERR_NONE;
}


#endif // _DEBUG
