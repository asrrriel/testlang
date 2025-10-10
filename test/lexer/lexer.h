#ifndef __LEXER_H__
#define __LEXER_H__

typedef enum {
    TOKEN_TYPE_NEWLINE,
    TOKEN_TYPE_TERMINATOR, 
    TOKEN_TYPE_STRLIT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_BANG,
    TOKEN_TYPE_CHRLIT,
    TOKEN_TYPE_DOLLAR,
    TOKEN_TYPE_HASH,
    TOKEN_TYPE_AND,
    TOKEN_TYPE_MODULO,
    TOKEN_TYPE_RPAREN,
    TOKEN_TYPE_LPAREN,
    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_STAR,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_SLASH,
    TOKEN_TYPE_DOT,
    TOKEN_TYPE_SEMI,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_EQUAL,
    TOKEN_TYPE_LANGLE,
    TOKEN_TYPE_QUESTION,
    TOKEN_TYPE_RANGLE,
    TOKEN_TYPE_LSQUARE,
    TOKEN_TYPE_AT,
    TOKEN_TYPE_RSQUARE,
    TOKEN_TYPE_BSLASH,
    TOKEN_TYPE_BACKTICK,
    TOKEN_TYPE_CARET,
    TOKEN_TYPE_WALL,
    TOKEN_TYPE_LCURLY,
    TOKEN_TYPE_TILDE,
    TOKEN_TYPE_RCURLY
} token_type_t;


typedef enum {
    NORMALO_MODE,
    COMMENTO_MODE,
    MULTILINO_COMMENTO_MODE,
    STRINGO_LITERALO_MODE,
    CHARACTERO_LITERALO_MODE,
} lexer_mode_t;


#include <stdint.h>

typedef struct {
    token_type_t type;
    uintptr_t value;
    uint32_t col;
    uint32_t row;
} token_t;

token_t* lex(const char* null_terminated_string);

void print_token(token_t token);

void free_token_array(token_t* array);

#endif // __LEXER_H__
