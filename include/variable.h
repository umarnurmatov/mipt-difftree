#pragma once

#include "hashutils.h"

typedef struct Variable
{
    char* str;
    utils_hash_t hash;
    double val;
} 
Variable;
