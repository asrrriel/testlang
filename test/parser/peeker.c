#include "peeker.h"
#include <stddef.h>
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

token_t* get_prev(token_type_t type,token_type_t until){
    int32_t lookahead = 0;
    token_t* next = 0;

    do{
        next = peek(lookahead);
        if(next->type == until){
            return NULL;
        }
        lookahead--;
    } while(next->type != type);

    return next;
}

void set_ptr(token_t* ptr){
    cur = ptr;
}
