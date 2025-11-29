#ifndef __SYMTAB_H__
#define __SYMTAB_H__

#include "parser/ast.h"
#include <stdint.h>

typedef struct {
    //basic information
    char* name;
    enum {
        ST_VAR,
        ST_FUNC,
        ST_LABEL
    } type;
    storage_type_t storage_type;

    //context stuff
    ast_node_t* scope;
    uint32_t refcount;
} symbol_t;

#endif // __SYMTAB_H__
