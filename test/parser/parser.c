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

struct __parser_op {
    enum {
        OPT_UNARY_LEFT,
        OPT_UNARY_RIGHT,
        OPT_BINARY,
        OPT_TERNARY
    } type;
    bool right_assoc;
    token_type_t search_for;
    ast_type_t node_type;
    uint8_t priority;

};

struct __parser_op operators[] = {
    {OPT_BINARY, true, TOKEN_TYPE_EQUAL,AST_TYPE_ASS,0},
    {OPT_BINARY, false, TOKEN_TYPE_OROR,AST_TYPE_LOR,1},
    {OPT_BINARY, false, TOKEN_TYPE_ANDAND,AST_TYPE_LAND,2},
    {OPT_BINARY, false, TOKEN_TYPE_WALL,AST_TYPE_OR,3},
    {OPT_BINARY, false, TOKEN_TYPE_CARET,AST_TYPE_XOR,4},
    {OPT_BINARY, false, TOKEN_TYPE_AND,AST_TYPE_AND,5},

    {OPT_BINARY, false, TOKEN_TYPE_EQEQ,AST_TYPE_EQ,6},
    {OPT_BINARY, false, TOKEN_TYPE_NE,AST_TYPE_NE,6},

    {OPT_BINARY, false, TOKEN_TYPE_LANGLE,AST_TYPE_LT,7},
    {OPT_BINARY, false, TOKEN_TYPE_RANGLE,AST_TYPE_GT,7},
    {OPT_BINARY, false, TOKEN_TYPE_LTE,AST_TYPE_LTE,7},
    {OPT_BINARY, false, TOKEN_TYPE_GTE,AST_TYPE_GTE,7},

    {OPT_BINARY, false, TOKEN_TYPE_PLUS,AST_TYPE_ADD,8},
    {OPT_BINARY, false, TOKEN_TYPE_MINUS,AST_TYPE_SUB,8},


    {OPT_BINARY, false, TOKEN_TYPE_STAR,AST_TYPE_MUL,9},
    {OPT_BINARY, false, TOKEN_TYPE_SLASH,AST_TYPE_DIV,9},
    {OPT_BINARY, false, TOKEN_TYPE_MODULO,AST_TYPE_MOD,9},

    {OPT_UNARY_LEFT, true, TOKEN_TYPE_MINUS,AST_TYPE_UN_MINUS,10},
    {OPT_UNARY_LEFT, true, TOKEN_TYPE_BANG,AST_TYPE_NOT,10},
};

