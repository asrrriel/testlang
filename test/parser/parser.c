#include "parser.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "peeker.h"
#include "util/srcfile.h"
#include <ctype.h>
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
            consume(1);
        }
    }

    if(!is_basetype(next = consume(1))){
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
        consume(1);
    }

    return toret;
}

typedef struct {
    storage_type_t type;
    char* name;
} decl_stem_t;

decl_stem_t parse_decl_stem(src_file_t* file){
    decl_stem_t toret = {0};

    storage_type_t decl_type = parse_type_expr(file);

    token_t* next = expect_d(TOKEN_TYPE_STRING);

    //failiure cases: invalid storage type, no name token(or its not a string),
    // name token is a keyword
    if(!decl_type.valid || !next 
        || is_basetype(next) || is_qualifier(next) || is_stmt_kw(next)){

        return (decl_stem_t){0};
    }

    toret.type = decl_type;
    toret.name = (char*)next->value;

    return toret;
}

//TODO: expression parsing
ast_node_t* parse_expr_until(UNUSED src_file_t* file,UNUSED token_type_t until){
    ast_node_t* node = malloc(sizeof(ast_node_t));
    token_t* next;
    //primary expr
    if((next = expect(TOKEN_TYPE_CHRLIT))){
        node->type = AST_TYPE_CHRLIT;
        if(!next->value){
             throw_code_issue(*file, COMP_ERR_EMPTY_CHRLIT,
                                 *next, true); //TODO: non-fatal errors
        }
        if(strlen((char*)next->value) > 1){
            throw_code_issue(*file, COMP_ERR_MULTIBYTE_CHRLIT,
                                 *next, true); //TODO: non-fatal errors
        }
        node->chrlit.value = ((char*)next->value)[0];
        consume(1);
    } else if((next = expect(TOKEN_TYPE_STRING)) && isdigit(((char*)next->value)[0])){ 
        node->type = AST_TYPE_INTLIT;
        node->intlit.value = (char*)next->value;
        consume(1);
    } else if((next = expect(TOKEN_TYPE_STRLIT))){ 
        node->type = AST_TYPE_STRLIT;
        node->intlit.value = (char*)next->value;
        consume(1);
    } else {
        consume(1);
        free(node);
        return NULL;
    }

    return node;
}

//TODO: statement parsing
ast_node_list_t* parse_stmts_until(UNUSED src_file_t* file,UNUSED token_type_t until){
    consume(1);
    return NULL;
}

