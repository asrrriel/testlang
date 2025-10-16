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

static const char* stmt_kws[3] = {
    "if",
    "goto",
    "return",
};

static bool is_stmt_kw(token_t* token){
    if(!token || token->type != TOKEN_TYPE_STRING || !token->value){
        return false;
    }

    for(uint8_t i = 0; i < 3; i++){
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
ast_node_t* parse_expr_until(src_file_t* file, token_t* until){
    ast_node_t* node = malloc(sizeof(ast_node_t));
    token_t* next;

    node->type = AST_TYPE_PROGRAM; //impossible to get other than not hitting any case

    //assignment
    token_t* old = peek(0);
    set_ptr(until);
    if((next = get_prev_us(TOKEN_TYPE_EQUAL, old))) {
        print_token(next);
        set_ptr(old);
        ast_node_t* left = parse_expr_until(file, next);
        set_ptr(next+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);


        node->type = AST_TYPE_ASS;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    }

    //TODO: ternary

    //logical OR
    set_ptr(old);
    if((next = get_next_us(TOKEN_TYPE_OROR, until))) {
        ast_node_t* left = parse_expr_until(file, next);
        set_ptr(next+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_LOR;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    }

    //logical AND
    if((next = get_next_us(TOKEN_TYPE_ANDAND, until))) {
        ast_node_t* left = parse_expr_until(file, next);
        set_ptr(next+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_LAND;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    }

    //bitwise OR
    if((next = get_next_us(TOKEN_TYPE_WALL, until))) {
        ast_node_t* left = parse_expr_until(file, next);
        set_ptr(next+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_OR;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    }

    //bitwise XOR
    if((next = get_next_us(TOKEN_TYPE_CARET, until))) {
        ast_node_t* left = parse_expr_until(file, next);
        set_ptr(next+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_XOR;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    }

    //bitwise AND
    if((next = get_next_us(TOKEN_TYPE_AND, until))) {
        ast_node_t* left = parse_expr_until(file, next);
        set_ptr(next+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_AND;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    }

    //equality and unequality checks
    token_t* next1 = get_next_us(TOKEN_TYPE_EQEQ, until);
    token_t* next2 = get_next_us(TOKEN_TYPE_NE, until);

    if(next1 && next1 < next2) {
        ast_node_t* left = parse_expr_until(file, next1);
        set_ptr(next1+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_EQ;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    } else if(next2) { // if next2 > next1
        ast_node_t* left = parse_expr_until(file, next2);
        set_ptr(next2+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_NE;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    }

    //LT,LTE,GT and GTE checks
    next1 = get_next_us(TOKEN_TYPE_RANGLE, until);
    next2 = get_next_us(TOKEN_TYPE_LANGLE, until);
    token_t* next3 = get_next_us(TOKEN_TYPE_LTE, until);
    token_t* next4 = get_next_us(TOKEN_TYPE_GTE, until);

    if(next1 && next1 < next2 && next2 < next3 && next3 < next4) {
        ast_node_t* left = parse_expr_until(file, next1);
        set_ptr(next1+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_LT;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    } else if(next2 && next2 < next3 && next3 < next4) {
        ast_node_t* left = parse_expr_until(file, next2);
        set_ptr(next2+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_GT;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    } else if(next3 && next3 < next4) {
        ast_node_t* left = parse_expr_until(file, next3);
        set_ptr(next3+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_LTE;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    } else if(next4) {
        ast_node_t* left = parse_expr_until(file, next4);
        set_ptr(next4+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_GTE;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    }

    //additions and subtractions
    next1 = get_next_us(TOKEN_TYPE_PLUS, until);
    next2 = get_next_us(TOKEN_TYPE_MINUS, until);

    if(next1 && (next1 < next2 || !next2)) {
        ast_node_t* left = parse_expr_until(file, next1);
        set_ptr(next1+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_ADD;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    } else if(next2) { // if next2 > next1
        ast_node_t* left = parse_expr_until(file, next2);
        set_ptr(next2+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        if(left && right){
            node->type = AST_TYPE_SUB;
            node->binary.left = left;
            node->binary.right = right;

            goto done; //fuck you
        }
    }

    //multiplications, divisions and remainders
    next1 = get_next_us(TOKEN_TYPE_STAR, until);
    next2 = get_next_us(TOKEN_TYPE_SLASH, until);
    next3 = get_next_us(TOKEN_TYPE_MODULO, until);

    if(next1 && next1 < next2 && next2 < next3 ) {
        ast_node_t* left = parse_expr_until(file, next1);
        set_ptr(next1+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_MUL;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    } else if(next2 && next2 < next3) {
        ast_node_t* left = parse_expr_until(file, next2);
        set_ptr(next2+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_DIV;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    } else if(next3) {
        ast_node_t* left = parse_expr_until(file, next3);
        set_ptr(next3+1);
        ast_node_t* right = parse_expr_until(file, until);
        set_ptr(until);

        node->type = AST_TYPE_MOD;
        node->binary.left = left;
        node->binary.right = right;

        goto done; //fuck you
    }

    //unary minuses and nots
    set_ptr(until);
    next1 = get_prev_us(TOKEN_TYPE_MINUS, old-1);
    next2 = get_prev_us(TOKEN_TYPE_BANG, old-1);

    if(next1 && (next1 > next2 || !next2)) {
        set_ptr(next1+1);
        ast_node_t* right = parse_expr_until(file, until);

        node->type = AST_TYPE_UN_MINUS;
        node->unary.expr = right;

        goto done; //fuck you
    } else if(next2) { // if next2 < next1
        set_ptr(next2+1);
        ast_node_t* right = parse_expr_until(file, until);

        node->type = AST_TYPE_NOT;
        node->unary.expr = right;

        goto done; //fuck you
    }
    set_ptr(old);

    //primary expr
    if((next = expect(TOKEN_TYPE_LPAREN))) {
        throw_code_issue(*file, COMP_ERR_UNIMPLEMENTED, // TODO: ecapsulation
                                             *peek(0), false); //TODO: non-fatal errors
        free_ast_node(node);
        return NULL;
    } else if((next = expect(TOKEN_TYPE_CHRLIT))){
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
    } else if((next = expect(TOKEN_TYPE_STRING))){ 
        node->type = AST_TYPE_IDENT;
        node->ident.name = (char*)next->value;
        consume(1);
    } else if((next = expect(TOKEN_TYPE_STRLIT))){
        node->type = AST_TYPE_STRLIT;
        node->intlit.value = (char*)next->value;
        consume(1);
    }

    while ((next = expect(TOKEN_TYPE_LPAREN))){

        if(node->type == AST_TYPE_PROGRAM){ //unchanged
            throw_code_issue(*file, COMP_ERR_DANGLING_POSTFIX,
                                             *next, true); //TODO: non-fatal errors
        }

        ast_node_t* newnode = malloc(sizeof(ast_node_t));

        newnode->type = AST_TYPE_FUNC_CALL;
        newnode->func_call.fp = node;
        newnode->func_call.args = create_ast_node_list();

        node = newnode;

        token_t* rparen = get_next_us(TOKEN_TYPE_RPAREN, until); //HACK: doesnt account for parenthesis depth

        do {
            consume(1); //lparen or comma
            token_t* next_comma = get_next_us(TOKEN_TYPE_COMMA, rparen);

            ast_node_t* arg = parse_expr_until(file,next_comma);

            if(arg){
                append_node(newnode->func_call.args, arg);
            }

            next = peek(0);
        } while (next < rparen);

        if(next != rparen){
            throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE,
                                             *peek(0), true); //TODO: non-fatal errors
        }
        consume(1); //rparen
    }

    done:

    if(node->type == AST_TYPE_PROGRAM){ //unchanged
        free(node);
        return NULL;
    }

    return node;
}

//TODO: statement parsing
ast_node_t* parse_stmt(UNUSED src_file_t* file){
    ast_node_t* node = NULL;
    token_t* next = peek(0);

    if(is_qualifier(next) || is_basetype(next)){
        node = malloc(sizeof(ast_node_t));
        node->type = AST_TYPE_DECL;

        decl_stem_t stem = parse_decl_stem(file);

        if(!stem.name) {
            throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE, //TODO: special error for this
                                             *peek(0), true); //TODO: non-fatal errors
        }

        node->decl.type = stem.type;
        node->decl.name = stem.name;

        token_t* next = peek(0);

        if(!next || next->type == TOKEN_TYPE_TERMINATOR) {
            throw_code_issue(*file, COMP_ERR_DECLARATION_CUT_OFF,
                                     *peek(0), true); //TODO: non-fatal errors
        }

        if(next->type == TOKEN_TYPE_SEMI){
            consume(1);
            node->decl.starting_value = NULL;
        } else if(next->type == TOKEN_TYPE_EQUAL){
            consume(1);
            node->decl.starting_value = parse_expr_until(file,get_next(TOKEN_TYPE_SEMI, TOKEN_TYPE_NEWLINE));
        }
    } else if(is_stmt_kw(next)){
        uint8_t kw = 0;
        for(uint8_t i = 0; i < 3; i++){
            if(strcmp((char*)next->value,stmt_kws[i]) == 0){
                kw = i;
            }
        }
        consume(1); // keyword
        switch (kw){
            case 0: //if
                if(!expect_d(TOKEN_TYPE_LPAREN)){
                    throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE, //TODO: special error for this
                                             *peek(0), true); //TODO: non-fatal errors
                }
                node = malloc(sizeof(ast_node_t));
                node->type = AST_TYPE_IF;
                if(!(node->if_stmt.cond = parse_expr_until(file,get_next(TOKEN_TYPE_RPAREN, TOKEN_TYPE_SEMI)))) //TODO: paren depth aware
                    throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE, //TODO: special error for this
                                         *peek(0), true); //TODO: non-fatal errors
                if(!(node->if_stmt.body = parse_stmt(file)))
                    throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE, //TODO: special error for this
                                         *peek(0), true); //TODO: non-fatal errors
                break;
            case 1: //goto
                if(!(next = expect_d(TOKEN_TYPE_STRING))){
                    throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE, //TODO: special error for this
                                             *peek(0), true); //TODO: non-fatal errors
                }
                node = malloc(sizeof(ast_node_t));
                node->type = AST_TYPE_GOTO;
                node->goto_stmt.label = (char*)next->value;
                break;
            case 2: // return
                node = malloc(sizeof(ast_node_t));
                node->type = AST_TYPE_RETURN;
                if(!(node->return_stmt.val = parse_expr_until(file,get_next(TOKEN_TYPE_SEMI, TOKEN_TYPE_TERMINATOR)))) //TODO: paren depth aware
                   throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE, //TODO: special error for this
                                        *peek(0), true); //TODO: non-fatal errors
                break;
        }
    } else if(next->type == TOKEN_TYPE_LCURLY){
        consume(1); //lcurly
        node = malloc(sizeof(ast_node_t));
        node->type = AST_TYPE_BLOCK;
        node->block.stmts = create_ast_node_list();

        while(!expect(TOKEN_TYPE_RCURLY)){
            ast_node_t* stmt = parse_stmt(file);
            if(!stmt){
                throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE, //TODO: special error for this
                                       *peek(0), true); //TODO: non-fatal errors
            }
            append_node(node->block.stmts, stmt);
        }

        consume(1); //rcurly
    } else {
        node = malloc(sizeof(ast_node_t));
        node->type = AST_TYPE_EVAL;
        node->eval.expr = parse_expr_until(file,get_next(TOKEN_TYPE_SEMI, TOKEN_TYPE_TERMINATOR));
        if(!node->eval.expr){
            throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE, //TODO: special error for this
                                       *peek(0), true); //TODO: non-fatal errors
        }
    }

    if(!expect_d(TOKEN_TYPE_SEMI)){
        print_token(peek(0));
        throw_code_issue(*file, COMP_ERR_MISSING_SEMICOLON,
                                         *next, false); //TODO: non-fatal errors
    }

    return node;
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
        node->decl.starting_value = parse_expr_until(file,get_next(TOKEN_TYPE_SEMI, TOKEN_TYPE_TERMINATOR));
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

            if(expect(TOKEN_TYPE_DOTDOTDOT)){
                ast_node_t* param = malloc(sizeof(ast_node_t));
                param->type = AST_TYPE_DOTDOTDOT;
                append_node(node->func_decl.args, param);
                consume(1); // dotdotdot

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
            node->func_decl.body = NULL;
        } else {
            if(!(node->func_decl.body = parse_stmt(file))){
                free_ast_node(node);
                return NULL;
            }

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
        free(indent_str);
        return;
    }

    switch(node->type){
        case AST_TYPE_PROGRAM:
            printf("%sProgram:\n",indent_str);
            free(indent_str); // no need for it after this
            for(size_t i = 0;i < node->program.programisms->count;i++){
                print_ast_node(node->program.programisms->nodes[i], indent + INDENT_WIDTH);
            }
            break;

        case AST_TYPE_DECL:
            printf("%sDeclaration:\n",indent_str);
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
            printf("%sFunction declaration:\n",indent_str);
            printf("%s  |->Return type(+qualifiers):\n",indent_str);
            print_storage_type(node->decl.type, indent + INDENT_WIDTH*2);
            printf("%s  |->Name: \"%s\"\n",indent_str,node->func_decl.name);
            printf("%s  |->Arguments: \n",indent_str);
            for(size_t i = 0;i < node->func_decl.args->count;i++){
                print_ast_node(node->func_decl.args->nodes[i], indent + INDENT_WIDTH*2);
            }
            if(node->func_decl.body){
                printf("%s  \\->Body: \n",indent_str);
                print_ast_node(node->func_decl.body, indent + INDENT_WIDTH*2);
            }
            free(indent_str); // no need for it after this
            break;

        case AST_TYPE_FUNC_CALL:
            printf("%sFunction call:\n",indent_str);
            printf("%s  |->Function pointer to call: \n",indent_str);
            print_ast_node(node->func_call.fp, indent + INDENT_WIDTH*2);
            printf("%s  \\->Arguments: \n",indent_str);
            free(indent_str); // no need for it after this
            for(size_t i = 0;i < node->func_call.args->count;i++){
                print_ast_node(node->func_call.args->nodes[i], indent + INDENT_WIDTH*2);
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
            printf("%sElipses\n",indent_str);
            free(indent_str);
            break;

        case AST_TYPE_IDENT:
            printf("%sIdentifier \"%s\"\n",indent_str,node->ident.name);
            free(indent_str);
            break;
       
        case AST_TYPE_RETURN:
            printf("%sReturn statement\n",indent_str);
            if(node->return_stmt.val){
                printf("%s  \\->Value: \n",indent_str);
                print_ast_node(node->return_stmt.val, indent + INDENT_WIDTH*2);
            } 
            free(indent_str);
            break;

        case AST_TYPE_ADD:
            printf("%sAddition\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        default:
            printf("%sUnknown '%u'\n",indent_str,node->type);
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
        
        case AST_TYPE_FUNC_CALL:
            for(size_t i = 0;i < node->func_call.args->count;i++){
                free_ast_node(node->func_call.args->nodes[i]);
            }
            free(node->func_call.args->nodes);
            free(node->func_call.args);
            free_ast_node(node->func_call.fp);
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
            free_ast_node(node->binary.left);
            free_ast_node(node->binary.right);
            break;

        default:
            break;
    }
    free(node);
}
