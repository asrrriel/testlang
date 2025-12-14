#include "sym.h"
#include "err/err.h"
#include "parser/ast.h"
#include "parser/parser.h"
#include "symchk.h"
#include "util/srcfile.h"
#include <stdio.h>
#include <stdlib.h>

bool append_sym(symbol_t** t,size_t* filled, size_t* size,symbol_t to_append){
    if((*filled)++ >= *size){
        if (!(*t = realloc(*t, (*size += *size / 2) * sizeof(symbol_t)))){
            return false;
        }
    }
    (*t)[*filled - 1] = to_append;
    return true;
}

bool is_type_equal(storage_type_t a, storage_type_t b){
    return a.valid 
        && b.valid 
        && a.base == b.base 
        && a.ptr_depth == b.ptr_depth 
        && a.qualifiers == b.qualifiers;
}

storage_type_t __walk_ast(src_file_t* file, symbol_t** syms,size_t* filled, size_t* size,ast_node_t* node,ast_node_t* scope){
    if(!node){
        return (storage_type_t){
            false,0,0,0
        };
    }

    storage_type_t toret = (storage_type_t){
        false,0,0,0
    };

    switch(node->type){
        //nodes we gotta do stuff for
        case AST_TYPE_DECL:
            printf("declared: %s\n",node->decl.name);
            append_sym(syms,filled,size, (symbol_t){
                .name = node->decl.name,
                .type = ST_VAR,
                .storage_type = node->decl.type,
                .scope = scope,
                .refcount = 0,
            });
            toret = node->decl.type;
            storage_type_t type = __walk_ast(file,syms,filled,size,node->decl.starting_value,scope);

            if (!is_type_equal(toret, type)) {
                throw_noncode_issue(*file, COMP_ERR_TYPE_MISMATCH, false);
            }

            break;

        case AST_TYPE_LABEL:
            printf("declared label: %s\n",node->label.name);
            append_sym(syms,filled,size, (symbol_t){
                .name = node->label.name,
                .type = ST_LABEL,
                .storage_type.valid = false,
                .storage_type.base = 0,
                .storage_type.ptr_depth = 0,
                .storage_type.qualifiers = 0,
                .scope = scope,
                .refcount = 0,
            });
            break;

        case AST_TYPE_IDENT:
            printf("used: %s\n",node->ident.name);
            break;

        case AST_TYPE_FUNC_DECL:
            printf("declared func: %s\n",node->func_decl.name);
            append_sym(syms,filled,size, (symbol_t){
                .name = node->func_decl.name,
                .type = ST_FUNC,
                .storage_type = node->func_decl.return_type,
                .scope = scope,
                .refcount = 0,
            });
            for(size_t i = 0;i < node->func_decl.args->count;i++){
                __walk_ast(file,syms,filled,size,node->func_decl.args->nodes[i],scope);
            }
            __walk_ast(file,syms,filled,size,node->func_decl.body,scope);
            break;

        //scope stuff
        case AST_TYPE_BLOCK:
            for(size_t i = 0;i < node->block.stmts->count;i++){
                __walk_ast(file,syms,filled,size,node->block.stmts->nodes[i],node);
            }
            break;

        //nodes for traversal
        case AST_TYPE_PROGRAM:
            for(size_t i = 0;i < node->program.programisms->count;i++){
                __walk_ast(file,syms,filled,size,node->program.programisms->nodes[i],scope);
            }
            break;

        case AST_TYPE_IF:
            __walk_ast(file,syms,filled,size,node->if_stmt.cond,scope);
            __walk_ast(file,syms,filled,size,node->if_stmt.body,scope);
            break;

        case AST_TYPE_RETURN:
            if (node->return_stmt.val){
                __walk_ast(file,syms,filled,size,node->return_stmt.val,scope);
            }
            break;

        case AST_TYPE_FUNC_CALL:
            for(size_t i = 0;i < node->func_call.args->count;i++){
                __walk_ast(file,syms,filled,size,node->func_call.args->nodes[i],scope);
            }
            __walk_ast(file,syms,filled,size,node->func_call.fp,scope);
            break;

        case AST_TYPE_EVAL:
            __walk_ast(file,syms,filled,size,node->eval.expr,scope);
            break;

        case AST_TYPE_NOT:
        case AST_TYPE_UN_MINUS:
            __walk_ast(file,syms,filled,size,node->unary.expr,scope);
            break;

        case AST_TYPE_ADD:  //fallthru
        case AST_TYPE_SUB:  //fallthru
        case AST_TYPE_MUL:  //fallthru
        case AST_TYPE_DIV:  //fallthru
        case AST_TYPE_MOD:  //fallthru
        case AST_TYPE_AND:  //fallthru
        case AST_TYPE_OR:   //fallthru
        case AST_TYPE_XOR:  //fallthru
        case AST_TYPE_LAND: //fallthru
        case AST_TYPE_LOR:  //fallthru
        case AST_TYPE_EQ:   //fallthru
        case AST_TYPE_NE:   //fallthru
        case AST_TYPE_GT:   //fallthru
        case AST_TYPE_LT:   //fallthru
        case AST_TYPE_GTE:  //fallthru
        case AST_TYPE_LTE:  //fallthru
        case AST_TYPE_ASS:  //fallthru
            __walk_ast(file,syms,filled,size,node->binary.left,scope);
            __walk_ast(file,syms,filled,size,node->binary.right,scope);
            break;

        case AST_TYPE_TERNARY_COND:
            __walk_ast(file,syms,filled,size,node->ternary.cond,scope);
            __walk_ast(file,syms,filled,size,node->ternary.val_false,scope);
            __walk_ast(file,syms,filled,size,node->ternary.val_true,scope);
            break;

        //the rest
        case AST_TYPE_CHRLIT:
        case AST_TYPE_INTLIT:
        case AST_TYPE_DOTDOTDOT:
        case AST_TYPE_EMPTY_STMT:
        case AST_TYPE_GOTO:
        case AST_TYPE_STRLIT:
            break;
    }

    return toret;
}

void populate_symtab(src_file_t* file){
    file->symbols = malloc(sizeof(symbol_t) * 5);

    if(!file->symbols){
        throw_noncode_issue(*file, COMP_ERR_INTERNAL_FAILIURE, true);
    }

    file->num_symbols = 0;
    file->symbol_size = 5;

    __walk_ast(file,&file->symbols,&file->num_symbols,&file->symbol_size,file->ast,file->ast);
}
