#include "difftree.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#include "colorutils.h"
#include "logutils.h"
#include "memutils.h"
#include "ioutils.h"
#include "sortutils.h"
#include "stringutils.h"
#include "assertutils.h"
#include "logutils.h"
#include "stack.h"

#define LOG_CTG_DIFF_TREE "FACT TREE"

#define DEFAULT_NODE_ "nothing"
#define CHAR_ACCEPT_ 'y'
#define CHAR_DECLINE_ 'n'
#define NIL_STR "nil"

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

static DiffTreeErr diff_tree_allocate_new_node_(DiffTreeNode** node, utils_str_t title);

static void diff_tree_swap_nodes_(DiffTreeNode* node_a, DiffTreeNode* node_b);

static DiffTreeErr diff_tree_fwrite_node_(DiffTreeNode* node, FILE* file);

static DiffTreeErr diff_tree_fread_node_(DiffTree* dtree, DiffTreeNode** node, const char* fname);

static DiffTreeErr diff_tree_scan_node_name_(DiffTree* dtree);

static int diff_tree_advance_buf_pos_(DiffTree* dtree);

static void diff_tree_skip_spaces_(DiffTree* dtree);

static char* diff_tree_dump_graphviz_(DiffTree* diff_tree);

static void diff_tree_dump_node_graphviz_(FILE* file, DiffTreeNode* node, int rank);

#ifdef _DEBUG

DiffTreeErr diff_tree_verify_(DiffTree* diff_tree);

void diff_tree_node_dtor_(DiffTree* tree, DiffTreeNode* node);

#endif // _DEBUG

DiffTreeErr diff_tree_ctor(DiffTree* diff_tree)
{
    utils_assert(diff_tree);

    DiffTreeErr err = DIFF_TREE_ERR_NONE;

    utils_str_t deflt_s = {
        .str = strdup(DEFAULT_NODE_),
        .len = SIZEOF(DEFAULT_NODE_) - 1
    };
    
    diff_tree_allocate_new_node_(&diff_tree->root, deflt_s);

    diff_tree->size = 1;

    DIFF_TREE_DUMP(diff_tree, err);

    return DIFF_TREE_ERR_NONE;
}

void diff_tree_dtor(DiffTree* diff_tree)
{
    utils_assert(diff_tree);

    diff_tree_node_dtor_(diff_tree, diff_tree->root); 

    diff_tree->size = 0;
    diff_tree->root = NULL;

    NFREE(diff_tree->buf.ptr);
    diff_tree->buf.pos = 0;
    diff_tree->buf.len = 0;
}

void diff_tree_node_dtor_(DiffTree* dtree, DiffTreeNode* node)
{
    if(!node) return;

    if(node->left)
        diff_tree_node_dtor_(dtree, node->left);
    if(node->right)
        diff_tree_node_dtor_(dtree, node->right);

    if(dtree->buf.ptr) {
        if(node->name.str >= dtree->buf.ptr + dtree->buf.len 
                || node->name.str < dtree->buf.ptr)
            NFREE(node->name.str);
    }
    else
        NFREE(node->name.str);

    NFREE(node);
}

DiffTreeErr diff_tree_insert(DiffTree* diff_tree, DiffTreeNode* node, DiffTreeNode** ret)
{
    DIFF_TREE_ASSERT_OK_(diff_tree);
    utils_assert(ret);
    utils_assert(node);

    DiffTreeErr err = DIFF_TREE_ERR_NONE;

    utils_str_t diff_s = UTILS_STR_INITLIST;
        
    ++diff_tree->size;

    return DIFF_TREE_ERR_NONE;
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

    io_err = fprintf(file, " \"%s\" ", node->name.str);
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
    sscanf(dtree->buf.ptr + dtree->buf.pos, " \"%*[^\"]\"%n", &name_len);

    dtree->buf.pos += (size_t) name_len;

    dtree->buf.ptr[dtree->buf.pos - 1] = '\0';

    return DIFF_TREE_ERR_NONE;
}

#define DIFF_TREE_LOG_SYNTAX_ERR(fname, dtree, expc)                            \
    UTILS_LOGE(                                                                 \
        LOG_CTG_DIFF_TREE,                                                      \
        "%s:1:%ld: syntax error: unexpected symbol <%c>, expected <" expc ">",  \
        fname,                                                                  \
        dtree->buf.pos,                                                         \
        dtree->buf.ptr[dtree->buf.pos]                                          \
    );                                                                          

