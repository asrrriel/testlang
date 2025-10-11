#ifndef __SRCFILE_H__
#define __SRCFILE_H__

#include <lexer/lexer.h>
#include <parser/ast.h>

typedef struct {
    char* path;
    char* content;
    token_t* tokens;
    ast_node_t* ast;

    size_t num_err;
} src_file_t;

#endif // __SRCFILE_H__