ast_node_t* parse_programism(src_file_t* file){
    ast_node_t* node = malloc(sizeof(ast_node_t));
    decl_stem_t stem = parse_decl_stem(file);

    token_t* next = peek(0);

    if(!next || next->type == TOKEN_TYPE_TERMINATOR) {
        throw_code_issue(*file, COMP_ERR_DECLARATION_CUT_OFF,
                                 *next, true); //TODO: non-fatal errors
    }

    if(next->type == TOKEN_TYPE_SEMI){
        node->type = AST_TYPE_DECL;
        node->decl.type = stem.type;
        node->decl.name = stem.name;
        node->decl.starting_value = NULL;
        consume(1);
    } else if(next->type == TOKEN_TYPE_EQUAL){
        node->type = AST_TYPE_DECL;
        node->decl.type = stem.type;
        node->decl.name = stem.name;
        consume(1);
        node->decl.starting_value = parse_expr_until(file,TOKEN_TYPE_SEMI);
        if(!expect_d(TOKEN_TYPE_SEMI)){
            throw_code_issue(*file, COMP_ERR_MISSING_SEMICOLON,
                                             *next, true); //TODO: non-fatal errors
        }
    } else if(next->type == TOKEN_TYPE_LPAREN){
        node->type = AST_TYPE_FUNC_DECL;
        node->func_decl.return_type = stem.type;
        node->func_decl.name = stem.name;
        node->func_decl.args = create_ast_node_list();

        while((next = peek(0))->type != TOKEN_TYPE_TERMINATOR){
            consume(1); // lparen or colon

            if(expect(TOKEN_TYPE_DOT)){
                ast_node_t* param = malloc(sizeof(ast_node_t));
                param->type = AST_TYPE_DOTDOTDOT;
                append_node(node->func_decl.args, param);
                consume(3); // dot x3

            } else {
                decl_stem_t stem = parse_decl_stem(file);

                if(!stem.name){
                    throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE,
                                                 *peek(0), true); //TODO: non-fatal errors
                }

                ast_node_t* param = malloc(sizeof(ast_node_t));
                param->type = AST_TYPE_DECL;
                param->decl.type = stem.type;
                param->decl.name = stem.name;
                param->decl.starting_value = NULL;

                append_node(node->func_decl.args, param);
            }
            if(expect(TOKEN_TYPE_RPAREN)){
                break;
            }
        }
        consume(1); //rparen

        if(expect(TOKEN_TYPE_SEMI)){
            consume(1);
        } else {
            throw_code_issue(*file, COMP_ERR_UNIMPLEMENTED, // no expression parsing (yet)
                                             *peek(0), false); //TODO: non-fatal errors
            free_ast_node(node);
            return NULL;
        }
    } else{
        throw_code_issue(*file, COMP_ERR_MISSING_SEMICOLON,
                                 *next, true); //TODO: non-fatal errors
    }

    return node;
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
        type.qualifiers == 0               ?  " none"     : ""
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
                printf("%s  |->Starting value:\n",indent_str);
            } else {
                printf("%s  \\->Name: \"%s\"\n",indent_str,node->decl.name);
            }
            free(indent_str);
            if(node->decl.starting_value){
                print_ast_node(node->decl.starting_value, indent + INDENT_WIDTH*2);
            }
            break;

        case AST_TYPE_FUNC_DECL:
            printf("%sFunction declaration node:\n",indent_str);
            printf("%s  |->Return type(+qualifiers):\n",indent_str);
            print_storage_type(node->decl.type, indent + INDENT_WIDTH);
            printf("%s  |->Name: \"%s\"\n",indent_str,node->func_decl.name);
            printf("%s  |->Arguments: \n",indent_str);
            free(indent_str); // no need for it after this
            for(size_t i = 0;i < node->func_decl.args->count;i++){
                print_ast_node(node->func_decl.args->nodes[i], indent + INDENT_WIDTH);
            }
            break;
        case AST_TYPE_CHRLIT:
            printf("%sCharacter literal '%c'\n",indent_str,node->chrlit.value);
            free(indent_str);
            break;

        case AST_TYPE_INTLIT:
            printf("%sInteger literal %s\n",indent_str,node->intlit.value);
            free(indent_str);
            break;

        case AST_TYPE_STRLIT:
            printf("%sString literal \"%s\"\n",indent_str,node->intlit.value);
            free(indent_str);
            break;

        case AST_TYPE_DOTDOTDOT:
            printf("%sElipses node\n",indent_str);
            free(indent_str);
            break;

        default:
            printf("%sUnknown node\n",indent_str);
            free(indent_str);
    }
}

void free_ast_node(ast_node_t* node){
    if(!node){
        return;
    }

    switch(node->type){
        case AST_TYPE_PROGRAM:
            for(size_t i = 0;i < node->program.programisms->count;i++){
                free_ast_node(node->program.programisms->nodes[i]);
            }
            free(node->program.programisms->nodes);
            free(node->program.programisms);
            break;

        case AST_TYPE_FUNC_DECL:
            for(size_t i = 0;i < node->func_decl.args->count;i++){
                free_ast_node(node->func_decl.args->nodes[i]);
            }
            free(node->func_decl.args->nodes);
            free(node->func_decl.args);
            break;

        default:
            break;
    }
    free(node);
}