DiffTreeErr diff_tree_fread_node_(DiffTree* dtree, DiffTreeNode** node, const char* fname)
{
    DIFF_TREE_ASSERT_OK_(dtree);

    DiffTreeErr err = DIFF_TREE_ERR_NONE;

    DIFF_TREE_DUMP(dtree, err);

    if(dtree->buf.ptr[dtree->buf.pos] == '(') {
        utils_str_t name = { .str = NULL, .len = 0 };
        err = diff_tree_allocate_new_node_(node, name);
        err == DIFF_TREE_ERR_NONE verified(return err);

        diff_tree_advance_buf_pos_(dtree);
        diff_tree_skip_spaces_(dtree);

        if(dtree->buf.ptr[dtree->buf.pos] != '"') {
            DIFF_TREE_LOG_SYNTAX_ERR(fname, dtree, "\"");
            return DIFF_TREE_SYNTAX_ERR;
        }

        ssize_t buf_pos_prev = dtree->buf.pos;
        err = diff_tree_scan_node_name_(dtree);
        err == DIFF_TREE_ERR_NONE verified(return err);

        (*node)->name.str = dtree->buf.ptr + buf_pos_prev + 1;
        (*node)->name.len = (size_t)(dtree->buf.pos - buf_pos_prev);

        diff_tree_skip_spaces_(dtree);

        err = diff_tree_fread_node_(dtree, &(*node)->left, fname);
        err == DIFF_TREE_ERR_NONE verified(return err);

        if((*node)->left) {
            UTILS_LOGD(LOG_CTG_DIFF_TREE, "%s -> %s", (*node)->left->name.str, (*node)->name.str);
            (*node)->left->parent = (*node);
        }

        diff_tree_skip_spaces_(dtree);

        err = diff_tree_fread_node_(dtree, &(*node)->right, fname);
        err == DIFF_TREE_ERR_NONE verified(return err);

        if((*node)->right) {
            UTILS_LOGD(LOG_CTG_DIFF_TREE, "%s -> %s", (*node)->right->name.str, (*node)->name.str);
            (*node)->right->parent = (*node);
        }

        dtree->size++;

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

        dtree->buf.pos += SIZEOF(NIL_STR) - 1;
        diff_tree_skip_spaces_(dtree);
        *node = NULL;
    }
    else {
        DIFF_TREE_LOG_SYNTAX_ERR(fname, dtree, "(");
        return DIFF_TREE_SYNTAX_ERR;
    }

    return DIFF_TREE_ERR_NONE;
}

#undef DIFF_TREE_LOG_SYNTAX_ERR

DiffTreeErr diff_tree_fread(DiffTree* dtree, const char* filename)
{
    utils_assert(dtree);
    utils_assert(filename);

    diff_tree_dtor(dtree);

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

DiffTreeErr diff_tree_allocate_new_node_(DiffTreeNode** node, utils_str_t name)
{
    utils_assert(node);

    DiffTreeNode* node_tmp = (DiffTreeNode*)calloc(1, sizeof(node_tmp[0]));
    
    node_tmp verified(return DIFF_TREE_ALLOC_FAIL);

    node_tmp->name = name;

    *node = node_tmp;

    return DIFF_TREE_ERR_NONE;
}

void diff_tree_swap_nodes_(DiffTreeNode* node_a, DiffTreeNode* node_b)
{
    // FIXME fix swap
    utils_swap(&node_a->name, &node_b->name, sizeof(node_a->name));
}

#ifdef _DEBUG

#define GRAPHVIZ_FNAME_ "graphviz"
#define GRAPHVIZ_CMD_LEN_ 100

#define CLR_RED_LIGHT_   "\"#FFB0B0\""
#define CLR_GREEN_LIGHT_ "\"#B0FFB0\""
#define CLR_BLUE_LIGHT_  "\"#B0B0FF\""

#define CLR_RED_BOLD_    "\"#FF0000\""
#define CLR_GREEN_BOLD_  "\"#03c03c\""
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
            for(ssize_t i = 0; i < diff_tree->buf.len; ++i) {
                utils_log_fprintf("%c", diff_tree->buf.ptr[i]);
            }    
            utils_log_fprintf("\n");
            GOTO_END;
        }

        utils_log_fprintf("<font color=\"#09AB00\">"); 
        for(ssize_t i = 0; i < diff_tree->buf.pos; ++i) {
            utils_log_fprintf("%c", diff_tree->buf.ptr[i]);
        }
        utils_log_fprintf("</font>");


        utils_log_fprintf("<font color=\"#C71022\"><b>%c</b></font>", diff_tree->buf.ptr[diff_tree->buf.pos]);


        utils_log_fprintf("<font color=\"#1022C7\">"); 
        for(ssize_t i = diff_tree->buf.pos + 1; i < diff_tree->buf.len; ++i) {
            utils_log_fprintf("%c", diff_tree->buf.ptr[i]);
        }
        utils_log_fprintf("</font>\n");

    } END;

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

    diff_tree_dump_node_graphviz_(file, diff_tree->root, 1);

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

void diff_tree_dump_node_graphviz_(FILE* file, DiffTreeNode* node, int rank)
{
    utils_assert(file);

    if(!node) return;

    if(node->left)
        diff_tree_dump_node_graphviz_(file, node->left, rank + 1); 
    if(node->right) 
        diff_tree_dump_node_graphviz_(file, node->right, rank + 1);

    if(!node->left && !node->right)
        fprintf(
            file, 
            "node_%p["
            "shape=record,"
            "label=\" { parent: %p | addr: %p | name: \' %s \' | { L: %p | R: %p } } \","
            "style=\"filled\","
            "color=" CLR_GREEN_BOLD_ ","
            "fillcolor=" CLR_GREEN_LIGHT_ ","
            "rank=%d"
            "];\n",
            node,
            node->parent,
            node,
            node->name.str,
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
                "<td colspan=\"2\">name: %s</td>"
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
            node->name.str,
            node->left,
            node->right,
            rank
        );

    if(node->left)
        fprintf(
            file,
            "node_%p -> node_%p ["
            "dir=both," 
            "label=\"No\","
            "color=" CLR_RED_BOLD_ ","
            "fontcolor=" CLR_RED_BOLD_ ","
            "];\n",
            node, 
            node->left
        );

    if(node->right)
        fprintf(
            file,
            "node_%p -> node_%p ["
            "dir=both," 
            "label=\"Yes\","
            "color=" CLR_BLUE_BOLD_ ","
            "fontcolor=" CLR_BLUE_BOLD_ ","
            "];\n",
            node, 
            node->right
        );
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
