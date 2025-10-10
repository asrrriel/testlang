#ifndef __AST_H__
#define __AST_H__

#include <stdint.h>
#include <stddef.h>

typedef enum {
    BASE_TYPE_CHAR,
    BASE_TYPE_SHORT,
    BASE_TYPE_INT,
    BASE_TYPE_VOID
} base_type_t;

#define QUAL_CONST      1 << 0
#define QUAL_LONG       1 << 1
#define QUAL_UNSIGNED   1 << 2
#define QUAL_EXTERN     1 << 3

typedef struct {
    uint8_t qualifiers;
    base_type_t base;
    size_t ptr_depth; // ehh its fine for now
} storage_type_t;

typedef enum {
    //global things
    AST_TYPE_PROGRAM,
    AST_TYPE_DECL,
    AST_TYPE_FUNC_DECL,

    //statements
    AST_TYPE_LABEL,
    AST_TYPE_BLOCK,
    AST_TYPE_EVAL,
    AST_TYPE_IF,
    AST_TYPE_GOTO,
    AST_TYPE_RETURN,

    //primary expressions
    AST_TYPE_PAREN,
    AST_TYPE_INTLIT,
    AST_TYPE_CHRLIT,
    AST_TYPE_STRLIT,
    AST_TYPE_IDENT,

    //unary expressions
    AST_TYPE_UN_MINUS,
    AST_TYPE_NOT,

    //binary expressions
    AST_TYPE_ADD,
    AST_TYPE_SUB,
    AST_TYPE_MUL,
    AST_TYPE_DIV,
    AST_TYPE_MOD,
    AST_TYPE_AND,
    AST_TYPE_OR,
    AST_TYPE_XOR,
    AST_TYPE_LAND,
    AST_TYPE_LOR,
    AST_TYPE_EQ,
    AST_TYPE_NE,
    AST_TYPE_GT,
    AST_TYPE_LT,
    AST_TYPE_GTE,
    AST_TYPE_LTE,

    //ternary expressions
    AST_TYPE_FUNC_CALL,
    AST_TYPE_TERNARY_COND
} ast_type_t;

typedef struct __ast_node {
    ast_type_t type;

    union {
        struct {
            struct __ast_node* programisms;
        } program;

        struct {
            storage_type_t type;
            char* name;
            struct __ast_node* starting_value;
        } decl;

        struct {
            storage_type_t return_type;
            char* name;
            struct __ast_node* args;
            struct __ast_node* body;
        } func_decl;

        struct {
            char* name;
        } label;

        struct {
            struct __ast_node* stmts;
        } block;

        struct {
            struct __ast_node* expr;
        } eval;

        struct {
            struct __ast_node* cond;
            struct __ast_node* body;
        } if_stmt;

        struct {
            char* label;
        } goto_stmt;

        struct {
            struct __ast_node* val;
        } return_stmt;

        struct {
            struct __ast_node* expr;
        } paren;

        struct {
            storage_type_t type;
            uint64_t value;
        } intlit;

        struct {
            char* value;
        } strlit;

        struct {
            char value;
        } chrlit;

        struct {
            char* name;
        } ident;

        struct {
            struct __ast_node* expr;
        } unary;

        struct {
            struct __ast_node* left;
            struct __ast_node* right;
        } binary;

        struct {
            struct __ast_node* cond;
            struct __ast_node* val_true;
            struct __ast_node* val_false;
        } ternary;

        struct {
            struct __ast_node* fp;
            struct __ast_node* args;
        } func_call;
    };
} ast_node_t;

#endif // __AST_H__
