#include "lexer.h"
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <util.h>

bool append_token(token_t** t,size_t* filled, size_t* size,token_t to_append){
    if((*filled)++ >= *size){
        if (!(*t = realloc(*t, (*size += *size / 2) * sizeof(token_t)))){
            return false;
        }
    }
    (*t)[*filled - 1] = to_append;
    return true;
}

bool append_char(char** t,size_t* filled, size_t* size,char to_append){
    if((*filled)++ >= *size){
        if (!(*t = realloc(*t, (*size += *size / 2) * sizeof(char)))){
            return false;
        }
    }
    (*t)[*filled - 1] = to_append;
    return true;
}

static const uint8_t symbols[127] = {
    ['!']  = TOKEN_TYPE_BANG,
    ['#']  = TOKEN_TYPE_HASH,
    ['$']  = TOKEN_TYPE_DOLLAR,
    ['%']  = TOKEN_TYPE_MODULO,
    ['&']  = TOKEN_TYPE_AND,
    ['(']  = TOKEN_TYPE_LPAREN,
    [')']  = TOKEN_TYPE_RPAREN,
    ['*']  = TOKEN_TYPE_STAR,
    ['+']  = TOKEN_TYPE_PLUS,
    [',']  = TOKEN_TYPE_COMMA,
    ['-']  = TOKEN_TYPE_MINUS,
    ['.']  = TOKEN_TYPE_DOT,
    ['/']  = TOKEN_TYPE_SLASH,
    [':']  = TOKEN_TYPE_COLON,
    [';']  = TOKEN_TYPE_SEMI,
    ['<']  = TOKEN_TYPE_LANGLE,
    ['=']  = TOKEN_TYPE_EQUAL,
    ['>']  = TOKEN_TYPE_RANGLE,
    ['?']  = TOKEN_TYPE_QUESTION,
    ['@']  = TOKEN_TYPE_AT,
    ['[']  = TOKEN_TYPE_LSQUARE,
    ['\\'] = TOKEN_TYPE_BSLASH,
    [']']  = TOKEN_TYPE_RSQUARE,
    ['^']  = TOKEN_TYPE_CARET,
    ['`']  = TOKEN_TYPE_BACKTICK,
    ['{']  = TOKEN_TYPE_LCURLY,
    ['|']  = TOKEN_TYPE_WALL,
    ['}']  = TOKEN_TYPE_RCURLY,
    ['~']  = TOKEN_TYPE_TILDE
};

static const char escapes[127] = {
    ['n']  = '\n',
    ['\\']  = '\\',
    ['\'']  = '\'',
    ['"']  = '"' 
};

typedef bool (*modefn_t)(lexer_mode_t* mode, uint32_t* to_append_type, uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum);

bool process_normalo_mode(lexer_mode_t* mode, uint32_t* to_append_type, uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum){
    switch(**pointer){
        case ' ':
            *last_nalnum = *pointer + 1; // this is not alphanumeric
            break;
        case '\n':
            (*row)++;
            *col = 0;
            *to_append_type = TOKEN_TYPE_NEWLINE;
            *last_nalnum = *pointer + 1; // this is not alphanumeric
            break;

        //these are done according to a LUT to minimize repetition
        case '!':   //fallthru
        case '#':   //fallthru
        case '$':   //fallthru
        case '%':   //fallthru
        case '&':   //fallthru
        case '(':   //fallthru
        case ')':   //fallthru
        case '*':   //fallthru
        case '+':   //fallthru
        case ',':   //fallthru
        case '-':   //fallthru
        case '.':   //fallthru
        case '/':   //fallthru
        case ':':   //fallthru
        case ';':   //fallthru
        case '<':   //fallthru
        case '=':   //fallthru
        case '>':   //fallthru
        case '?':   //fallthru
        case '@':   //fallthru
        case '[':   //fallthru
        case '\\':  //fallthru
        case ']':   //fallthru
        case '^':   //fallthru
        case '`':   //fallthru
        case '{':   //fallthru
        case '|':   //fallthru
        case '}':   //fallthru
        case '~':
            //comment stuff
            if(**pointer == '/' && (*pointer)[1] == '/'){
                (*pointer)++; //just quietly skip that character
                (*col)++;
                *mode = COMMENTO_MODE;
                break;
            }
            if(**pointer == '/' && (*pointer)[1] == '*'){
                (*pointer)++; //just quietly skip that character
                (*col)++;
                *mode = MULTILINO_COMMENTO_MODE;
                break;
            }

            //actual symbol processing
            *to_append_type = symbols[(uint8_t)**pointer];
            *last_nalnum = *pointer + 1; // this is not alphanumeric
            break;

        case '"':
            *mode = STRINGO_LITERALO_MODE;
            *last_nalnum = *pointer + 1; // use last_nalnum for the string buffer too
            break;

        case '\'':
            *mode = CHARACTERO_LITERALO_MODE;
            *last_nalnum = *pointer + 1; // use last_nalnum for the char buffer too
            break;

        default:
            if (*pointer >= *last_nalnum && !isalnum((*pointer)[1]) && (*pointer)[1] != '_'){
                size_t alnum_size = *pointer - *last_nalnum + 1;
                char*  buf = malloc(alnum_size + 1);
                memcpy(buf,*last_nalnum,alnum_size);
                buf[alnum_size] = '\0';
                *to_append_type = TOKEN_TYPE_STRING;
                *to_append_value = (uintptr_t)buf;
            }

    }
    return true;
}

