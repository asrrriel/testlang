#include "err.h"
#include "util/srcfile.h"
#include <stdlib.h>
#include <stdio.h>

#define SEVERITY_NOTE 0
#define SEVERITY_WARN 1
#define SEVERITY_ERR  2

static const char* issue_labels[] = {
    [COMP_ERR_INTERNAL_FAILIURE] = "Internal failiure",
    [COMP_ERR_IS_DIR] = "Is a directory",
    [COMP_ERR_CANT_OPEN] = "Couldn't open file",
    [COMP_WARN_EMPTY] = "File is empty"
};

static const uint8_t issue_severities[] = {
    [COMP_ERR_INTERNAL_FAILIURE] = SEVERITY_ERR,
    [COMP_ERR_IS_DIR] = SEVERITY_ERR,
    [COMP_ERR_CANT_OPEN] = SEVERITY_ERR,
    [COMP_WARN_EMPTY] = SEVERITY_WARN
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

    printf("%s at %s:%u: %s\n",
        severity_labels[issue_severities[issue]],
        file.path,
        highlight.row,
        issue_labels[issue]);

    if(fatal){
        exit(1);
    }
}

