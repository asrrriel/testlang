#include "peeker.h"
#include <stddef.h>
#include <stdio.h>
#include "lexer/lexer.h"

//HACK: peeker not thread safe
token_t* cur;

void set_token_source(token_t* tokens){
    cur = tokens;
}

token_t* peek(int32_t lookahead){
    return cur + lookahead;
}

token_t* consume(int32_t amt){
    return (cur+=amt)-amt; // theres no post-increment += :pensive:
}

token_t* expect(token_type_t type){
    if(cur->type != type){
        return NULL;
    }

    return cur;
}

token_t* expect_d(token_type_t type){
    if(cur->type != type){
        return NULL;
    }

    return cur++;
}

token_t* get_next_us(token_type_t type,token_t* until){
    int32_t lookahead = 0;
    token_t* next = 0;

    do{
        next = peek(lookahead);
        if(next >= until){
            return NULL;
        }
        lookahead++;
    } while(next->type != type);

    return next;
}
token_t* get_prev_us(token_type_t type,token_t* until){
    int32_t lookahead = 0;
    token_t* next = 0;

    do{
        next = peek(lookahead);
        if(next <= until){
            return NULL;
        }
        lookahead--;
    } while(next->type != type);

    return next;
}

token_t* get_next_uspa(token_type_t type,token_t* until){
    int32_t lookahead = 0;
    int32_t p = 0;
    token_t* next = 0;

    do{
        next = peek(lookahead);

        if (next && (next->type == TOKEN_TYPE_LPAREN || next->type == TOKEN_TYPE_QUESTION)){
            p++;
        } else if (next && (next->type == TOKEN_TYPE_RPAREN || next->type == TOKEN_TYPE_COLON) && p > 0) {
            p--;
        }


        if(next >= until){
            return NULL;
        }
        lookahead++;
    } while(next->type != type || p != 0);

    return next;
}
token_t* get_prev_uspa(token_type_t type,token_t* until){
    int32_t lookahead = 0;
    int32_t p = 0;
    token_t* next = 0;

    do{
        next = peek(lookahead);

        if (next->type == TOKEN_TYPE_LPAREN){
            p++;
        } else if (next->type == TOKEN_TYPE_RPAREN) {
            p--;
        }

        if(next <= until){
            return NULL;
        }
        lookahead--;
    } while(next->type != type || p != 0);

    return next;
}

token_t* get_next(token_type_t type,token_type_t until){
    int32_t lookahead = 0;
    token_t* next = 0;

    do{
        next = peek(lookahead);
        if(next->type == until){
            return NULL;
        }
        lookahead++;
    } while(next->type != type);

    return next;
}

token_t* get_next_pa(token_type_t type,token_type_t until){
    int32_t lookahead = 0;
    int32_t p = 0;
    token_t* next = 0;

    do{
        next = peek(lookahead);

        if (next && (next->type == TOKEN_TYPE_LPAREN || next->type == TOKEN_TYPE_QUESTION)){
            p++;
        } else if (next && (next->type == TOKEN_TYPE_RPAREN || next->type == TOKEN_TYPE_COLON) && p > 0) {
            p--;
        }

        if(next->type == until){
            return NULL;
        }
        lookahead++;
    } while(next->type != type || p != 0);

    return next;
}


void set_ptr(token_t* ptr){
    cur = ptr;
}
