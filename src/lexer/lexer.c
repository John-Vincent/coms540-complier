#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <libgen.h>
#include "../../includes/types.h"
#include "../../includes/main.h"
#include "../../includes/lexer.h"

#define SIZE_OF_FILE_ARRAY 100

static char *add_file(const char* file);

static char **file_strings;
static int number_of_files = 0;
static lexeme_t *first;

lexeme_t *lexical_analysis(int num_files, char** files)
{
    lexeme_t *curtok, *lasttok;
    FILE* file;
    char *file_copy;
    int i;
    
    //first token
    curtok = first = (lexeme_t*)malloc(sizeof(lexeme_t));
    if(!curtok)
    {
        fprintf(stderr, "fatal error: could not allocate memory\n");
        return NULL;
    }

    for(i = 0; i < num_files; i++)
    {
        file_copy = add_file(files[i]);
        file = fopen(files[i], "r");
        
        if(!file)
        {
            fprintf(stderr, "Error opening file %s:\n %s\n", files[i], strerror(errno));
            continue;
        }
        
        yyrestart(file);

        curtok->token = yylex();

        while(curtok->token)
        {  
            curtok->line_number = yyline;
            curtok->filename = file_copy;
            if(program_options & LEXER_DEBUG_OPTION ) 
            {
                printf("File %s Line %d Token %s Text '%s'\n", 
                    curtok->filename, curtok->line_number, yytoken_name, yytext
                );
            }
            curtok->next = lasttok;
            lasttok = curtok;
            curtok = (lexeme_t*)malloc(sizeof(lexeme_t));
            if(!curtok)
            {
                fprintf(stderr, "fatal error: could not allocate memory\n");
                fclose(file);
                return NULL;
            }
            if(lasttok)
                lasttok->prev = curtok;
            curtok->token = yylex();
        }
        fclose(file);
    }
    
    lasttok->prev = NULL;
    free(curtok);
    return lasttok;
}

static char *add_file(const char* file)
{
    char **temp;
    char *dup;

    if(number_of_files % SIZE_OF_FILE_ARRAY == 0)
    {
        temp = realloc( file_strings, sizeof(char*) * (number_of_files + SIZE_OF_FILE_ARRAY));
        if(temp == NULL)
        {
            fprintf(stderr, "failed to allocate memory.\n");
            return NULL;
        }
        file_strings = temp;
    }

    dup = (char*) malloc(sizeof(char*) * strlen(file));

    if(dup == NULL)
    {
        fprintf(stderr, "failed to allocate memory.\n");
        return NULL;
    }
    
    strcpy(dup, file);

    file_strings[number_of_files] = dup;
    number_of_files++;
    return dup;
}

void clean_lexer()
{
    int i;
    lexeme_t *cur, *next;

    for(i = 0; i < number_of_files; i++)
    {
        free(file_strings[i]);
    }

    free(file_strings);
    
    cur = first;
    next = cur->prev;
    while(next)
    {
        free(cur);
        cur = next;
        next = cur->prev;
    }
    free(cur);
}

