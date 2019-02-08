#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <libgen.h>
#include "../../includes/types.h"
#include "../../includes/main.h"
#include "../../includes/lexer.h"

#define SIZE_OF_FILE_ARRAY      100
#define FILE_STACK_SIZE         32
#define INCLUDE_CYCLE           -2

static char *add_file(const char* file, lexer_state_t *state);

static int process_token(lexer_state_t *state, lexeme_t *token);

static int fill_state(lexer_state_t *state, char *file);

lexer_state_t *lexical_analysis(int num_files, char** files)
{
    lexer_state_t *state;
    FILE* file;
    int i;
    
    //first token
    state = (lexer_state_t*) calloc(num_files, sizeof(lexer_state_t));

    if(!state)
    {
        fprintf(stderr, "failed to allocated memory");
        return NULL;
    }


    for(i = 0; i < num_files; i++)
    {
        file = fopen(files[i], "r");
                
        if(!file)
        {
            fprintf(stderr, "Error opening file %s:\n %s\n", files[i], strerror(errno));
            continue;
        }

        yyline = 1;
        
        yyrestart(file);
        state[i].file_stack = (char**) calloc(FILE_STACK_SIZE, sizeof(char*));
        state[i].stack_size = FILE_STACK_SIZE;
        state[i].number_of_files = 0;
        state[i].def_map = hashmap_new();

        fill_state(state + i, files[i]);
        
        fclose(file);
        free(state[i].file_stack);
        state[i].stack_size = 0;
        state[i].stack_index = 0;
        state[i].cur = state[i].first;
    }
    
    return state;
}

static int fill_state(lexer_state_t *state, char *file)
{
    lexeme_t *curtok;
    char *file_copy, **temp;
    int i;
    
    file_copy = add_file(file, state);

    if(file_copy == NULL)
        return -1;

    
    if(state->file_stack[state->stack_size - 1] != NULL)
    {
        temp = realloc(state->file_stack, FILE_STACK_SIZE + state->stack_size); 
        if(!temp)
        {
            fprintf(stderr, "failed to allocate memory");
            return -1;
        }
        state->file_stack = temp;
        state->stack_size += FILE_STACK_SIZE; 
    }

    for(i = 0; i < state->stack_size; i++)
    {
        if(state->file_stack[i] == NULL)
        {
            state->file_stack[i] = file_copy;
            break;
        }
        if(!strcmp(file_copy, state->file_stack[i]))
        {
            return INCLUDE_CYCLE;
        }
    }

    curtok = (lexeme_t*) calloc(1, sizeof(lexeme_t));
    if(!curtok)
    {
        fprintf(stderr, "fatal error: could not allocate memory\n");
        return -1;
    }
    curtok->token = yylex();

    while(curtok->token)
    {  
        curtok->line_number = yyline;
        curtok->filename = file_copy;
       
        if(!process_token(state, curtok))
        {

            curtok->next = state->cur;
            if(state->cur)
                state->cur->prev = curtok;
            state->cur = curtok;
            if(!state->first)
                state->first = curtok;
            curtok = (lexeme_t*)calloc(1, sizeof(lexeme_t));
            if(!curtok)
            {
                fprintf(stderr, "fatal error: could not allocate memory\n");
                return -1;
            }
        }
        curtok->token = yylex();
    }
    state->file_stack[i] = NULL;
    free(curtok);
    return 0;
}

