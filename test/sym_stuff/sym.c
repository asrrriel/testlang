#include "sym.h"
#include "err/err.h"
#include "parser/ast.h"
#include "parser/parser.h"
#include "symchk.h"
#include "util/util.h"
#include <stdio.h>
#include <stdlib.h>

void __walk_ast_for_symtab(UNUSED ast_node_t* node){
    printf("TODO: actually do symbol stuff\n");
}

void populate_symtab(src_file_t* file){
    file->symbols = malloc(sizeof(symbol_t) * 5);

    if(!file->symbols){
        throw_noncode_issue(*file, COMP_ERR_INTERNAL_FAILIURE, true);
    }

    file->num_symbols = 0;
    file->symbol_size = 5;

    __walk_ast_for_symtab(file->ast);
}
