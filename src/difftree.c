#include "difftree.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>

#include "colorutils.h"
#include "hashutils.h"
#include "logutils.h"
#include "memutils.h"
#include "ioutils.h"
#include "sortutils.h"
#include "stringutils.h"
#include "assertutils.h"
#include "logutils.h"

#include "operators.h"
#include "types.h"
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

static void diff_tree_free_subtree_(DiffTreeNode* node);

static DiffTreeErr diff_tree_fwrite_node_(DiffTreeNode* node, FILE* file);

static DiffTreeErr diff_tree_fread_node_(DiffTree* dtree, DiffTreeNode** node, const char* fname);



static void diff_tree_add_variable_(DiffTree* dtree, Variable new_var);

static OperatorType diff_tree_match_operator_(char* str);


static char* diff_tree_node_value_str_(DiffTree* dtree, NodeType node_type, NodeValue val);


static DiffTreeErr diff_tree_scan_node_name_(DiffTree* dtree);

static int diff_tree_advance_buf_pos_(DiffTree* dtree);

static void diff_tree_skip_spaces_(DiffTree* dtree);

static void diff_tree_dump_node_latex_(DiffTree* dtree, DiffTreeNode* node);

static DiffTreeErr diff_tree_init_latex_file_(const char* filename);

static void diff_tree_end_latex_file_();


#ifdef _DEBUG

static char* diff_tree_dump_graphviz_(DiffTree* diff_tree);

void diff_tree_dump_node_graphviz_(DiffTree* dtree, FILE* file, DiffTreeNode* node, int rank);

DiffTreeErr diff_tree_verify_(DiffTree* diff_tree);

#endif // _DEBUG

#define DEFAULT_VAR_VECTOR_CAPACITY 10
DiffTreeErr diff_tree_ctor(DiffTree* diff_tree, const char* latex_filename)
{
    utils_assert(diff_tree);

    DiffTreeErr err = DIFF_TREE_ERR_NONE;

    diff_tree->size = 0;
    
    err = diff_tree_init_latex_file_(latex_filename);
    err == DIFF_TREE_ERR_NONE verified(return err);

    vector_ctor(&diff_tree->vars, DEFAULT_VAR_VECTOR_CAPACITY, sizeof(Variable));

    return DIFF_TREE_ERR_NONE;
}

void diff_tree_dtor(DiffTree* diff_tree)
{
    utils_assert(diff_tree);

    diff_tree_free_subtree_(diff_tree->root); 

    diff_tree->size = 0;
    diff_tree->root = NULL;

    NFREE(diff_tree->buf.ptr);
    diff_tree->buf.pos = 0;
    diff_tree->buf.len = 0;

    vector_dtor(&diff_tree->vars);

    diff_tree_end_latex_file_();
}

