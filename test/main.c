#include "sym_stuff/symchk.h"
#include <stdio.h>
#include <stdlib.h>
#include <lexer/lexer.h>
#include <parser/parser.h>
#include <sys/stat.h>
#include <util/srcfile.h>
#include <err/err.h>

src_file_t file;

src_file_t open_src(char* path){
    struct stat st;
    src_file_t toret = {
        .path = path,
        .content = 0,
        .tokens = 0,
        .ast = 0,
        .num_err = 0
    };
    
    if (stat(path, &st) != 0) {
        throw_noncode_issue(toret, COMP_ERR_INTERNAL_FAILIURE, true);
    }

    if(S_ISDIR(st.st_mode)){
        throw_noncode_issue(toret, COMP_ERR_IS_DIR, true);
    }
    
    FILE* input = fopen(path,"r");
    if(!input){
        throw_noncode_issue(toret, COMP_ERR_CANT_OPEN, true);
    }

    fseek(input,0,SEEK_END);
    size_t filesize = ftell(input);
    if(filesize == 0){
        throw_noncode_issue(toret, COMP_WARN_EMPTY, false);
    }
    fseek(input,0,SEEK_SET);

    toret.content = malloc(filesize+1);

    if(!toret.content){
        throw_noncode_issue(toret, COMP_ERR_INTERNAL_FAILIURE, true);
    }

    fread(toret.content,filesize,1,input);

    toret.content[filesize] = '\0';

    fclose(input);
    return toret;
}

void process_src(src_file_t* src){
    src->tokens = lex(src->content);

    if(!src->tokens){
        throw_noncode_issue(file, COMP_ERR_INTERNAL_FAILIURE, true);
    }

    //token_t* cur = src->tokens;
    //do {
    //    print_token(cur);
    //    if(cur->type != TOKEN_TYPE_TERMINATOR)
    //        cur++;
    //} while(cur->type != TOKEN_TYPE_TERMINATOR); 

    parse(&file);

    //print_ast_node(&file, file.ast, 0);
    populate_symtab(&file);
}

void free_src(src_file_t* src){
    free_ast_node(src->ast);
    free_token_array(src->tokens);
    free(src->symbols);
    free(src->content);
}

int main(int argc, char** argv){
    if (argc < 2){
        printf("Usage: %s filename\n",argv[0]);
        return 1;
    }

    file = open_src(argv[1]);

    process_src(&file);

    free_src(&file);

    return 0;
}