//TODO: expression parsing
ast_node_t* parse_expr_until(src_file_t* file, token_t* until){
    ast_node_t* node = malloc(sizeof(ast_node_t));
    token_t* next = NULL;
    token_t* saved_next = NULL;
    ast_node_t* left = NULL;
    ast_node_t* mid = NULL;
    ast_node_t* right = NULL;
    ast_node_t* saved_left = NULL;
    ast_node_t* saved_mid = NULL;
    ast_node_t* saved_right = NULL;

    node->type = AST_TYPE_PROGRAM; //impossible to get other than not hitting any case

    //unary, binary and ternary expr
    for (uint8_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++){
        struct __parser_op op = operators[i];
        token_t* old = peek(0);
        left = mid = right = NULL; // zero out all of them

        if(op.right_assoc) {
            set_ptr(until-1); // be consistent with the behaviour of not including until
            next = get_prev_uspa(op.search_for, old-1);
            set_ptr(old);
        } else {
            next = get_next_uspa(op.search_for, until);
        }


        if ((saved_left || saved_mid || saved_right)
                && (!next || (op.right_assoc ? saved_next < next : saved_next > next))) {
            //substitute ourselves for the last one
            op = operators[i-1];
            left  = saved_left;
            mid   = saved_mid;
            right = saved_right;
            goto checkout;
        }

        if (!next){
            goto next; //so we actually rewind
        }

        switch (op.type) {
            case OPT_UNARY_LEFT:
                set_ptr(next+1);
                mid = parse_expr_until(file, until);

                if(!mid) goto next; //so we actually rewind
                break;
            case OPT_UNARY_RIGHT:
                mid = parse_expr_until(file, next);

                if(!mid) goto next; //so we actually rewind
                break;
            case OPT_BINARY:
                left = parse_expr_until(file, next);
                set_ptr(next+1);
                right = parse_expr_until(file, until);

                if(!left || !right) goto next; //so we actually rewind
                break;
            case OPT_TERNARY:
                throw_code_issue(*file, COMP_ERR_UNIMPLEMENTED, // TODO: ecapsulation
                                             *peek(0), false); //TODO: non-fatal errors
                free_ast_node(node);
                return NULL;
        }

        if (i+1 != sizeof(operators) / sizeof(operators[0]) && operators[i+1].priority == op.priority) {
            saved_next = next;
            saved_left = left;
            saved_mid = mid;
            saved_right = right;
            set_ptr(old); // it's rewind time
            continue;
        }

        checkout:

        switch (op.type) {
            case OPT_UNARY_LEFT: //fallthru
            case OPT_UNARY_RIGHT:
                if (mid) {
                    node->type = op.node_type;
                    node->unary.expr = mid;
                    goto done;
                }
                break;
            case OPT_BINARY:
                if(left && right) {
                    node->type = op.node_type;
                    node->binary.left = left;
                    node->binary.right = right;
                    goto done;
                }
                break;
            case OPT_TERNARY:
                if(left && mid && right) {
                    node->type = op.node_type;
                    node->ternary.cond = left;
                    node->ternary.val_true = left;
                    node->ternary.val_false = right;
                    goto done;
                }
                break;
        }

        next: //here cuz we have to rewind
            set_ptr(old); // it's rewind time
    }

    //primary expr
    if((next = expect(TOKEN_TYPE_LPAREN))) {
        token_t* rparen = get_next_uspa(TOKEN_TYPE_RPAREN, until+1);

        consume(1); // lparen

        if (!rparen) {
            throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE,
                                             *next, true); //TODO: non-fatal errors
        }

        ast_node_t* newnode = parse_expr_until(file, rparen);

        if (!newnode){
            throw_code_issue(*file, COMP_ERR_EXPECTED_EXPR,
                                             *peek(0), true); //TODO: non-fatal errors
            free_ast_node(node);
            return NULL;
        }

        node = newnode;

    } else if((next = expect(TOKEN_TYPE_CHRLIT))){
        if(!next->value){
            throw_code_issue(*file, COMP_ERR_EMPTY_CHRLIT,
                *next, true); //TODO: non-fatal errors
        }

        if(strlen((char*)next->value) > 1){
            throw_code_issue(*file, COMP_ERR_MULTIBYTE_CHRLIT,
                *next, true); //TODO: non-fatal errors
        }

        node->type = AST_TYPE_CHRLIT;
        node->chrlit.value = ((char*)next->value)[0];

        consume(1); //chrlit
    } else if((next = expect(TOKEN_TYPE_STRING)) && isdigit(((char*)next->value)[0])){ 
        node->type = AST_TYPE_INTLIT;
        node->intlit.value = (char*)next->value;

        consume(1); // string
    } else if((next = expect(TOKEN_TYPE_STRING))){ 
        node->type = AST_TYPE_IDENT;
        node->ident.name = (char*)next->value;

        consume(1); //string
    } else if((next = expect(TOKEN_TYPE_STRLIT))){
        node->type = AST_TYPE_STRLIT;
        node->intlit.value = (char*)next->value;

        consume(1); //strlit
    }

    //postfix expr
    if((next = expect(TOKEN_TYPE_LPAREN))){
        if(node->type == AST_TYPE_PROGRAM){ //unchanged
            throw_code_issue(*file, COMP_ERR_DANGLING_POSTFIX,
                                             *next, true); //TODO: non-fatal errors
            free_ast_node(node);
            return NULL;
        }

        ast_node_t* newnode = malloc(sizeof(ast_node_t));

        if (!newnode){
            throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE,
                                             *peek(0), true);
        }

        newnode->type = AST_TYPE_FUNC_CALL;
        newnode->func_call.fp = node;
        newnode->func_call.args = create_ast_node_list();

        node = newnode;

        token_t* rparen = get_next_us(TOKEN_TYPE_RPAREN, until); //HACK: doesnt account for parenthesis depth

        if(!rparen) {
            throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE,
                                             *peek(0), true); //TODO: non-fatal errors
        }

        do {
            consume(1); //lparen or comma
            token_t* next_comma = get_next_us(TOKEN_TYPE_COMMA, rparen);
            ast_node_t* arg = NULL;

            if(next_comma){
                arg = parse_expr_until(file,next_comma);
            } else {
                arg = parse_expr_until(file,rparen);
            }

            if(arg){
                append_node(newnode->func_call.args, arg);
            }

            next = peek(0);
        } while (next < rparen);

        if((next = peek(0)) != rparen){
            throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE,
                                             *next, true); //TODO: non-fatal errors
        }
        consume(1); //rparen
    }

    done:

    if(node->type == AST_TYPE_PROGRAM){ //unchanged
        free(node);
        return NULL;
    }

    set_ptr(until); //make the pointer position be predictable
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
            throw_code_issue(*file, COMP_ERR_DECLARATION_CUT_OFF,
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
            node->decl.starting_value = parse_expr_until(file,get_next_pa(TOKEN_TYPE_SEMI, TOKEN_TYPE_NEWLINE));
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
                if(!expect(TOKEN_TYPE_LPAREN)){
                    throw_code_issue(*file, COMP_ERR_MISSING_LPAREN, 
                                             *peek(0), true); //TODO: non-fatal errors
                }
                token_t* rparen = get_next_pa(TOKEN_TYPE_RPAREN, TOKEN_TYPE_SEMI);

                if (!rparen) {
                    throw_code_issue(*file, COMP_ERR_MISSING_RPAREN, 
                                             *peek(0), true); //TODO: non-fatal errors
                }

                consume(1); //lparen

                node = malloc(sizeof(ast_node_t));
                node->type = AST_TYPE_IF;

                if(!(node->if_stmt.cond = parse_expr_until(file,rparen))){
                    throw_code_issue(*file, COMP_ERR_EXPECTED_EXPR, 
                                         *peek(0), true); //TODO: non-fatal errors
                }

                if(!expect_d(TOKEN_TYPE_RPAREN)){
                    throw_code_issue(*file, COMP_ERR_MISSING_RPAREN, 
                                             *peek(0), true); //TODO: non-fatal errors
                }

                if(!(node->if_stmt.body = parse_stmt(file))){
                    throw_code_issue(*file, COMP_ERR_EXPECTED_STMT, 
                                         *peek(0), true); //TODO: non-fatal errors
                }

                return node; //return early to dodge the semicolon check
            case 1: //goto
                if(!(next = expect_d(TOKEN_TYPE_STRING))){
                    throw_code_issue(*file, COMP_ERR_EXPECTED_IDENT, 
                                             *peek(0), true); //TODO: non-fatal errors
                }
                node = malloc(sizeof(ast_node_t));
                node->type = AST_TYPE_GOTO;
                node->goto_stmt.label = (char*)next->value;
                break;
            case 2: // return
                node = malloc(sizeof(ast_node_t));
                node->type = AST_TYPE_RETURN;
                if(!(node->return_stmt.val = parse_expr_until(file,get_next_pa(TOKEN_TYPE_SEMI, TOKEN_TYPE_TERMINATOR))))
                   throw_code_issue(*file, COMP_ERR_EXPECTED_STMT,
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
                throw_code_issue(*file, COMP_ERR_EXPECTED_STMT,
                                       *peek(0), true); //TODO: non-fatal errors
            }
            append_node(node->block.stmts, stmt);
        }

        if (!expect_d(TOKEN_TYPE_RCURLY)) {
            throw_code_issue(*file, COMP_ERR_MISSING_RPAREN, //TODO: custom issue for this
                                       *peek(0), true); //TODO: non-fatal errors
        }

        return node; //return early to dodge the semicolon check

    } else if (next->type == TOKEN_TYPE_STRING && peek(1)->type == TOKEN_TYPE_COLON) {
        if (!next->value){
            throw_code_issue(*file, COMP_ERR_INTERNAL_FAILIURE,
                                             *next, true); //TODO: non-fatal errors
        }
        
        node = malloc(sizeof(ast_node_t));
        node->type = AST_TYPE_LABEL;
        node->label.name = (char*)next->value;
        consume(2); //string and colon
    } else {
        node = malloc(sizeof(ast_node_t));
        node->type = AST_TYPE_EVAL;
        node->eval.expr = parse_expr_until(file,get_next(TOKEN_TYPE_SEMI, TOKEN_TYPE_TERMINATOR));
        if(!node->eval.expr){
            throw_code_issue(*file, COMP_ERR_EXPECTED_EXPR,
                                       *peek(0), true); //TODO: non-fatal errors
        }
    }

    if(!expect_d(TOKEN_TYPE_SEMI)){
        throw_code_issue(*file, COMP_ERR_MISSING_SEMICOLON,
                                         *next, false); //TODO: non-fatal errors
    }

    return node;
}

