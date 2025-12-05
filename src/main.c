#include <cstdlib>
#include <stdlib.h>

#include "difftree.h"
#include "difftree_math.h"
#include "optutils.h"
#include "utils.h"
#include "logutils.h"

#define LOG_CATEGORY_OPT "OPTIONS"
#define LOG_CATEGORY_APP "APP"

static utils_long_opt_t long_opts[] = 
{
    { OPT_ARG_REQUIRED, "log",    NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "in" ,    NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "out" ,   NULL, 0, 0 },
    { OPT_ARG_OPTIONAL, "power",  NULL, 0, 0 },
    { OPT_ARG_OPTIONAL, "x0",     NULL, 0, 0 },
    { OPT_ARG_OPTIONAL, "ymin",   NULL, 0, 0 },
    { OPT_ARG_OPTIONAL, "ymax",   NULL, 0, 0 },
};

static const size_t POWER_DEFAULT = 4;
static const double X0_DEFAULT    = 10.f;
static const double YMIN_DEFAULT  = -3.f;
static const double YMAX_DEFAULT  = 3.f;
static const double DELTA         = 2.f;
static const double STEP          = 0.005f;

int main(int argc, char* argv[])
{
    size_t power = POWER_DEFAULT;
    double x0    = X0_DEFAULT;
    double ymin  = YMIN_DEFAULT;
    double ymax  = YMAX_DEFAULT;

    if(!utils_long_opt_get(argc, argv, long_opts, SIZEOF(long_opts)))
        return EXIT_FAILURE;

    if(long_opts[3].is_set) power = (size_t) atol(long_opts[3].arg);

    if(long_opts[4].is_set) x0   = atof(long_opts[4].arg);

    if(long_opts[5].is_set) ymin = atof(long_opts[5].arg);

    if(long_opts[6].is_set) ymax = atof(long_opts[6].arg);

    utils_init_log_file(long_opts[0].arg, LOG_DIR);

    DiffTree dtree = DIFF_TREE_INIT_LIST;
    DiffTreeErr err = DIFF_TREE_ERR_NONE;

    err = diff_tree_ctor(&dtree);
    if(err != DIFF_TREE_ERR_NONE) {
        diff_tree_dtor(&dtree);
        return EXIT_FAILURE;
    }

    diff_tree_init_latex_file(long_opts[2].arg);
    
    err = diff_tree_fread(&dtree, long_opts[1].arg); 
    if(err != DIFF_TREE_ERR_NONE) {
        diff_tree_dtor(&dtree);
        return EXIT_FAILURE;
    }

    DiffTree dtree_taylor = DIFF_TREE_INIT_LIST;
    diff_tree_copy_tree(&dtree, &dtree_taylor);

    DiffTree dtree_copy = DIFF_TREE_INIT_LIST;
    diff_tree_copy_tree(&dtree, &dtree_copy);

    DIFF_TREE_DUMP(&dtree, DIFF_TREE_ERR_NONE);

    DIFF_TREE_DUMP(&dtree_taylor, DIFF_TREE_ERR_NONE);

    diff_tree_dump_latex("\\section{Производная}\n");

    diff_tree_differentiate_tree_n(&dtree, (Variable*)vector_at(&dtree.vars, 0), 1);

    DIFF_TREE_DUMP(&dtree, DIFF_TREE_ERR_NONE);
    
    // for(size_t i = 0; i < dtree.vars.size; ++i) {
    //     Variable* var = (Variable*)vector_at(&dtree.vars, i);
    //     printf("Enter variable %c value: ", var->c);
    //
    //     if(input_double_until_correct(&var->val) != IO_ERR_NONE)
    //         return EXIT_FAILURE;
    // }

    diff_tree_dump_latex(
        "\\section{Разложение в ряд Тейлора}\n"
        "Разложим данную функцию в ряд Тейлора до $o((x-x_0)^%lu)$ в точке $x_0 = %g$\n",
        power, x0);

    DiffTreeNode* polynom = diff_tree_taylor_expansion(&dtree_taylor, (Variable*)vector_at(&dtree.vars, 0), x0, power);

    diff_tree_dump_latex("\\section{График в окрестности $x_0$}\n");

    double x_begin = x0 - DELTA;
    double x_end   = x0 + DELTA;

    diff_tree_dump_graph_latex(&dtree, x_begin, x_end, STEP);

    diff_tree_dump_taylor_graph_latex(&dtree_copy, polynom, x0 - 1.f, x0 + 1.f, STEP, ymin, ymax);

    diff_tree_mark_to_delete(&dtree, polynom);

    diff_tree_end_latex_file();

    diff_tree_dtor(&dtree);

    diff_tree_dtor(&dtree_copy);

    diff_tree_dtor(&dtree_taylor);

    utils_end_log();

    return EXIT_SUCCESS;
}
