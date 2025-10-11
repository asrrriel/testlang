#include <stdio.h>
#include <stdlib.h>
#include <lexer/lexer.h>
#include <parser/peeker.h>

int main(int argc, char** argv){
    if (argc < 2){
        printf("Usage: %s filename\n",argv[0]);
        return 1;
    }

    FILE* input = fopen(argv[1],"r");

    fseek(input,0,SEEK_END);
    size_t filesize = ftell(input);
    fseek(input,0,SEEK_SET);

    char* buffer = malloc(filesize+1);

    fread(buffer,filesize,1,input);

    buffer[filesize] = '\0';

    token_t* lexed = lex(buffer);

    if(!lexed){
        printf("failed to lexxxxxx\n");
        return 1;
    }

    set_token_source(lexed);

    print_token(peek(0));
    print_token(peek(0));
    print_token(consume());
    print_token(expect(TOKEN_TYPE_HASH));
    print_token(peek(0));
    print_token(peek(1));
    print_token(expect_d(TOKEN_TYPE_STRING));
    print_token(peek(0));
    print_token(get_next(TOKEN_TYPE_CHRLIT,TOKEN_TYPE_TERMINATOR));
    

    free(buffer);
    free_token_array(lexed);

    return 0;
}
