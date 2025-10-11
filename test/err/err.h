#ifndef __ERR_H__
#define __ERR_H__

#include "lexer/lexer.h"
#include "util/srcfile.h"

typedef enum {
    COMP_ERR_UNIMPLEMENTED,
    COMP_ERR_INTERNAL_FAILIURE,
    COMP_ERR_IS_DIR,
    COMP_ERR_CANT_OPEN,
    COMP_WARN_EMPTY,
    COMP_ERR_DUPLICATE_QUALIFIER,
    COMP_ERR_MISSING_SEMICOLON,
    COMP_ERR_EXPECTED_ARG_LIST,
    COMP_ERR_DECLARATION_CUT_OFF,

} comp_issue_t;

void throw_noncode_issue(src_file_t file,comp_issue_t issue,bool fatal);
void throw_code_issue(src_file_t file,comp_issue_t issue,token_t highlight,bool fatal);



#endif // __ERR_H__
