#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../includes/main.h"
#include "../../includes/lexer.h"
#include "../../includes/parser.h"

//characters needed ofr on the parse args function
#define LEXER           'l'
#define PARSER          'p'
#define TYPE            't'
#define INTERMEDIATE    'i'
#define COMPILE         'c'
#define OPTION_FLAG     '-'

static int parse_args(int argc, char** argv);

//source files that need to be 'compiled'
static char** file_list;
//number of files in list
static int files = 0;

//bit field for current program options
uint64_t program_options = INITIAL_OPTION;

/*
 * main entry point for program
 */ 
int main(int argc, char** argv)
{
    int i;
    lexer_state_t *lexer;

    //temp to meet assigment 1 specs
    program_options = program_options | LEXER_DEBUG_OPTION | LEXER_SAVE_OPTION;

    file_list = (char**)malloc(sizeof(argc) * (argc-1));

    if(file_list == NULL) 
    {
        fprintf(stderr, "CRITICAL FAILURE: unable to allocate memory\n");
        return -1;
    }

    //call to interperate the command line arguments 
    if(parse_args(argc, argv))
        return -1;
    //run lexer on the files if lexer option is set 
    if(program_options & LEXER_OPTION)
    {
        lexer = lexical_analysis(files, file_list);
        if(!lexer)
        {
            fprintf(stderr, "failed to parse input\n");
            return -2;
        }
    }
    //run parser
    if(program_options & PARSER_OPTION)
    {
        if(parse_input(files, file_list))
            return -3;
    }
    //run type analysis
    if(program_options & TYPE_OPTION)
    {
        fprintf(stderr, "failed to analyze types\n");
        return -4;
    }
    //create intermediate code
    if(program_options & INTERMEDIATE_OPTION)
    {
        fprintf(stderr, "failed to generate intermediate code\n");
        return -6;
    }
    //compile to target code
    if(program_options & COMPILE_OPTION)
    {
        fprintf(stderr, "failed to compile input\n");
        return -7;
    }

    if(lexer)
    {
        for(i = 0; i < files; i++)
        {
            clean_lexer(lexer + i);
        }

        free(lexer);
    }

    //clean up after file list
    free(file_list);

    return 0;
}

static int parse_args(int argc, char** argv)
{
    int i, main_option_set = 0;
    char cur;

    for(i = 1; i < argc; i++)
    {
        //check if first character is option flag
        if(*argv[i] == OPTION_FLAG)
        {   
            //set the current character
            //we know its safe to check 1 because 0was '-' 
            //and it must be null terminated
            cur = argv[i][1];
            switch(cur)
            {
                case COMPILE:
                    program_options = program_options | COMPILE_OPTION;
                case INTERMEDIATE:
                    program_options = program_options | INTERMEDIATE_OPTION;
                case TYPE:
                    program_options = program_options | TYPE_OPTION;
                case PARSER:
                    program_options = program_options | PARSER_OPTION;
                    if(main_option_set)
                    {
                        fprintf(
                            stderr, 
                            "only one of %c %c %c %c %c can be selected\n", 
                            LEXER, 
                            PARSER, 
                            TYPE, 
                            INTERMEDIATE, 
                            COMPILE
                        );
                        return -1;
                    }
                    main_option_set = 1;
                    break;
                case LEXER:
                    program_options = program_options | LEXER_OPTION;
                    if(main_option_set)
                    {
                        fprintf(
                            stderr, 
                            "only one of %c %c %c %c %c can be selected\n", 
                            LEXER, 
                            PARSER, 
                            TYPE, 
                            INTERMEDIATE, 
                            COMPILE
                        );
                        return -1;
                    }
                    main_option_set = 1;
                    break;
                case OPTION_FLAG:
                    if(!strcmp(argv[i], "--debug-lexer"))
                    {
                        program_options = program_options | LEXER_DEBUG_OPTION;
                    }
                    else if(!strcmp(argv[i], "--lexer-save"))
                    {
                        program_options = program_options | LEXER_SAVE_OPTION; 
                    } 
                    else if(!strcmp(argv[i], "--debug-parser"))
                    {
                        program_options = program_options | PARSER_DEBUG_OPTION;
                    }
                    break;
                default:
                    fprintf(stderr, "unrecognized option: %s, %c\n", argv[i], cur);
                    return -1;
                    break;
            }
        }
        else
        {
            if(!main_option_set)
            {
                fprintf(
                    stderr, 
                    "must select a run option first: %c, %c, %c, %c, %c\n",
                    LEXER, 
                    PARSER, 
                    TYPE, 
                    INTERMEDIATE, 
                    COMPILE
                );
                return -1;
            }
            file_list[files] = argv[i]; 
            files++;
        }
    }

    if(!main_option_set)
    {
        fprintf(
            stderr, 
            "must select a run option first: %c, %c, %c, %c, %c\n",
            LEXER, 
            PARSER, 
            TYPE, 
            INTERMEDIATE, 
            COMPILE
        );
        return -1;
    }

    if(!files)
    {
        fprintf(stderr, "no input files program terminating\n");
        return -1;
    }
    return 0;
}
