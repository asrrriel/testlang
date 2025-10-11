#include <stdio.h>
#include <stdlib.h>
#include <lexer/lexer.h>
#include <parser/peeker.h>
#include <util/srcfile.h>

src_file_t file;

src_file_t open_src(char* path){
    FILE* input = fopen(path,"r");
    if(!input){
        goto error;
    }

    fseek(input,0,SEEK_END);
    size_t filesize = ftell(input);
    if(filesize == 0){
        goto error;
    }
    fseek(input,0,SEEK_SET);

    char* buffer = malloc(filesize+1);

    if(!buffer){
        goto error;
    }

    fread(buffer,filesize,1,input);

    buffer[filesize] = '\0';

    fclose(input);
    return (src_file_t){
        .path = path,
        .content = buffer,
        .tokens = 0,
        .ast = 0,
        .num_err = 0
    };

    error:
        fclose(input);
        return (src_file_t){
            .path = path,
            .content = 0,
            .tokens = 0,
            .ast = 0,
            .num_err = 1
        };
}

void process_src(src_file_t* src){
    src->tokens = lex(src->content);

    if(!src->tokens){
        printf("ERROR! failed to lexxxxxx\n");
        exit(1);
    }

    set_token_source(src->tokens);

    print_token(peek(0));
    print_token(peek(0));
    print_token(consume());
    print_token(expect(TOKEN_TYPE_HASH));
    print_token(peek(0));
    print_token(peek(1));
    print_token(expect_d(TOKEN_TYPE_STRING));
    print_token(peek(0));
    print_token(get_next(TOKEN_TYPE_CHRLIT,TOKEN_TYPE_TERMINATOR));

}

void free_src(src_file_t* src){
    free(src->content);
    free_token_array(src->tokens);
}

int main(int argc, char** argv){
    if (argc < 2){
        printf("Usage: %s filename\n",argv[0]);
        return 1;
    }

    file = open_src(argv[1]);

    if(file.num_err != 0){
        printf("ERROR! failed to open file \"%s\"!\n",file.path);
        exit(1);
    }

    process_src(&file);

    free_src(&file);

    return 0;
}
