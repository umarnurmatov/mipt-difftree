#include "variable.h"
#include "assertutils.h"

#ifdef _DEBUG

void variable_print_callback(FILE* stream, void* var)
{
    utils_assert(stream);
    utils_assert(var);

    Variable* var_ = (Variable*)var;
    fprintf(stream, 
            "[str: %s; hash: %lu; val: %f]", 
            var_->str,
            var_->hash,
            var_->val
           );
}

#endif // _DEBUG
