#include "operators.h"

#include <string.h>

#include "assertutils.h"
#include "logutils.h"

int operators_prepare_op_arr()
{                                                                                       
    // size_t tbl_size_tmp = SIZEOF(op_arr);                                                  
    //
    // for(ssize_t ind_i = 0; (unsigned) ind_i < SIZEOF(op_arr); ++ind_i) {                      
    //     if(ind_i < 1) continue;                                                          
    //
    //     ssize_t ind_j = ind_i - 1;                                                        
    //     Operator key = op_arr[ind_i];                                   
    //     while(key.hash < op_arr[ind_i].hash) {                                          
    //         op_arr[cmdj + 1] = tbl_tmp[cmdj];                                          
    //
    //         --ind_j;                                                                     
    //         if(ind_j < 0) break;                                                         
    //     }                                                                               
    //
    //     if(cmdj >= 0 && tbl_tmp[cmdj].hash == key.hash) {                               
    //         NFREE(tbl_tmp);                                                             
    //         return ASSEMBLER_ERR_HASH_COLLISION;                                        
    //     }                                                                               
    //     tbl_tmp[cmdj + 1] = key;                                                        
    // }                                                                                   
    //
    // asmblr->pref##_tbl      = tbl_tmp;                                                  
    // asmblr->pref##_tbl_size = tbl_size_tmp;                                             
    //
    // return ASSEMBLER_ERR_NONE;                                                          
}

const Operator* get_operator(OperatorType op_type)
{
    utils_assert((unsigned)op_type < SIZEOF(op_arr));

    return &op_arr[(unsigned)op_type];
}


const Operator* match_function(char* str)
{
    utils_assert(str);

    // FIXME make hash comparasion
    for(size_t i = 0; i < SIZEOF(op_arr); ++i) {
        if(!strcmp(str, op_arr[i].str) 
            && op_arr[i].argnum == OPERATOR_ARGNUM_1) {

            return op_arr + i;
        }
    }

    return NULL;
}
