#ifndef __PEEKER_H__
#define __PEEKER_H__

#include <lexer/lexer.h>
#include <stddef.h>

void set_token_source(token_t* tokens);

token_t* peek(size_t lookahead);
token_t* get_next(token_type_t type,token_type_t until);
token_t* consume();
token_t* expect(token_type_t type);
token_t* expect_d(token_type_t type);

#endif // __PEEKER_H__
