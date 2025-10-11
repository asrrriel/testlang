#include "parser.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "peeker.h"
#include "util/srcfile.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <err/err.h>
#include <util/util.h>
#include <stdio.h>

static ast_node_list_t* create_ast_node_list(){
    ast_node_list_t* toret = malloc(sizeof(ast_node_list_t)); 

    toret->count = 0;
    toret->size = 5;
    toret->nodes = malloc(toret->size* sizeof(ast_node_t*));

    return toret;
}

static bool append_node(ast_node_list_t* l,ast_node_t* to_append){
    if(l->count++ >= l->size){
        if (!(l->nodes = realloc(l->nodes, (l->size += l->size / 2) * sizeof(ast_node_t*)))){
            return false;
        }
    }
    l->nodes[l->count - 1] = to_append;
    return true;
}

static const char* qualifiers[4] = {
    "const",
    "long",
    "unsigned",
    "extern"
};

static bool is_qualifier(token_t* token){
    if(!token || token->type != TOKEN_TYPE_STRING || !token->value){
        return false;
    }

    for(uint8_t i = 0; i < 4; i++){
        if(strcmp((char*)token->value,qualifiers[i]) == 0){
            return true;
        }
    }

    return false;
}

static const char* basetypes[4] = {
    "char",
    "short",
    "int",
    "void"
};

static bool is_basetype(token_t* token){
    if(!token || token->type != TOKEN_TYPE_STRING || !token->value){
        return false;
    }

    for(uint8_t i = 0; i < 4; i++){
        if(strcmp((char*)token->value,basetypes[i]) == 0){
            return true;
        }
    }

    return false;
}

static const char* stmt_kws[2] = {
    "if",
    "goto",
};

static bool is_stmt_kw(token_t* token){
    if(!token || token->type != TOKEN_TYPE_STRING || !token->value){
        return false;
    }

    for(uint8_t i = 0; i < 2; i++){
        if(strcmp((char*)token->value,stmt_kws[i]) == 0){
            return true;
        }
    }

    return false;
}

ast_node_t* parse_expr(UNUSED src_file_t* file){
    return NULL;
}

storage_type_t parse_type_expr(src_file_t* file){
    storage_type_t toret = {
        .valid = true
    };
    token_t* next;

    if(is_qualifier(peek(0))){
        while(is_qualifier(next = peek(0))){
            uint8_t q = 0;
            for(uint8_t i = 0; i < 4; i++){
                if(strcmp((char*)next->value,qualifiers[i]) == 0){
                    q = 1 << i;
                    break;
                }
            }

            if(toret.qualifiers & q){
                throw_code_issue(*file, COMP_ERR_DUPLICATE_QUALIFIER,
                                 *next, true); //TODO: non-fatal errors
            }

            toret.qualifiers |= q;
            consume();
        }
    }

    if(!is_basetype(next = consume())){
        return (storage_type_t){0}; // not a declaration
    }

    for(uint8_t i = 0; i < 4; i++){
        if(strcmp((char*)next->value,basetypes[i]) == 0){
            toret.base = (base_type_t)i;
            break;
        }
    }

    while((next = peek(0))->type == TOKEN_TYPE_STAR){
        toret.ptr_depth++;
        consume();
    }

    return toret;
}

ast_node_t* parse_decl(src_file_t* file){
    storage_type_t decl_type = parse_type_expr(file);

    token_t* next = expect_d(TOKEN_TYPE_STRING);

    //failiure cases: invalid storage type, no name token(or its not a string),
    // name token is a keyword
    if(!decl_type.valid || !next 
        || is_basetype(next) || is_qualifier(next) || is_stmt_kw(next))
        return NULL;

    ast_node_t* node = malloc(sizeof(ast_node_t));

    node->type = AST_TYPE_DECL;
    node->decl.type = decl_type;
    node->decl.name = (char*)next->value;

    next = consume();

    if(next->type == TOKEN_TYPE_EQUAL){
        node->decl.starting_value = parse_expr(file);
    } else if (next->type == TOKEN_TYPE_SEMI) {
        node->decl.starting_value = NULL;
    } else {
        throw_code_issue(*file, COMP_ERR_MISSING_SEMICOLON,
                                 *next, true); //TODO: non-fatal errors
    }

    return node;
}

ast_node_t* parse_func_decl(UNUSED src_file_t* file){
    return NULL;
}

ast_node_t* parse_programism(src_file_t* file){
    ast_node_t* node = NULL;
    if((node = parse_decl(file))){
        return node;
    }  else {
        return parse_func_decl(file);
    }
}

ast_node_t* parse_program(src_file_t* file){
    ast_node_t* toret = malloc(sizeof(ast_node_t));

    toret->type = AST_TYPE_PROGRAM;
    toret->program.programisms = create_ast_node_list();

    while(1) {
        ast_node_t* node = parse_programism(file);

        if(!node){
            break;
        }

        append_node(toret->program.programisms, node);
    };

    return toret;
}

void parse(src_file_t* file){
    set_token_source(file->tokens);

    file->ast = parse_program(file);
}

void print_storage_type(storage_type_t type,size_t indent){
    char* indent_str = malloc(indent+1);
    memset(indent_str,' ',indent);
    indent_str[indent] = 0;

    if(!type.valid){
        printf("%sINVALID TYPE\n",indent_str);
        free(indent_str);
        return;
    }

    printf("%s  |->Qualifiers:%s%s%s%s%s\n", indent_str,
        (type.qualifiers & QUAL_CONST)     ?  " const"    : "",
        (type.qualifiers & QUAL_LONG)      ?  " long"     : "",
        (type.qualifiers & QUAL_UNSIGNED)  ?  " unsigned" : "",
        (type.qualifiers & QUAL_EXTERN)    ?  " extern"   : "",
        type.qualifiers == 0                ? " none"     : ""
    );

    printf("%s  |->Base type: %s\n",indent_str,basetypes[type.base]);
    printf("%s  \\->Pointer depth: %zu\n",indent_str,type.ptr_depth);

    free(indent_str);
}

#define INDENT_WIDTH 4

void print_ast_node(ast_node_t* node,size_t indent){
    char* indent_str = malloc(indent+1);
    memset(indent_str,' ',indent);
    indent_str[indent] = 0;
    if(!node){
        printf("%sNull node\n",indent_str);
    }

    switch(node->type){
        case AST_TYPE_PROGRAM:
            printf("%sProgram node:\n",indent_str);
            free(indent_str); // no need for it after this
            for(size_t i = 0;i < node->program.programisms->count;i++){
                print_ast_node(node->program.programisms->nodes[i], indent + INDENT_WIDTH);
            }
            break;

        case AST_TYPE_DECL:
            printf("%sDeclaration node:\n",indent_str);
            printf("%s  |->Storage type:\n",indent_str);
            print_storage_type(node->decl.type, indent + INDENT_WIDTH);
            if(node->decl.starting_value){
                printf("%s  |->Name: \"%s\"\n",indent_str,node->decl.name);
                printf("%s  |->Storage type:\n",indent_str);
            } else {
                printf("%s  \\->Name: \"%s\"\n",indent_str,node->decl.name);
            }
            free(indent_str);
            if(node->decl.starting_value){
                print_ast_node(node->decl.starting_value, indent + INDENT_WIDTH);
            }
            break;

        default:
            printf("%sUnknown node\n",indent_str);
            free(indent_str);
    }
}
