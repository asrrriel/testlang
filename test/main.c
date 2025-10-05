#include <stdio.h>
#include <stdlib.h>
#include <lexer/lexer.h>

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

    do {

        switch (lexed->type) {
            case TOKEN_TYPE_TERMINATOR:
                break;
            case TOKEN_TYPE_NEWLINE:
                printf("Newline at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_STRING:
                printf("String \"%s\" at %u:%u\n",(char*)lexed->value,lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_STRLIT:
                printf("String literal \"%s\" at %u:%u\n",(char*)lexed->value,lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_CHRLIT:
                printf("Character literal \"%s\" at %u:%u\n",(char*)lexed->value,lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_BANG:
                printf("Exclamation point at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_HASH:
                printf("Hashtag at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_DOLLAR:
                printf("Dollar sign at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_MODULO:
                printf("Percent sign at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_AND:
                printf("Ampersand at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_LPAREN:
                printf("Left paren at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_RPAREN:
                printf("Right paren at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_STAR:
                printf("Asterisk at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_PLUS:
                printf("Plus sing at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_COMMA:
                printf("Comma at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_MINUS:
                printf("Minus sign at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_DOT:
                printf("Dot at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_SLASH:
                printf("Slash at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_COLON:
                printf("Colon at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_SEMI:
                printf("Semicolon at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_LANGLE:
                printf("Left angle bracket at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_EQUAL:
                printf("Equals sign at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_RANGLE:
                printf("Right angle bracket at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_QUESTION:
                printf("Question mark at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_AT:
                printf("At sign at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_LSQUARE:
                printf("Left square bracket at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_BSLASH:
                printf("Backslash at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_RSQUARE:
                printf("Right square bracket at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_CARET:
                printf("Caret at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_BACKTICK:
                printf("Backtick at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_LCURLY:
                printf("Left curly brace at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_WALL:
                printf("Wall at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_RCURLY:
                printf("Right curly brace at %u:%u\n",lexed->row,lexed->col);
                break;
            case TOKEN_TYPE_TILDE:
                printf("Tilda at %u:%u\n",lexed->row,lexed->col);
                break;
        }


        if(lexed->type != TOKEN_TYPE_TERMINATOR)
            lexed++;

    } while(lexed->type != TOKEN_TYPE_TERMINATOR);
    
    return 0;
}