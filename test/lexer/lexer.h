#ifndef __LEXER_H__
#define __LEXER_H__

#define TOKEN_TYPE_TERMINATOR   0
#define TOKEN_TYPE_NEWLINE      1
#define TOKEN_TYPE_STRING       2
#define TOKEN_TYPE_STRLIT       3
#define TOKEN_TYPE_CHRLIT       4
#define TOKEN_TYPE_BANG         5
#define TOKEN_TYPE_HASH         6
#define TOKEN_TYPE_DOLLAR       7
#define TOKEN_TYPE_MODULO       8
#define TOKEN_TYPE_AND          9
#define TOKEN_TYPE_LPAREN       10
#define TOKEN_TYPE_RPAREN       11
#define TOKEN_TYPE_STAR         12
#define TOKEN_TYPE_PLUS         13
#define TOKEN_TYPE_COMMA        14
#define TOKEN_TYPE_MINUS        15
#define TOKEN_TYPE_DOT          16
#define TOKEN_TYPE_SLASH        17
#define TOKEN_TYPE_COLON        18
#define TOKEN_TYPE_SEMI         19
#define TOKEN_TYPE_LANGLE       20
#define TOKEN_TYPE_EQUAL        21
#define TOKEN_TYPE_RANGLE       22
#define TOKEN_TYPE_QUESTION     23
#define TOKEN_TYPE_AT           24
#define TOKEN_TYPE_LSQUARE      25
#define TOKEN_TYPE_BSLASH       26
#define TOKEN_TYPE_RSQUARE      27
#define TOKEN_TYPE_CARET        28
#define TOKEN_TYPE_BACKTICK     30
#define TOKEN_TYPE_LCURLY       31
#define TOKEN_TYPE_WALL         32
#define TOKEN_TYPE_RCURLY       33
#define TOKEN_TYPE_TILDE        34

#define NORMALO_MODE 0
#define COMMENTO_MODE 1
#define MULTILINO_COMMENTO_MODE 2
#define STRINGO_LITERALO_MODE 3
#define CHARACTERO_LITERALO_MODE 4

#include <stdint.h>

typedef struct {
    uint32_t type; //size doesnt matter unless you pack the struct
    uintptr_t value;
    uint32_t col;
    uint32_t row;
} token_t;

token_t* lex(const char* null_terminated_string);

#endif // __LEXER_H__
