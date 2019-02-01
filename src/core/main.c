#include <stdio.h>
#include <string.h>
#include "../../includes/types.h"
#include "../../include/main.h"
#include "../../includes/lexer.h"

//characters needed ofr on the parse args function
#define LEXER           'l'
#define PARSER          'p'
#define TYPE            't'
#define INTERMEDIATE    'i'
#define COMPILE         'c'
#define OPTION_FLAG     '-'

extern char* yytext;
extern int yyline;

//source files that need to be 'compiled'
char** file_list;
//number of files in list
int files = 0;

//bit field for current program options
int program_options = INVALID_STATE;
//error message that should be printed on exit.
char error_message[100]

/*
 * main entry point for program
 */ 
int main(int argc, char** argv)
{
    /* initialize the file path array
     * this will hold the argument strings that 
     * are determained to be source files to parse.
     * we will initialize it to be as long as the number
     * of arguments so it is at least big enough to
     * hold all files.
     */
    files = malloc(sizeof(argc) * (argc-1));
    if(files == NULL) 
    {
        fprintf(stderr, "CRITICAL FAILURE: unable to allocate memory");
        return -1;
    }

    //call to interperate the command line arguments 
    parse_args(argc, argv);
    int token;
    token = yylex();
    while(token)
    {   
        printf("File TODO Line %d Token %s Text '%s'\n", yyline, yytoken_name, yytext);
        token = yylex();
    }
    free(files);
    return 0;
}

void parse_args(int argc, char** argv)
{
    int i, j;
    size_t len;
    char cur;

    for(i = 1; i < argc; i++)
    {
        //check if first character is option flag
        if(*argv[i] == OPTION_FLAG)
        {   
            //set the current character
            //we know its safe to check 2 because one was '-' 
            //and it must be null terminated
            cur = argv[i][2];
            switch(cur)
            {
                case LEXER:
                    state = state | LEXER_STATE;
                    break;
                case PARSER:
                    state = state | PARSER_STATE;
                    break;
                case TYPE:
                    state = state | TYPE_STATE;
                    break;
                case INTERMEDIATE:
                    state = state | INTERMEDIATE_STATE;
                    break;
                case COMPILE:
                    state = state | COMPILE_STATE;
                    break;
                case OPTION_FLAG:
                default:
                    state = ERROR_STATE;
                    break;
            }
        }
        else
        {
            file_list[files] = argv[i]; 
            files++;
        }
    }
}