void diff_tree_free_subtree_(DiffTreeNode* node)
{
    if(!node) return;

    if(node->left)
        diff_tree_free_subtree_(node->left);
    if(node->right)
        diff_tree_free_subtree_(node->right);

    NFREE(node);
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
    vector_push(&dtree->vars, &new_var);
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

static OperatorType diff_tree_match_operator_(char* str)
{
    // FIXME make hash comparasion
    for(size_t i = 0; i < SIZEOF(op_arr); ++i) {
        if(!strcmp(str, op_arr[i].str)) {
            return op_arr[i].type;
        }
    }

    return OPERATOR_TYPE_NONE;
}

#define DIFF_TREE_LOG_SYNTAX_ERR(fname, dtree, expc)                                       \
    UTILS_LOGE(                                                                            \
        LOG_CTG_DIFF_TREE,                                                                 \
        "%s:1:%ld: syntax error: unexpected symbol <%c> (ASCII:%d), expected <" expc ">",  \
        fname,                                                                             \
        dtree->buf.pos,                                                                    \
        dtree->buf.ptr[dtree->buf.pos],                                                    \
        (int)dtree->buf.ptr[dtree->buf.pos]                                                \
    );                                                                          

DiffTreeErr diff_tree_fread_node_(DiffTree* dtree, DiffTreeNode** node, const char* fname)
{
    DIFF_TREE_ASSERT_OK_(dtree);
    utils_assert(node);
    utils_assert(fname);

    DiffTreeErr err = DIFF_TREE_ERR_NONE;

    DIFF_TREE_DUMP(dtree, err);

    if(dtree->buf.ptr[dtree->buf.pos] == '(') {
        
        *node = TYPED_CALLOC(1, DiffTreeNode);

        diff_tree_advance_buf_pos_(dtree);
        diff_tree_skip_spaces_(dtree);

        ssize_t buf_pos_id_begin = dtree->buf.pos;
        err = diff_tree_scan_node_name_(dtree);
        err == DIFF_TREE_ERR_NONE verified(return err);
        ssize_t buf_pos_id_end = dtree->buf.pos;

        diff_tree_advance_buf_pos_(dtree);
        diff_tree_skip_spaces_(dtree);

        err = diff_tree_fread_node_(dtree, &(*node)->left, fname);
        err == DIFF_TREE_ERR_NONE verified(return err);

        err = diff_tree_fread_node_(dtree, &(*node)->right, fname);
        err == DIFF_TREE_ERR_NONE verified(return err);

        if(!(*node)->left && !(*node)->right) {
            if(isdigit(dtree->buf.ptr[buf_pos_id_begin])) {
                double num = atof(dtree->buf.ptr + buf_pos_id_begin);
                (*node)->value.num = num;
                (*node)->type = NODE_TYPE_NUM;

                UTILS_LOGD(LOG_CTG_DIFF_TREE, "%s:1:%ld: numerical value %f read", fname, buf_pos_id_begin, num);
            }
            else {

                utils_hash_t hash = utils_djb2_hash(dtree->buf.ptr + buf_pos_id_begin, 
                                                    (size_t)(buf_pos_id_end - buf_pos_id_begin));
                Variable new_var = {
                    .str = dtree->buf.ptr + buf_pos_id_begin,
                    .hash = hash,
                    .val = NAN
                };

                diff_tree_add_variable_(dtree, new_var);
               
                (*node)->value.var_hash = hash;
                (*node)->type = NODE_TYPE_VAR;

                UTILS_LOGD(LOG_CTG_DIFF_TREE, "%s:1:%ld: variable hash %lu read", fname, buf_pos_id_begin, hash);
            }
        } 
        else {
            OperatorType op_type = diff_tree_match_operator_(dtree->buf.ptr + buf_pos_id_begin);

            if(op_type == OPERATOR_TYPE_NONE) {
                UTILS_LOGE(
                    LOG_CTG_DIFF_TREE,
                    "%s:1:%ld: syntax error: unknown operator",
                    fname, buf_pos_id_begin
                );

                return DIFF_TREE_SYNTAX_ERR;
            }

            (*node)->type = NODE_TYPE_OP;
            (*node)->value.op_type = op_type;

            UTILS_LOGD(LOG_CTG_DIFF_TREE, "%s:1:%ld: operator read", fname, buf_pos_id_begin);
        } 


        if(dtree->buf.ptr[dtree->buf.pos] != ')') {
            DIFF_TREE_LOG_SYNTAX_ERR(fname, dtree, ")");
            return DIFF_TREE_SYNTAX_ERR;
        }

        diff_tree_advance_buf_pos_(dtree);
        diff_tree_skip_spaces_(dtree);

    }
    else if(strncmp(dtree->buf.ptr + dtree->buf.pos, NIL_STR, SIZEOF(NIL_STR) - 1) == 0) {

        if(dtree->buf.ptr[dtree->buf.pos] != 'n') {
            DIFF_TREE_LOG_SYNTAX_ERR(fname, dtree, "n");
            return DIFF_TREE_SYNTAX_ERR;
        }

        dtree->buf.pos += (ssize_t)(SIZEOF(NIL_STR) - 1);
        diff_tree_skip_spaces_(dtree);

        *node = NULL;
    }
    else {
        DIFF_TREE_LOG_SYNTAX_ERR(fname, dtree, "(");
        return DIFF_TREE_SYNTAX_ERR;
    }

    dtree->size++;
    return DIFF_TREE_ERR_NONE;
}

#undef DIFF_TREE_LOG_SYNTAX_ERR

DiffTreeErr diff_tree_fread(DiffTree* dtree, const char* filename)
{
    utils_assert(dtree);
    utils_assert(filename);

    FILE* file = open_file(filename, "r");
    file verified(return DIFF_TREE_IO_ERR);
    
    size_t fsize = get_file_size(file);

    dtree->buf.ptr = TYPED_CALLOC(fsize, char);
    dtree->buf.ptr verified(return DIFF_TREE_ALLOC_FAIL);

    size_t bytes_transferred = fread(dtree->buf.ptr, sizeof(dtree->buf.ptr[0]), fsize, file);
    fclose(file);
    // TODO check for errors
    dtree->buf.len = (unsigned) bytes_transferred;
    
    DiffTreeErr err = diff_tree_fread_node_(dtree, &dtree->root, filename);

    if(err != DIFF_TREE_ERR_NONE) {
        DIFF_TREE_DUMP(dtree, err);
        return err;
    }

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

DiffTreeNode* diff_tree_new_node(NodeType node_type, NodeValue node_value, DiffTreeNode *left, DiffTreeNode *right)
DiffTreeNode* diff_tree_new_node(NodeType node_type, NodeValue node_value, DiffTreeNode *left, DiffTreeNode *right, DiffTreeNode *parent)
{
    DiffTreeNode* node = TYPED_CALLOC(1, DiffTreeNode);

    UTILS_LOGD(LOG_CTG_DIFF_TREE, "%p", node);

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
    
    UTILS_LOGD(LOG_CTG_DIFF_TREE, "new: %p, node: %p", new_node, node);

    if(node->left)
        new_node->left = diff_tree_copy_subtree(dtree, node->left, new_node);

    if(node->right)
        new_node->right = diff_tree_copy_subtree(dtree, node->right, new_node);

    new_node->parent = parent;

    return new_node;
}

#define BUF_LEN 100
static char* diff_tree_node_value_str_(DiffTree* dtree, NodeType node_type, NodeValue val)
{
    static char buffer[BUF_LEN] = "";

    switch(node_type) {
        case NODE_TYPE_OP:
            return (char*)node_op_type_str(val.op_type);
        case NODE_TYPE_VAR:
            return diff_tree_find_variable(dtree, val.var_hash)->str;
        case NODE_TYPE_NUM:
            strfromd(buffer, BUF_LEN, "%f", val.num);
            return buffer;
        default:
            return "???";
    }
}
#undef BUF_LEN

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
        "\\begin{document}\n\n"
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

static void diff_tree_dump_node_latex_(DiffTree* dtree, DiffTreeNode* node)
{
    utils_assert(node);
    utils_assert(file_tex);

    if(node->left)
        diff_tree_dump_node_latex_(dtree, node->left);

    switch(node->type) {
        case NODE_TYPE_VAR:
            fprintf(file_tex, " %s ", diff_tree_find_variable(dtree, node->value.var_hash)->str);
            break;
        case NODE_TYPE_OP:
            fprintf(file_tex, " %s ", op_arr[(int)node->value.op_type].str);
            break;
        case NODE_TYPE_NUM:
            fprintf(file_tex, " %f ", node->value.num);
            break;
        default:
            UTILS_LOGW(LOG_CTG_DIFF_TREE, "unrecognized operator type");
            break;
    }

    if(node->right)
        diff_tree_dump_node_latex_(dtree, node->right);
}

void diff_tree_dump_latex(DiffTree* tree)
{
    DIFF_TREE_ASSERT_OK_(tree);
    utils_assert(file_tex);

    fprintf(file_tex, 
            "\\begin{equation}\n");

    diff_tree_dump_node_latex_(tree, tree->root);

    fprintf(file_tex,
            "\n\\end{equation}\n\n");

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

void diff_tree_dump(DiffTree* diff_tree, DiffTreeErr err, const char* msg, const char* filename, int line, const char* funcname)
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

    char* img_pref = diff_tree_dump_graphviz_(diff_tree);

    utils_log_fprintf(
        "\n<img src=" IMG_DIR "/%s.svg width=70%%\n", 
        strrchr(img_pref, '/') + 1
    );

    utils_log_fprintf("</pre>\n"); 

    utils_log_fprintf("<hr color=\"black\" />\n");

    NFREE(img_pref);
}

char* diff_tree_dump_graphviz_(DiffTree* diff_tree)
{
    utils_assert(diff_tree);

    FILE* file = open_file(LOG_DIR "/" GRAPHVIZ_FNAME_ ".txt", "w");
    // FIXME exit
    if(!file)
        exit(EXIT_FAILURE);

    fprintf(file, "digraph {\n rankdir=TB;\n"); 
    fprintf(file, "nodesep=0.9;\nranksep=0.75;\n");

    // fprintf(
    //     file, 
    //     "node [fontname=\"Fira Mono\","
    //     "color=" CLR_RED_BOLD_","
    //     "style=\"filled\","
    //     "shape=tripleoctagon,"
    //     "fillcolor=" CLR_RED_LIGHT_ ","
    //     "];\n"
    // );

    diff_tree_dump_node_graphviz_(diff_tree, file, diff_tree->root, 1);

    fprintf(file, "};");

    fclose(file);
    
    create_dir(LOG_DIR "/" IMG_DIR);
    char* img_tmpnam = tempnam(LOG_DIR "/" IMG_DIR, "img-");
    utils_assert(img_tmpnam);

    static char strbuf[GRAPHVIZ_CMD_LEN_]= "";

    // snprintf
    sprintf(
        strbuf, 
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
