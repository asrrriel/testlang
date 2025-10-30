#ifndef __PEEKER_H__
#define __PEEKER_H__

#include <lexer/lexer.h>
#include <stddef.h>

//HACK: peeker not thread safe
void set_token_source(token_t* tokens);

token_t* peek(int32_t lookahead);
token_t* consume(int32_t amt);
token_t* expect(token_type_t type);
token_t* expect_d(token_type_t type);

token_t* get_next(token_type_t type,token_type_t until);

token_t* get_next_pa(token_type_t type,token_type_t until);

token_t* get_next_us(token_type_t type,token_t* until);
token_t* get_prev_us(token_type_t type,token_t* until);

token_t* get_next_uspa(token_type_t type,token_t* until);
token_t* get_prev_uspa(token_type_t type,token_t* until);

void set_ptr(token_t* ptr);


#endif // __PEEKER_H__
