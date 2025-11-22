#ifndef __SYMTAB_H__
#define __SYMTAB_H__

#include "parser/ast.h"
#include <stdint.h>

typedef struct {
    //basic information
    char* name;
    storage_type_t type;

    //refcount
    uint32_t refcount;
} symbol_t;

#endif // __SYMTAB_H__
