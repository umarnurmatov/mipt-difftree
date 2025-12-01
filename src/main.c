#include "difftree.h"
#include "difftree_math.h"
#include "optutils.h"
#include "utils.h"
#include "logutils.h"
#include "ioutils.h"
#include <cstdlib>

#define LOG_CATEGORY_OPT "OPTIONS"
#define LOG_CATEGORY_APP "APP"

static utils_long_opt_t long_opts[] = 
{
    { OPT_ARG_REQUIRED, "log", NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "in" , NULL, 0, 0 },
    { OPT_ARG_REQUIRED, "out" , NULL, 0, 0 },
};

int main(int argc, char* argv[])
{
    utils_long_opt_get(argc, argv, long_opts, SIZEOF(long_opts));

    if(!long_opts[0].is_set) {
        return EXIT_FAILURE;
    }

    if(!long_opts[1].is_set) {
        return EXIT_FAILURE;
    }

    if(!long_opts[2].is_set) {
        return EXIT_FAILURE;
    }

    utils_init_log_file(long_opts[0].arg, LOG_DIR);

    DiffTree dtree = DIFF_TREE_INIT_LIST;
    DiffTreeErr err = DIFF_TREE_ERR_NONE;

    err = diff_tree_ctor(&dtree, long_opts[2].arg);
    if(err != DIFF_TREE_ERR_NONE) {
        diff_tree_dtor(&dtree);
        return EXIT_FAILURE;
    }
    
    err = diff_tree_fread(&dtree, long_opts[1].arg); 
    if(err != DIFF_TREE_ERR_NONE) {
        diff_tree_dtor(&dtree);
        return EXIT_FAILURE;
    }

    DIFF_TREE_DUMP(&dtree, DIFF_TREE_ERR_NONE);

    // diff_tree_taylor_expansion(&dtree, (Variable*)vector_at(&dtree.vars, 0), 10, 2);
    
    diff_tree_dump_latex("\\section{Производная}\n");

    diff_tree_differentiate_tree_n(&dtree, (Variable*)vector_at(&dtree.vars,0), 1);

    DIFF_TREE_DUMP(&dtree, DIFF_TREE_ERR_NONE);

    for(size_t i = 0; i < dtree.vars.size; ++i) {
        Variable* var = (Variable*)vector_at(&dtree.vars, i);
        printf("Enter variable %c value: ", var->c);

        if(input_double_until_correct(&var->val) != IO_ERR_NONE)
            return EXIT_FAILURE;
    }

    diff_tree_dump_latex("\\section{Разложение в ряд Тейлора}\n");

    diff_tree_dump_latex("\\section{График}\n");

    UTILS_LOGI(LOG_CATEGORY_APP, "evaluated: %f\n", diff_tree_evaluate_tree(&dtree));

    diff_tree_dtor(&dtree);

    utils_end_log();

    return EXIT_SUCCESS;
}
