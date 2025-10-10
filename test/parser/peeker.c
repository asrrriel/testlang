#include "peeker.h"
#include <stddef.h>
#include "lexer/lexer.h"

token_t* cur;

void set_token_source(token_t* tokens){
    cur = tokens;
}

token_t* peek(){
    return cur;
}

token_t* consume(){
    return cur++;
}

token_t* expect(token_type_t type){
    if(cur->type != type){
        return NULL;
    }

    return cur;
}