static int process_token(lexer_state_t *state, lexeme_t *token)
{
    char token_name[20], *dup;
    int line, token_num, length, i;
    FILE *file;
    YY_BUFFER_STATE lex_buff;

    switch(token->token)
    {
        case STRCONST_TOKEN:
            dup = (char*) malloc(sizeof(char)*strlen(yytext));
            if(!dup)
            {
                fprintf(stderr, "failed to allocate memory");
            }
            token->value = dup;
            break;
        case INTCONST_TOKEN:
            token->value = (void*) malloc(sizeof(int));
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory");
            }
            *(int*)(token->value) = atoi(yytext);
            break;
        case HEXCONST_TOKEN:
            token->value = malloc(sizeof(int));
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory");
            }
            *(int*)(token->value) = strtol(yytext, NULL, 0);
            break;
        case REALCONST_TOKEN:
            token->value = (void*) malloc(sizeof(float));
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory");
            }
            *(float*)(token->value) = strtof(yytext, NULL);
            break;
        case CHARCONST_TOKEN:
            token->value = (void*) malloc(4);
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory");
            }
            strcpy(token->value, yytext);
            break;
        case INCLUDE_TOKEN:
            line = yyline;
            yyline = 1;
            token_num = yylex();
            if(token_num != STRCONST_TOKEN && token_num != INCLUDE_FILE_TOKEN && token_num != NEWLINE_TOKEN)
            {
                fprintf(stderr, "invalid import on line %d: %s\n", line, yytext);
                return -1;
            }
            else if(toke_num == NEWLINE_TOKEN)
            {
                fprintf(stderr, "invalid import on line %s: no file provided", line);
                return -1;
            }
            else if(token_num == STRCONST_TOKEN)
            {
                length = strlen(yytext);
                dup = (char*) calloc(1, sizeof(char) * (strlen(token->filename)+length));
                dup = strcpy(dup, token->filename);
                dirname(dup);
                strcat(dup, "/");
            }
            else
            {
                fprintf(stderr, "not supporting includes from system files: %s\n", yytext);
                return -1;
                //length = strlen(yytext);
                //dup = (char*) calloc(1 , sizeof(char) * (length + 13));
                //dup = strcpy(dup, "/usr/include/");
            }
            dup = strncat(dup, yytext+1, length-2);
            file = fopen(dup, "r");
            if(!file)
            {
                fprintf(stderr, "include failure: failed to open file %s\n%s\n", dup, strerror(errno));
                free(dup);
                yyline = line;
                return -1;
            }
            lex_buff = yy_create_buffer(file, YY_BUF_SIZE);
            yypush_buffer_state(lex_buff);
            if(fill_state(state, dup) == INCLUDE_CYCLE)
            {
                fprintf(stderr, "include cycle detected %s, line %d", token->filename, token->line_number);
            }
            yypop_buffer_state();
            yyline = line;
            fclose(file);
            free(dup);
            return -1;
        case DEFINE_TOKEN:
            return -1;
        case UNDEF_TOKEN:
            return -1;
        case ENDIF_TOKEN:
            return -1;
        case ELIF_TOKEN:
            return -1;
        case IFDEF_TOKEN:
            return -1;
        case IFNDEF_TOKEN:
            return -1;
        case ElSE_DIREC_TOKEN:
            return -1;
        case NEWLINE_TOKEN:
            return -1;
    }
    if(program_options & LEXER_DEBUG_OPTION ) 
    {
        tok_to_str(token_name, token->token);
        printf("File %s Line %d Token %s Text '%s'\n", 
            token->filename, token->line_number, token_name, yytext
        );
    }
    return 0;
}

static char *add_file(const char* file, lexer_state_t *state)
{
    char **temp;
    char *dup;

    if(state->number_of_files % SIZE_OF_FILE_ARRAY == 0)
    {
        temp = realloc( state->file_strings, sizeof(char*) * (state->number_of_files + SIZE_OF_FILE_ARRAY));
        if(temp == NULL)
        {
            fprintf(stderr, "failed to allocate memory.\n");
            return NULL;
        }
        state->file_strings = temp;
    }

    dup = (char*) malloc(sizeof(char*) * strlen(file));

    if(dup == NULL)
    {
        fprintf(stderr, "failed to allocate memory.\n");
        return NULL;
    }
    
    strcpy(dup, file);

    state->file_strings[state->number_of_files] = dup;
    state->number_of_files++;
    return dup;
}

void clean_lexer(lexer_state_t *state)
{
    int i;
    lexeme_t *cur, *next;

    for(i = 0; i < state->number_of_files; i++)
    {
        free(state->file_strings[i]);
    }

    free(state->file_strings);
    
    cur = state->first;
    next = cur->prev;
    while(cur)
    {
        if(cur->value)
            free(cur->value);
        next = cur->prev;
        free(cur);
        cur = next;
    }
    free(state);
}

