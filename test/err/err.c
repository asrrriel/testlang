#include "err.h"
#include "util/srcfile.h"
#include <stdlib.h>
#include <stdio.h>

#define SEVERITY_NOTE 0
#define SEVERITY_WARN 1
#define SEVERITY_ERR  2

static const char* issue_labels[] = {
    [COMP_ERR_UNIMPLEMENTED] = "Unimplemented feature",
    [COMP_ERR_INTERNAL_FAILIURE] = "Internal failiure",
    [COMP_ERR_IS_DIR] = "Is a directory",
    [COMP_ERR_CANT_OPEN] = "Couldn't open file",
    [COMP_WARN_EMPTY] = "File is empty",
    [COMP_ERR_DUPLICATE_QUALIFIER] = "Duplicate qualifier",
    [COMP_ERR_MISSING_SEMICOLON] = "Missing semicolon",
    [COMP_ERR_EXPECTED_ARG_LIST] = "Expected argument list",
    [COMP_ERR_DECLARATION_CUT_OFF] = "Incomplete declaration",
    [COMP_ERR_EMPTY_CHRLIT] = "Empty character literal",
    [COMP_ERR_MULTIBYTE_CHRLIT] = "Multibyte character doesn't fit in a standard char",
    [COMP_ERR_DANGLING_POSTFIX] = "Dangling postfix",
    [COMP_ERR_MISSING_LPAREN] = "Missing opening parenthesis",
    [COMP_ERR_MISSING_RPAREN] = "Missing closing parenthesis",
    [COMP_ERR_EXPECTED_EXPR] = "Expected valid expression",
    [COMP_ERR_EXPECTED_STMT] = "Expected valid statement",
    [COMP_ERR_EXPECTED_IDENT] = "Expected identifier",
    [COMP_ERR_TYPE_MISMATCH] = "Type mismatch",
    [COMP_ERR_UNDEFINED_IDENT] = "Undefinded identifier",
};

static const uint8_t issue_severities[] = {
    [COMP_ERR_UNIMPLEMENTED] = SEVERITY_ERR,
    [COMP_ERR_INTERNAL_FAILIURE] = SEVERITY_ERR,
    [COMP_ERR_IS_DIR] = SEVERITY_ERR,
    [COMP_ERR_CANT_OPEN] = SEVERITY_ERR,
    [COMP_WARN_EMPTY] = SEVERITY_WARN,
    [COMP_ERR_DUPLICATE_QUALIFIER] = SEVERITY_ERR,
    [COMP_ERR_MISSING_SEMICOLON] = SEVERITY_ERR,
    [COMP_ERR_EXPECTED_ARG_LIST] = SEVERITY_ERR,
    [COMP_ERR_DECLARATION_CUT_OFF] = SEVERITY_ERR,
    [COMP_ERR_EMPTY_CHRLIT] = SEVERITY_ERR,
    [COMP_ERR_MULTIBYTE_CHRLIT] = SEVERITY_ERR,
    [COMP_ERR_DANGLING_POSTFIX] = SEVERITY_ERR,
    [COMP_ERR_MISSING_LPAREN] = SEVERITY_ERR,
    [COMP_ERR_MISSING_RPAREN] = SEVERITY_ERR,
    [COMP_ERR_EXPECTED_EXPR] = SEVERITY_ERR,
    [COMP_ERR_EXPECTED_STMT] = SEVERITY_ERR,
    [COMP_ERR_EXPECTED_IDENT] = SEVERITY_ERR,
    [COMP_ERR_TYPE_MISMATCH] = SEVERITY_ERR,
    [COMP_ERR_UNDEFINED_IDENT] = SEVERITY_ERR,
};

static const char* severity_labels[] = {
    [SEVERITY_NOTE] = "[NOTE]",
    [SEVERITY_WARN] = "[WARN]",
    [SEVERITY_ERR ] = "[ERR ]",
};


void throw_noncode_issue(src_file_t file,comp_issue_t issue,bool fatal){
    printf("%s while compiling \"%s\": %s\n",
        severity_labels[issue_severities[issue]],
        file.path,
        issue_labels[issue]);

    if(fatal){
        exit(1);
    }
}

void throw_code_issue(src_file_t file,comp_issue_t issue,token_t highlight,bool fatal){

    printf("%s at %s:%u:%u: %s\n",
        severity_labels[issue_severities[issue]],
        file.path,
        highlight.row,
        highlight.col,
        issue_labels[issue]);

    if(fatal){
        exit(1);
    }
}