void tok_to_str(char* buff, int token)
{
    switch(token)
    {
        case INCLUDE_TOKEN:
            strcpy(yytoken_name, "INCLUDE");
            break;
        case DEFINE_TOKEN:
            strcpy(yytoken_name, "DEFINE");
            break;
        case TYPE_TOKEN:
            strcpy(yytoken_name, "TYPE");
            break;
        case FOR_TOKEN:
            strcpy(yytoken_name, "FOR");
            break;
        case WHILE_TOKEN:
            strcpy(yytoken_name, "WHILE");
            break;
        case DO_TOKEN:
            strcpy(yytoken_name, "DO");
            break;
        case IF_TOKEN:
            strcpy(yytoken_name, "IF");
            break;
        case ELSE_TOKEN:
            strcpy(yytoken_name, "ELSE");
            break;
        case BREAK_TOKEN:
            strcpy(yytoken_name, "BREAK");
            break;
        case CONTINUE_TOKEN:
            strcpy(yytoken_name, "CONTINUE");
            break;
        case RETURN_TOKEN:
            strcpy(yytoken_name, "RETURN");
            break;
        case IDENT_TOKEN:
            strcpy(yytoken_name, "IDENT");
            break;
        case INTCONST_TOKEN:
            strcpy(yytoken_name, "INTCONST");
            break;
        case STRCONST_TOKEN:
            strcpy(yytoken_name, "STRCONST");
            break;
        case CHARCONST_TOKEN:
            strcpy(yytoken_name, "CHARCONST");
            break;
        case INCLUDE_FILE_TOKEN:
            strcpy(yytoken_name, "INCLUDE_FILE");
            break;
        case LPAR_TOKEN:
            strcpy(yytoken_name, "LPAR");
            break;
        case RPAR_TOKEN:
            strcpy(yytoken_name, "RPAR");
            break;
        case LBRACKET_TOKEN:
            strcpy(yytoken_name, "LBRACKET");
            break;
        case RBRACKET_TOKEN:
            strcpy(yytoken_name, "RBRACKET");
            break;
        case LBRACE_TOKEN:
            strcpy(yytoken_name, "LBRACE");
            break;
        case RBRACE_TOKEN:
            strcpy(yytoken_name, "RBRACE");
            break;
        case COMMA_TOKEN:
            strcpy(yytoken_name, "COMMA");
            break;
        case SEMI_TOKEN:
            strcpy(yytoken_name, "SEMI");
            break;
        case QUEST_TOKEN:
            strcpy(yytoken_name, "QUEST");
            break;
        case COLON_TOKEN:
            strcpy(yytoken_name, "COLON");
            break;
        case EQUAL_TOKEN:
            strcpy(yytoken_name, "EQUAL");
            break;
        case NEQUAL_TOKEN:
            strcpy(yytoken_name, "NEQUAL");
            break;
        case GT_TOKEN:
            strcpy(yytoken_name, "GT");
            break;
        case GE_TOKEN:
            strcpy(yytoken_name, "GE");
            break;
        case LT_TOKEN:
            strcpy(yytoken_name, "LT");
            break;
        case LE_TOKEN:
            strcpy(yytoken_name, "LE");
            break;
        case PLUS_TOKEN:
            strcpy(yytoken_name, "PLUS");
            break;
        case MINUS_TOKEN:
            strcpy(yytoken_name, "MINUS");
            break;
        case STAR_TOKEN:
            strcpy(yytoken_name, "STAR");
            break;
        case SLASH_TOKEN:
            strcpy(yytoken_name, "SLASH");
            break;
        case MOD_TOKEN:
            strcpy(yytoken_name, "MOD");
            break;
        case TILDE_TOKEN:
            strcpy(yytoken_name, "TILDE");
            break;
        case PIPE_TOKEN:
            strcpy(yytoken_name, "PIPE");
            break;
        case BANG_TOKEN:
            strcpy(yytoken_name, "BANG");
            break;
        case AMP_TOKEN:
            strcpy(yytoken_name, "AMP");
            break;
        case DAMP_TOKEN:
            strcpy(yytoken_name, "DAMP");
            break;
        case DPIPE_TOKEN:
            strcpy(yytoken_name, "DPIPE");
            break;
        case ASSIGN_TOKEN:
            strcpy(yytoken_name, "ASSIGN");
            break;
        case PLUSASSIGN_TOKEN:
            strcpy(yytoken_name, "PLUSASSIGN");
            break;
        case MINUSASSIGN_TOKEN:
            strcpy(yytoken_name, "MINUSASSIGN");
            break;
        case STARASSIGN_TOKEN:
            strcpy(yytoken_name, "STARASSIGN");
            break;
        case SLASHASSIGN_TOKEN:
            strcpy(yytoken_name, "SLASHASSIGN");
            break;
        case INCR_TOKEN:
            strcpy(yytoken_name, "INCR");
            break;
        case DECR_TOKEN:
            strcpy(yytoken_name, "DECR");
            break;
        case UNKNOWN_TOKEN:
            strcpy(yytoken_name, "UNKNOWN");
            break;
        case REALCONST_TOKEN:
            strcpy(yytoken_name, "REALCONST");
            break;
    }
}