void tok_to_str(char* buff, int token)
{
    switch(token)
    {
        case INCLUDE_TOKEN:
            strcpy(buff, "INCLUDE");
            break;
        case DEFINE_TOKEN:
            strcpy(buff, "DEFINE");
            break;
        case UNDEF_TOKEN:
            strcpy(buff, "UNDEF");
            break;
        case TYPE_TOKEN:
            strcpy(buff, "TYPE");
            break;
        case FOR_TOKEN:
            strcpy(buff, "FOR");
            break;
        case WHILE_TOKEN:
            strcpy(buff, "WHILE");
            break;
        case DO_TOKEN:
            strcpy(buff, "DO");
            break;
        case IF_TOKEN:
            strcpy(buff, "IF");
            break;
        case ELSE_TOKEN:
            strcpy(buff, "ELSE");
            break;
        case BREAK_TOKEN:
            strcpy(buff, "BREAK");
            break;
        case CONTINUE_TOKEN:
            strcpy(buff, "CONTINUE");
            break;
        case RETURN_TOKEN:
            strcpy(buff, "RETURN");
            break;
        case IDENT_TOKEN:
            strcpy(buff, "IDENT");
            break;
        case INTCONST_TOKEN:
            strcpy(buff, "INTCONST");
            break;
        case HEXCONST_TOKEN:
            strcpy(buff, "HEXCONST");
            break;
        case STRCONST_TOKEN:
            strcpy(buff, "STRCONST");
            break;
        case CHARCONST_TOKEN:
            strcpy(buff, "CHARCONST");
            break;
        case INCLUDE_FILE_TOKEN:
            strcpy(buff, "INCLUDE_FILE");
            break;
        case LPAR_TOKEN:
            strcpy(buff, "LPAR");
            break;
        case RPAR_TOKEN:
            strcpy(buff, "RPAR");
            break;
        case LBRACKET_TOKEN:
            strcpy(buff, "LBRACKET");
            break;
        case RBRACKET_TOKEN:
            strcpy(buff, "RBRACKET");
            break;
        case LBRACE_TOKEN:
            strcpy(buff, "LBRACE");
            break;
        case RBRACE_TOKEN:
            strcpy(buff, "RBRACE");
            break;
        case COMMA_TOKEN:
            strcpy(buff, "COMMA");
            break;
        case SEMI_TOKEN:
            strcpy(buff, "SEMI");
            break;
        case QUEST_TOKEN:
            strcpy(buff, "QUEST");
            break;
        case COLON_TOKEN:
            strcpy(buff, "COLON");
            break;
        case EQUAL_TOKEN:
            strcpy(buff, "EQUAL");
            break;
        case NEQUAL_TOKEN:
            strcpy(buff, "NEQUAL");
            break;
        case GT_TOKEN:
            strcpy(buff, "GT");
            break;
        case GE_TOKEN:
            strcpy(buff, "GE");
            break;
        case LT_TOKEN:
            strcpy(buff, "LT");
            break;
        case LE_TOKEN:
            strcpy(buff, "LE");
            break;
        case PLUS_TOKEN:
            strcpy(buff, "PLUS");
            break;
        case MINUS_TOKEN:
            strcpy(buff, "MINUS");
            break;
        case STAR_TOKEN:
            strcpy(buff, "STAR");
            break;
        case SLASH_TOKEN:
            strcpy(buff, "SLASH");
            break;
        case MOD_TOKEN:
            strcpy(buff, "MOD");
            break;
        case TILDE_TOKEN:
            strcpy(buff, "TILDE");
            break;
        case PIPE_TOKEN:
            strcpy(buff, "PIPE");
            break;
        case BANG_TOKEN:
            strcpy(buff, "BANG");
            break;
        case AMP_TOKEN:
            strcpy(buff, "AMP");
            break;
        case DAMP_TOKEN:
            strcpy(buff, "DAMP");
            break;
        case DPIPE_TOKEN:
            strcpy(buff, "DPIPE");
            break;
        case ASSIGN_TOKEN:
            strcpy(buff, "ASSIGN");
            break;
        case PLUSASSIGN_TOKEN:
            strcpy(buff, "PLUSASSIGN");
            break;
        case MINUSASSIGN_TOKEN:
            strcpy(buff, "MINUSASSIGN");
            break;
        case STARASSIGN_TOKEN:
            strcpy(buff, "STARASSIGN");
            break;
        case SLASHASSIGN_TOKEN:
            strcpy(buff, "SLASHASSIGN");
            break;
        case INCR_TOKEN:
            strcpy(buff, "INCR");
            break;
        case DECR_TOKEN:
            strcpy(buff, "DECR");
            break;
        case UNKNOWN_TOKEN:
            strcpy(buff, "UNKNOWN");
            break;
        case REALCONST_TOKEN:
            strcpy(buff, "REALCONST");
            break;
        case IFNDEF_TOKEN:
            strcpy(buff, "IFNDEF");
            break;
        case IFDEF_TOKEN: 
            strcpy(buff, "IFDEF");
            break;
        case ENDIF_TOKEN:
            strcpy(buff, "ENDIF");
            break;
        case ELIF_TOKEN:
            strcpy(buff, "ELIF");
            break;
        case ElSE_DIREC_TOKEN:
            strcpy(buff, "ELSE_DIRECTIVE");
            break;
    }
}