ast_node_t* parse_programism(src_file_t* file){

    if (peek(0)->type == TOKEN_TYPE_TERMINATOR){
        return NULL;
    }

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
                                 *peek(0), true); //TODO: non-fatal errors
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

        case AST_TYPE_BLOCK:
            printf("%sBlock:\n",indent_str);
            for(size_t i = 0;i < node->block.stmts->count;i++){
                print_ast_node(node->block.stmts->nodes[i], indent + INDENT_WIDTH);
            }
            break;
        
        case AST_TYPE_EVAL:
            printf("%sEval:\n",indent_str);
            print_ast_node(node->eval.expr, indent + INDENT_WIDTH);
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

        case AST_TYPE_LABEL:
            printf("%sLabel \"%s\"\n",indent_str,node->label.name);
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

        case AST_TYPE_IF:
            printf("%sIf statement\n",indent_str);
            printf("%s  |->Condition: \n",indent_str);
            print_ast_node(node->if_stmt.cond, indent + INDENT_WIDTH*2);
            printf("%s  \\->Body: \n",indent_str);
            print_ast_node(node->if_stmt.body, indent + INDENT_WIDTH*2);
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

        case AST_TYPE_GOTO:
            printf("%sGoto statement\n",indent_str);
            printf("%s  \\->label: %s\n",indent_str,node->goto_stmt.label);
            free(indent_str);
            break;

        case AST_TYPE_ASS:
            printf("%sAssignment:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_ADD:
            printf("%sAddition\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_SUB:
            printf("%sSubtraction\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_AND:
            printf("%sBitwise and:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_LAND:
            printf("%sLogical and:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_OR:
            printf("%sBitwise or:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_LOR:
            printf("%sLogical or:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_XOR:
            printf("%sExclusive OR:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;
        
        case AST_TYPE_EQ:
            printf("%sEq check:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_NE:
            printf("%sNONequalty check:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_GT:
            printf("%sGreater than:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_LT:
            printf("%sLess than:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_LTE:
            printf("%sLess than or equal:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_GTE:
            printf("%sGreater than or equal:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_MUL:
            printf("%sMultiplication:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_DIV:
            printf("%sDivision:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_TERNARY_COND:
            printf("%sDivision:\n",indent_str);
            print_ast_node(node->ternary.cond, indent + INDENT_WIDTH);
            print_ast_node(node->ternary.val_true, indent + INDENT_WIDTH);
            print_ast_node(node->ternary.val_false, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_MOD:
            printf("%sModulo:\n",indent_str);
            print_ast_node(node->binary.left, indent + INDENT_WIDTH);
            print_ast_node(node->binary.right, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_UN_MINUS:
            printf("%sUnary minus:\n",indent_str);
            print_ast_node(node->unary.expr, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_NOT:
            printf("%sNot:\n",indent_str);
            print_ast_node(node->unary.expr, indent + INDENT_WIDTH);
            free(indent_str);
            break;

        case AST_TYPE_EMPTY_STMT:
            printf("%sEmpty statement\n",indent_str);
            free(indent_str);
            break;

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

        case AST_TYPE_DECL:
            //name owned by the token
            free_ast_node(node->decl.starting_value);
            break;

        case AST_TYPE_FUNC_CALL:
            for(size_t i = 0;i < node->func_call.args->count;i++){
                free_ast_node(node->func_call.args->nodes[i]);
            }
            free(node->func_call.args->nodes);
            free(node->func_call.args);
            free_ast_node(node->func_call.fp);
            break;

        case AST_TYPE_BLOCK:
            for(size_t i = 0;i < node->block.stmts->count;i++){
                free_ast_node(node->block.stmts->nodes[i]);
            }
            free(node->block.stmts->nodes);
            free(node->block.stmts);
            break;

        case AST_TYPE_LABEL:
            free(node->label.name);
            break;

        case AST_TYPE_STRLIT:
            //value owned by the token
            break;

        case AST_TYPE_IF:
            free_ast_node(node->if_stmt.cond);
            free_ast_node(node->if_stmt.body);
            break;

        case AST_TYPE_GOTO:
            free(node->goto_stmt.label);
            break;

        case AST_TYPE_RETURN:
            if (node->return_stmt.val){
                free_ast_node(node->return_stmt.val);
            }
            break;

        case AST_TYPE_EVAL:
            free_ast_node(node->eval.expr);
            break;

        case AST_TYPE_IDENT:
            free(node->ident.name);
            break;

        case AST_TYPE_NOT:
        case AST_TYPE_UN_MINUS:
            free_ast_node(node->unary.expr);
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
        
        case AST_TYPE_TERNARY_COND:
            free_ast_node(node->ternary.cond);
            free_ast_node(node->ternary.val_false);
            free_ast_node(node->ternary.val_true);
            break;

        case AST_TYPE_CHRLIT:
        case AST_TYPE_INTLIT:
        case AST_TYPE_DOTDOTDOT:
        case AST_TYPE_EMPTY_STMT:
            break;
    }
    free(node);
}
