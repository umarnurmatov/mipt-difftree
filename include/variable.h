#pragma once

#include "utils.h"
#include "hashutils.h"

typedef struct Variable
{
    char* str;
    utils_hash_t hash;
    double val;

} Variable;

#ifdef _DEBUG

    #include <stdio.h>
    void variable_print_callback(FILE* stream, void* var);

#endif // _DEBUG