bool process_commento_mode(lexer_mode_t* mode, UNUSED uint32_t* to_append_type, UNUSED uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum){
    switch(**pointer){
        case '\n':
            (*row)++;
            *col = 0;
            *last_nalnum = *pointer + 1; // this is not alphanumeric
            *mode = NORMALO_MODE;
            break;
    }
    return true;
}

bool process_multilino_commento_mode(lexer_mode_t* mode, UNUSED uint32_t* to_append_type, UNUSED uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum){
    switch(**pointer){
        case '\n':
            (*row)++;
            *col = 0;
            break;
        case '*':
            if ((*pointer)[1] == '/') {
                (*pointer)++;
                (*col)++;
                *last_nalnum = *pointer + 1;
                *mode = NORMALO_MODE;
            }
            break;
    }
    return true;
}

//HACK: global state for string and character literals, may break stuff if we want to lex multiple files at once
char* buf = NULL;
size_t buf_filled;
size_t buf_size;

bool process_stringo_literalo_mode(lexer_mode_t* mode, uint32_t* to_append_type, uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum){
    if(buf == NULL){
        buf_size = 2, buf_filled = 0;
        buf = malloc(buf_size);
    }

    switch(**pointer){
        case '"':
            *to_append_type = TOKEN_TYPE_CHRLIT;
            *mode = NORMALO_MODE;
            if (*pointer > *last_nalnum){
                if(!append_char(&buf, &buf_filled, &buf_size, 0)){
                    return false;
                }
                *to_append_value = (uintptr_t)buf;
            } else {
                *to_append_value = 0;
               free(buf);
            }
            buf = NULL;
            break;
        case '\n':
            (*row)++;
            *col = 0;
            break;
        case '\\': // fallthru
            const char e = escapes[(uint8_t)(*pointer)[1]];
            if(e){
                (*pointer)++;
                (*col)++;
                if(!append_char(&buf, &buf_filled, &buf_size, e)){
                    return false;
                }
                break;
            }
        default: //fallthru
            if(!append_char(&buf, &buf_filled, &buf_size, **pointer)){
                return false;
            }
            break;
        }
    return true;
}

bool process_charactero_literalo_mode(lexer_mode_t* mode, uint32_t* to_append_type, uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum){
    if(buf == NULL){
        buf_size = 2, buf_filled = 0;
        buf = malloc(buf_size);
    }

    switch(**pointer){
        case '\'':
            *to_append_type = TOKEN_TYPE_CHRLIT;
            *mode = NORMALO_MODE;
            if (*pointer > *last_nalnum){
                if(!append_char(&buf, &buf_filled, &buf_size, 0)){
                    return false;
                }
                *to_append_value = (uintptr_t)buf;
            } else {
                *to_append_value = 0;
               free(buf);
            }
            buf = NULL;
            break;
        case '\n':
            (*row)++;
            *col = 0;
            break;
        case '\\': // fallthru
            const char e = escapes[(uint8_t)(*pointer)[1]];
            if(e){
                (*pointer)++;
                (*col)++;
                if(!append_char(&buf, &buf_filled, &buf_size, e)){
                    return false;
                }
                break;
            }
        default: //fallthru
            if(!append_char(&buf, &buf_filled, &buf_size, **pointer)){
                return false;
            }
            break;
        }
    return true;
}

static const modefn_t mode_functions[5] = {
    [NORMALO_MODE] = process_normalo_mode,
    [COMMENTO_MODE] = process_commento_mode,
    [MULTILINO_COMMENTO_MODE] = process_multilino_commento_mode,
    [STRINGO_LITERALO_MODE] = process_stringo_literalo_mode,
    [CHARACTERO_LITERALO_MODE] = process_charactero_literalo_mode,
};

token_t* lex(const char* null_terminated_string){
    token_t* root = malloc(sizeof(token_t) * 10); 
    size_t num_tokens = 0;
    size_t size       = 10;

    size_t row = 0;
    size_t col = 0;

    char*  pointer     = (char*)null_terminated_string;
    char*  last_nalnum = (char*)null_terminated_string;
    const size_t len   = strlen(null_terminated_string);

    uint32_t  to_append_type = 0;
    uintptr_t to_append_value = 0;

    lexer_mode_t mode = NORMALO_MODE;

    while(pointer < null_terminated_string + len) {
        if(!mode_functions[mode](&mode, &to_append_type, &to_append_value, &row, &col, &pointer, &last_nalnum))
            goto error;

        if (to_append_type != 0) {
            if (!append_token(&root,&num_tokens,&size,(token_t){
                .type = to_append_type,
                .col =  col,
                .row = row,
                .value = to_append_value
            })) goto error;

            to_append_type = 0;
            to_append_value = 0;
        }

        col++;
        pointer++;
    }

    if (!append_token(&root,&num_tokens,&size,(token_t){
        .type = TOKEN_TYPE_TERMINATOR,
        .col =  col,
        .row = row,
        .value = 0
    })) goto error;

    return root;

    error:
        free(root);
        return NULL;
}

void free_token_array(token_t* array){
    token_t* cur = array;

    do {
        switch (cur->type) {
            case TOKEN_TYPE_TERMINATOR:
                break; // no advance
            case TOKEN_TYPE_STRING: //fallthru
            case TOKEN_TYPE_STRLIT: //fallthru
            case TOKEN_TYPE_CHRLIT: //fallthru
                free((void*)cur->value);
            default: // fallthru
                cur++;
                break;
        }
    } while(cur->type != TOKEN_TYPE_TERMINATOR); 
    free(array);
}
