#include "lexer.h"
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <util/util.h>

bool append_token(token_t** t,size_t* filled, size_t* size,token_t to_append){
    if((*filled)++ >= *size){
        if (!(*t = realloc(*t, (*size += *size / 2) * sizeof(token_t)))){
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

typedef bool (*modefn_t)(lexer_mode_t* mode, uint32_t* to_append_type, uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum, bool* in_escape);

bool process_normalo_mode(lexer_mode_t* mode, uint32_t* to_append_type, uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum, bool* in_escape){
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
            if(**pointer == '&' && (*pointer)[1] == '&'){
                *to_append_type = TOKEN_TYPE_ANDAND;
                (*pointer)++; //just quietly skip that character
                (*col)++;
                *last_nalnum = *pointer + 1; // this is not alphanumeric
                break;
            }
            if(**pointer == '|' && (*pointer)[1] == '|'){
                *to_append_type = TOKEN_TYPE_OROR;
                (*pointer)++; //just quietly skip that character
                (*col)++;
                *last_nalnum = *pointer + 1; // this is not alphanumeric
                break;
            }
            if(**pointer == '!' && (*pointer)[1] == '='){
                *to_append_type = TOKEN_TYPE_NE;
                (*pointer)++; //just quietly skip that character
                (*col)++;
                *last_nalnum = *pointer + 1; // this is not alphanumeric
                break;
            }
            if(**pointer == '>' && (*pointer)[1] == '='){
                *to_append_type = TOKEN_TYPE_GTE;
                (*pointer)++; //just quietly skip that character
                (*col)++;
                *last_nalnum = *pointer + 1; // this is not alphanumeric
                break;
            }
            if(**pointer == '<' && (*pointer)[1] == '='){
                *to_append_type = TOKEN_TYPE_LTE;
                (*pointer)++; //just quietly skip that character
                (*col)++;
                *last_nalnum = *pointer + 1; // this is not alphanumeric
                break;
            }
            if(**pointer == '.' && (*pointer)[1] == '.' && (*pointer)[2] == '.'){
                *to_append_type = TOKEN_TYPE_DOTDOTDOT;
                (*pointer)+=2; //just quietly skip that character
                (*col)+=2;
                *last_nalnum = *pointer + 1; // this is not alphanumeric
                break;
            }

            //actual symbol processing
            *to_append_type = symbols[(uint8_t)**pointer];
            *last_nalnum = *pointer + 1; // this is not alphanumeric
            break;

        case '"':
            *in_escape = false;
            *mode = STRINGO_LITERALO_MODE;
            *last_nalnum = *pointer + 1; // use last_nalnum for the string buffer too
            break;

        case '\'':
            *in_escape = false;
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

bool process_commento_mode(lexer_mode_t* mode, UNUSED uint32_t* to_append_type, UNUSED uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum, UNUSED bool* in_escape){
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

bool process_multilino_commento_mode(lexer_mode_t* mode, UNUSED uint32_t* to_append_type, UNUSED uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum, UNUSED bool* in_escape){
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

bool process_stringo_literalo_mode(lexer_mode_t* mode, uint32_t* to_append_type, uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum, bool* in_escape){
    switch(**pointer){
        case '\n':
            (*row)++;
            *col = 0;
            *in_escape = false;
            break;
        case '\\':
            *in_escape = !(*in_escape); // accout for the escape "\\"'s second backslash triggering this
            break;
        case '"':
            if (*pointer > *last_nalnum && !(*in_escape)){
                size_t alnum_size = *pointer - *last_nalnum;
                char*  buf = malloc(alnum_size + 1);

                memcpy(buf,*last_nalnum,alnum_size);
                buf[alnum_size] = '\0';

                *to_append_type = TOKEN_TYPE_STRLIT;
                *to_append_value = (uintptr_t)buf;

                *mode = NORMALO_MODE;
            }
        default: //fallthru
            *in_escape = false;
            break;
    }
    return true;
}

bool process_charactero_literalo_mode(lexer_mode_t* mode, uint32_t* to_append_type, uintptr_t* to_append_value, size_t* row, size_t* col, char** pointer, char** last_nalnum, bool* in_escape){
    switch(**pointer){
        case '\n':
            (*row)++;
            *col = 0;
            *in_escape = false;
            break;
        case '\\':
            *in_escape = !(*in_escape); // accout for the escape "\\"'s second backslash triggering this
            break;
        case '\'':
            if (*pointer > *last_nalnum && !(*in_escape)){
                size_t alnum_size = *pointer - *last_nalnum;
                char*  buf = malloc(alnum_size + 1);

                memcpy(buf,*last_nalnum,alnum_size);
                buf[alnum_size] = '\0';

                *to_append_type = TOKEN_TYPE_CHRLIT;
                *to_append_value = (uintptr_t)buf;

                *mode = NORMALO_MODE;
            }
        default: //fallthru
            *in_escape = false;
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

    size_t row = 1;
    size_t col = 0;

    char*  pointer     = (char*)null_terminated_string;
    char*  last_nalnum = (char*)null_terminated_string;
    const size_t len   = strlen(null_terminated_string);

    uint32_t  to_append_type = 0;
    uintptr_t to_append_value = 0;

    lexer_mode_t mode = NORMALO_MODE;
    bool in_escape = false;

    while(pointer < null_terminated_string + len) {
        if(!mode_functions[mode](&mode, &to_append_type, &to_append_value, &row, &col, &pointer, &last_nalnum,&in_escape))
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

void print_token(token_t* token){
    if(token == NULL){
        printf("Null token or failed expect\n");
        return;
    }

    switch (token->type) {
        case TOKEN_TYPE_TERMINATOR:
            break;
        case TOKEN_TYPE_NEWLINE:
            printf("Newline at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_STRING:
            printf("String \"%s\" at %u:%u\n",(char*)token->value,token->row,token->col);
            break;
        case TOKEN_TYPE_STRLIT:
            printf("String literal \"%s\" at %u:%u\n",(char*)token->value,token->row,token->col);
            break;
        case TOKEN_TYPE_CHRLIT:
            printf("Character literal \"%s\" at %u:%u\n",(char*)token->value,token->row,token->col);
            break;
        case TOKEN_TYPE_BANG:
            printf("Exclamation point at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_HASH:
            printf("Hashtag at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_DOLLAR:
            printf("Dollar sign at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_MODULO:
            printf("Percent sign at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_AND:
            printf("Ampersand at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_LPAREN:
            printf("Left paren at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_RPAREN:
            printf("Right paren at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_STAR:
            printf("Asterisk at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_PLUS:
            printf("Plus sing at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_COMMA:
            printf("Comma at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_MINUS:
            printf("Minus sign at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_DOT:
            printf("Dot at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_SLASH:
            printf("Slash at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_COLON:
            printf("Colon at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_SEMI:
            printf("Semicolon at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_LANGLE:
            printf("Left angle bracket at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_EQUAL:
            printf("Equals sign at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_RANGLE:
            printf("Right angle bracket at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_QUESTION:
            printf("Question mark at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_AT:
            printf("At sign at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_LSQUARE:
            printf("Left square bracket at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_BSLASH:
            printf("Backslash at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_RSQUARE:
            printf("Right square bracket at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_CARET:
            printf("Caret at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_BACKTICK:
            printf("Backtick at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_LCURLY:
            printf("Left curly brace at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_WALL:
            printf("Wall at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_RCURLY:
            printf("Right curly brace at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_TILDE:
            printf("Tilde at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_ANDAND:
            printf("Double ampersand at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_OROR:
            printf("Double wall at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_NE:
            printf("Negated equal sign at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_GTE:
            printf("Less-than-or-equal sign at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_LTE:
            printf("Greater-than-or-equal at %u:%u\n",token->row,token->col);
            break;
        case TOKEN_TYPE_DOTDOTDOT:
            printf("Elipses at %u:%u\n",token->row,token->col);
            break;
    }
}
