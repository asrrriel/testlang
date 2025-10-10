#ifndef __PEEKER_H__
#define __PEEKER_H__

#include <lexer/lexer.h>

void set_token_source(token_t* tokens);

token_t* peek();
token_t* consume();
token_t* expect(token_type_t type);

#endif // __PEEKER_H__
