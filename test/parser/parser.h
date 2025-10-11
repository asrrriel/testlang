#ifndef __PARSER_H__
#define __PARSER_H__

#include "parser/ast.h"
#include "util/srcfile.h"

void parse(src_file_t* file);

void print_ast_node(ast_node_t* node,size_t indent);
void free_ast_node(ast_node_t* node);

#endif // __PARSER_H__
