#include "sym.h"
#include "err/err.h"
#include "parser/ast.h"
#include "parser/parser.h"
#include "symchk.h"
#include <stdio.h>
#include <stdlib.h>

void __walk_ast_for_symtab(ast_node_t* node,ast_node_t* scope){
    if(!node){
        return;
    }

    switch(node->type){
        //nodes we gotta do stuff for
        case AST_TYPE_DECL:
            printf("declared: %s\n",node->decl.name);
            __walk_ast_for_symtab(node->decl.starting_value,scope);
            break;

        case AST_TYPE_LABEL:
            printf("declared label: %s\n",node->label.name);
            break;

        case AST_TYPE_IDENT:
            printf("used: %s\n",node->ident.name);
            break;

        case AST_TYPE_FUNC_DECL:
            printf("declared func: %s\n",node->func_decl.name);
            for(size_t i = 0;i < node->func_decl.args->count;i++){
                __walk_ast_for_symtab(node->func_decl.args->nodes[i],scope);
            }
            __walk_ast_for_symtab(node->func_decl.body,scope);
            break;

        //scope stuff
        case AST_TYPE_BLOCK:
            for(size_t i = 0;i < node->block.stmts->count;i++){
                __walk_ast_for_symtab(node->block.stmts->nodes[i],node);
            }
            break;

        //nodes for traversal
        case AST_TYPE_PROGRAM:
            for(size_t i = 0;i < node->program.programisms->count;i++){
                __walk_ast_for_symtab(node->program.programisms->nodes[i],scope);
            }
            break;

        case AST_TYPE_IF:
            __walk_ast_for_symtab(node->if_stmt.cond,scope);
            __walk_ast_for_symtab(node->if_stmt.body,scope);
            break;

        case AST_TYPE_RETURN:
            if (node->return_stmt.val){
                __walk_ast_for_symtab(node->return_stmt.val,scope);
            }
            break;

        case AST_TYPE_FUNC_CALL:
            for(size_t i = 0;i < node->func_call.args->count;i++){
                __walk_ast_for_symtab(node->func_call.args->nodes[i],scope);
            }
            __walk_ast_for_symtab(node->func_call.fp,scope);
            break;

        case AST_TYPE_EVAL:
            __walk_ast_for_symtab(node->eval.expr,scope);
            break;

        case AST_TYPE_NOT:
        case AST_TYPE_UN_MINUS:
            __walk_ast_for_symtab(node->unary.expr,scope);
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
            __walk_ast_for_symtab(node->binary.left,scope);
            __walk_ast_for_symtab(node->binary.right,scope);
            break;

        case AST_TYPE_TERNARY_COND:
            __walk_ast_for_symtab(node->ternary.cond,scope);
            __walk_ast_for_symtab(node->ternary.val_false,scope);
            __walk_ast_for_symtab(node->ternary.val_true,scope);
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
}

void populate_symtab(src_file_t* file){
    file->symbols = malloc(sizeof(symbol_t) * 5);

    if(!file->symbols){
        throw_noncode_issue(*file, COMP_ERR_INTERNAL_FAILIURE, true);
    }

    file->num_symbols = 0;
    file->symbol_size = 5;

    __walk_ast_for_symtab(file->ast,file->ast);
}
