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
#define DEFINE_LIST_SIZE        20
#define INCLUDE_CYCLE           -2

/**
 * makes a copy of a file string
 * and adds that copy to the array of file
 * strings stored in the lexer state. 
 * this allows me to have a filename reference
 * that i know won't be freed until the lexer state
 * itself is freed.
 */
static char *add_file(const char* file, lexer_state_t *state);

/**
 * this function takes a state and a token and determains what, if any,
 * special actions need to be taken to accept that token
 * this function also prints the token information if in lexer debug mode.
 * This function will interpret includes and call off to fill state once the file to
 * be included is opend and set up with lex.
 * returns 0 if the lexeme should be added to the state, and -1 if it is a directive
 * or special case that should be ignored.
 */
static int process_token(lexer_state_t *state, lexeme_t *token);

/**
 * This function takes a state and adds the following definition
 * indetifier and value to the definition hashmap on the state.
 */
static int add_definition(lexer_state_t *state);

/**
 * This function adds a lexeme to the front of the lexer state
 * lexeme linked list.
 */
static int add_lexeme(lexer_state_t *state, lexeme_t **lexeme);

/**
 * This function takes an identifier definition maping and places its values 
 * onto the lexer state linked list. If in lexer debug mode it also prints these
 * lexemes as it replaces the identifier. It will handled nested defined identifiers itself.
 */
static void resolve_ident(lexer_state_t *state, def_map_t *map, char* filename);

/**
 *  This function handles ifdef and ifndef directives, as well as 
 *  the subsiquent #else #elif and #endif
 */
static int handle_ifdef(lexer_state_t *state, int ndef, char* file);

/**
 *  This function is called after lex is set up to read a new file
 *  This functin will add the filename to the file list in the state
 *  then start processing and adding lexemes to the state.
 */
static int fill_state(lexer_state_t *state, char *file);

/**
 *  This function is meant to be passed to the iterate function
 *  of the definition hashmap. It will take care of freeing all the
 *  allocated memory in the definition maps stored in the hashmap.
 */
static int clean_def_map(void *nothing, void *map);

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
        if(!state[i].file_stack)
        {
            fprintf(stderr, "failed to allocate memory\n");
            fclose(file);
            continue;
        }
        state[i].stack_size = FILE_STACK_SIZE;
        state[i].number_of_files = 0;
        state[i].def_map = hashmap_new();

        fill_state(state + i, files[i]);
        
        fclose(file);
        yypop_buffer_state();
        free(state[i].file_stack);
        state[i].stack_size = 0;
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

    state->cur_file = file_copy;

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
            add_lexeme(state, &curtok);
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

static int add_definition(lexer_state_t *state)
{
    int token, i, j, size;
    char *dup;
    lexeme_t *list, *temp;
    def_map_t *map;

    token = yylex();
    if(token != IDENT_TOKEN)
    {
        fprintf(stderr, "expected identifier for definition but got %s\nignoring definition\n", yytext);
        while(token != NEWLINE_TOKEN)
            token = yylex();
        return -2;
    }
    //adding two because i keep getting invalid writes from valgrind when i don't
    dup = (char*) calloc(1, strlen(yytext)+2);
    if(!dup)
    {
        fprintf(stderr, "failed to allocate memory");
        return -1;
    }
    dup = strcpy(dup, yytext);
    size = DEFINE_LIST_SIZE;
    list = (lexeme_t*) malloc(sizeof(lexeme_t) * size);
    if(!list)
    {
        fprintf(stderr, "failed to allocate memory");
        return -1;
    }
    list->prev = NULL;
    list->next = NULL;
    i = 0;
    token = yylex();
    while(token != NEWLINE_TOKEN)
    {
        list[i].token = token;
        list[i].filename = NULL;
        list[i].value = NULL;
        list[i].line_number = yyline;
        if( token == DEFINE_TOKEN ||
            token == UNDEF_TOKEN ||
            token == IFNDEF_TOKEN ||
            token == ENDIF_TOKEN ||
            token == ELSE_DIREC_TOKEN ||
            token == INCLUDE_TOKEN)
        {
            fprintf(stderr, "directives in define are not supported: %s, %s:%d\n", yytext, state->cur_file, yyline);
        }
        if(!process_token(state, list+i))
        {
            i++;
            if(i >= size-1)
            {
                temp = realloc(list, size *2);
                if(!temp)
                {
                    fprintf(stderr, "memory allocation failed\n");
                    return -1;
                }
                list = temp;
                size = size * 2;
            }
            list[i-1].next = list+i;
            list[i].prev = list + i - 1;
            list[i].next = NULL;
        }
        token = yylex();
    }
    if(hashmap_get(state->def_map, dup, (void**)&map) == MAP_OK)
    {
        for(j = 0; j < map->size; j++)
        {
            if(map->list[j].value)
                free(map->list[j].value);
        }
        free(map->list);
        free(dup);
        map->list = list;
        map->size = i;
        return 0;
    }
    map = (def_map_t*) calloc(1, sizeof(def_map_t));
    if(!map)
    {
        fprintf(stderr, "failed to allocate memory\n");
        free(list);
        free(dup);
        return -1;
    }
    map->list = list;
    map->key = dup;
    map->size = i;
    hashmap_put(state->def_map, dup, map);
    return 0;
}

static int add_lexeme(lexer_state_t *state, lexeme_t **lexeme)
{
    if(!state->first)
        state->first = *lexeme;
    if(state->cur)
    {
        state->cur->next = *lexeme;
        (*lexeme)->prev = state->cur;
        state->cur = (*lexeme);
    }
    else
    {
        state->cur = *lexeme;
        (*lexeme)->prev = NULL;
    }

    return 0;
}

static int handle_ifdef(lexer_state_t *state, int ndef, char* file)
{
    int token, boolean;
    def_map_t *map;
    lexeme_t *cur;

    token = yylex();
    if(token != IDENT_TOKEN)
    {
        fprintf(stderr, "expexted identified but got %s, at %d in %s", yytext, yyline, file);
        return -1;
    }

    hashmap_get(state->def_map, yytext, (void**)&map);

    boolean = ndef && map==NULL;
    cur = (lexeme_t*)calloc(1, sizeof(lexeme_t));
    if(!cur)
    {
        fprintf(stderr, "failed to allocate memoryi\n");
    }
    while(1)
    {
        token = yylex();
        if(token == ENDIF_TOKEN)
            break;
        else if (token == ELSE_DIREC_TOKEN)
            boolean = !boolean;
        else if (token == ELIF_TOKEN)
        {
            token = yylex();
            if(token != IDENT_TOKEN)
            {
                fprintf(stderr, "expexted identified but got %s, at %d in %s\n", yytext, yyline, file);
                continue;
            }
            boolean = hashmap_get(state->def_map, yytext, (void**)&map) == MAP_OK;
        }
        else if(boolean)
        {
            
            cur->token = token;
            cur->line_number = yyline;
            cur->filename = file;
            if(!process_token(state, cur))
            {
                add_lexeme(state, &cur);
                cur = (lexeme_t*)calloc(1, sizeof(lexeme_t));
                if(!cur)
                {
                    fprintf(stderr, "failed to allocate memoryi\n");
                }            
            }
        }
    }
    free(cur);
    return 0;
}

static int process_token(lexer_state_t *state, lexeme_t *token)
{
    char token_name[20], *dup, *file_temp;
    int line, token_num, length, i;
    def_map_t *map;
    FILE *file;
    YY_BUFFER_STATE lex_buff;

    switch(token->token)
    {
        case STRCONST_TOKEN:
            dup = (char*) malloc(sizeof(char)*strlen(yytext)+1);
            if(!dup)
            {
                fprintf(stderr, "failed to allocate memory\n");
            }
            strcpy(dup, yytext);
            token->value = dup;
            break;
        case INTCONST_TOKEN:
            token->value = (void*) malloc(sizeof(int));
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory\n");
            }
            *(int*)(token->value) = atoi(yytext);
            break;
        case HEXCONST_TOKEN:
            token->value = malloc(sizeof(int));
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory\n");
            }
            *(int*)(token->value) = strtol(yytext, NULL, 0);
            break;
        case REALCONST_TOKEN:
            token->value = (void*) malloc(sizeof(float));
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory\n");
            }
            *(float*)(token->value) = strtof(yytext, NULL);
            break;
        case CHARCONST_TOKEN:
            token->value = (void*) malloc(4);
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory\n");
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
            else if(token_num == NEWLINE_TOKEN)
            {
                fprintf(stderr, "%s invalid import on line %d: no file provided\n",token->filename, line);
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
            file_temp = state->cur_file;
            if(fill_state(state, dup) == INCLUDE_CYCLE)
            {
                fprintf(stderr, "include cycle detected %s, line %d\n", token->filename, token->line_number);
            }
            yypop_buffer_state();
            yyline = line;
            fclose(file);
            free(dup);
            state->cur_file = file_temp;
            return -1;
        case INCLUDE_FILE_TOKEN:
            fprintf(stderr, "unrecognized token %s, line %d\n", yytext, yyline);
            return -1;
        case DEFINE_TOKEN:
            add_definition(state);
            return -1;
        case UNDEF_TOKEN:
            token_num = yylex();
            if(token_num != IDENT_TOKEN)
            {
                fprintf(stderr, "expected identifier got %s in %s:%d", yytext, state->cur_file, yyline);
                return -1;
            }
            if(hashmap_get(state->def_map, yytext, (void**)&map) == MAP_OK)
            {
                hashmap_remove(state->def_map, yytext);
                free(map->key);
                for(i = 0; i < map->size; i ++)
                {
                    if(map->list[i].value)
                        free(map->list[i].value);
                }
                free(map->list);
            }
            return -1;
        case ENDIF_TOKEN:
            fprintf(stderr, "#endif found with no related #ifdef or #ifndef, %s:%d\n", state->cur_file, yyline);
            return -1;
        case ELIF_TOKEN:
            fprintf(stderr, "#elif found with no related #ifdef or #ifndef, %s:%d\n", state->cur_file, yyline);
            return -1;
        case IFDEF_TOKEN:
            handle_ifdef(state, 0, token->filename);
            return -1;
        case IFNDEF_TOKEN:
            handle_ifdef(state, 1, token->filename);
            return -1;
        case ELSE_DIREC_TOKEN:
            fprintf(stderr, "#else found with no related #ifdef or #ifndef, %s:%d\n", state->cur_file, yyline);
            return -1;
        case TYPE_TOKEN:
            token->value = malloc(5);
            strcpy(token->value, yytext);
            break;
        case NEWLINE_TOKEN:
            return -1;
        case IDENT_TOKEN:
            if(token->filename != NULL && hashmap_get(state->def_map, yytext, (void**)&map) == MAP_OK)
            {
                resolve_ident(state, map, token->filename);
                return -1;
            }
            token->value = malloc(strlen(yytext)+1);
            strcpy(token->value, yytext);
            break;
    }
    //if filename is null then this is a token for a definition and should not be printed
    if(program_options & LEXER_DEBUG_OPTION && token->filename != NULL) 
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
    if(cur)
    {
        next = cur->next;
        while(cur)
        {
            if(cur->value)
                free(cur->value);
            next = cur->next;
            free(cur);
            cur = next;
        }
    }
    hashmap_iterate(state->def_map, &clean_def_map, NULL);

    hashmap_free(state->def_map);

    free(state);
}

static int clean_def_map(void *nothing, void *map)
{
    int i;
    def_map_t *def_map;

    def_map = (def_map_t*) map;
    
    free(def_map->key);

    for(i = 0; i < def_map->size; i++)
    {
        if(def_map->list[i].value)
            free(def_map->list[i].value);
    }

    free(def_map->list);

    free(def_map);
    return 0;
}

static void resolve_ident(lexer_state_t *state, def_map_t *map, char* filename)
{
    int i;
    char token_name[20];
    lexeme_t *cur;
    def_map_t *nest;

    for(i = 0; i < map->size; i++)
    {
        cur = (lexeme_t*) calloc(1, sizeof(lexeme_t));
        cur->filename = filename;
        cur->line_number = yyline;
        cur->token = map->list[i].token;
        switch(cur->token)
        {
            case INTCONST_TOKEN:
                cur->value = malloc(sizeof(int));
                *(int*)cur->value = *(int*) map->list[i].value;
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text ''%d''\n", 
                        filename, cur->line_number, token_name, *(int*)cur->value
                    );
                }
                break;
            case CHARCONST_TOKEN:
                cur->value = malloc(4);
                *(int*)cur->value = *(int*) map->list[i].value;
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '%s'\n", 
                        filename, cur->line_number, token_name, (char*)cur->value 
                    );
                }
                break;
            case REALCONST_TOKEN:
                cur->value = malloc(sizeof(float));
                *(float*)cur->value = *(float*) map->list[i].value;
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '%f'\n", 
                        filename, cur->line_number, token_name, *(float*)cur->value 
                    );
                }
                break;
            case STRCONST_TOKEN:
                cur->value = malloc(strlen((char*)map->list[i].value));
                strcpy((char*)cur->value, (char*)map->list[i].value);
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '%s'\n", 
                        filename, cur->line_number, token_name, (char*)cur->value 
                    );
                }
                break;
            case HEXCONST_TOKEN:
                cur->value = malloc(sizeof(int));
                *(int*)cur->value = *(int*)map->list[i].value;
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '%#x'\n", 
                        filename, cur->line_number, token_name, *(int*)cur->value
                    );
                }
                break;
            case TYPE_TOKEN:
                cur->value = malloc(5);
                strcpy(cur->value, map->list[i].value);
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '%s'\n", filename, cur->line_number, token_name, (char*)cur->value);
                }
                break;
            case FOR_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text 'for'\n", filename, cur->line_number, token_name);
                }
                break;
            case WHILE_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text 'while'\n", filename, cur->line_number, token_name);
                }
                break;
            case DO_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text 'do'\n", filename, cur->line_number, token_name);
                }
                break;
            case IF_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text 'if'\n", filename, cur->line_number, token_name);
                }
                break; 
            case ELSE_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text 'else'\n", filename, cur->line_number, token_name);
                }
                break;
            case BREAK_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text 'break'\n", filename, cur->line_number, token_name);
                }
                break;
            case CONTINUE_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text 'continue'\n", filename, cur->line_number, token_name);
                }
                break;
            case RETURN_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text 'return'\n", filename, cur->line_number, token_name);
                }
                break;
            case IDENT_TOKEN:
                if(hashmap_get(state->def_map, map->list[i].value, (void**)&nest) == MAP_OK)
                {
                    resolve_ident(state, nest, filename);
                    continue;
                }
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    cur->value = malloc(strlen((char*)map->list[i].value));
                    strcpy(cur->value, map->list[i].value);
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '%s'\n", filename, cur->line_number, token_name, (char*)cur->value);
                }
                break;
            case LPAR_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '('\n", filename, cur->line_number, token_name);
                }
                break;
            case RPAR_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text ')'\n", filename, cur->line_number, token_name);
                }
                break;
            case LBRACKET_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '['\n", filename, cur->line_number, token_name);
                }
                break;
            case RBRACKET_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text ']'\n", filename, cur->line_number, token_name);
                }
                break;
            case LBRACE_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '{'\n", filename, cur->line_number, token_name);
                }
                break;
            case RBRACE_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '}'\n", filename, cur->line_number, token_name);
                }
                break;
            case COMMA_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text ','\n", filename, cur->line_number, token_name);
                }
                break;
            case SEMI_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text ';'\n", filename, cur->line_number, token_name);
                }
                break;
            case QUEST_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '?'\n", filename, cur->line_number, token_name);
                }
                break;
            case COLON_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text ':'\n", filename, cur->line_number, token_name);
                }
                break;
            case EQUAL_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '=='\n", filename, cur->line_number, token_name);
                }
                break;
            case NEQUAL_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '!='\n", filename, cur->line_number, token_name);
                }
                break;
            case GT_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '>'\n", filename, cur->line_number, token_name);
                }
                break;
            case GE_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '>='\n", filename, cur->line_number, token_name);
                }
                break;
            case LT_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '<'\n", filename, cur->line_number, token_name);
                }
                break;
            case LE_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '<='\n", filename, cur->line_number, token_name);
                }
                break;
            case PLUS_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '+'\n", filename, cur->line_number, token_name);
                }
                break;
            case MINUS_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '-'\n", filename, cur->line_number, token_name);
                }
                break;
            case STAR_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '*'\n", filename, cur->line_number, token_name);
                }
                break;
            case SLASH_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '/'\n", filename, cur->line_number, token_name);
                }
                break;
            case MOD_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '%%'\n", filename, cur->line_number, token_name);
                }
                break;
            case TILDE_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '~'\n", filename, cur->line_number, token_name);
                }
                break;
            case PIPE_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '|'\n", filename, cur->line_number, token_name);
                }
                break;
            case BANG_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '!'\n", filename, cur->line_number, token_name);
                }
                break;
            case AMP_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '&'\n", filename, cur->line_number, token_name);
                }
                break;
            case DAMP_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '&&'\n", filename, cur->line_number, token_name);
                }
                break;
            case DPIPE_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '||'\n", filename, cur->line_number, token_name);
                }
                break;
            case ASSIGN_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '='\n", filename, cur->line_number, token_name);
                }
                break;
            case PLUSASSIGN_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '+='\n", filename, cur->line_number, token_name);
                }
                break;
            case MINUSASSIGN_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '-='\n", filename, cur->line_number, token_name);
                }
                break;
            case STARASSIGN_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '*='\n", filename, cur->line_number, token_name);
                }
                break;
            case SLASHASSIGN_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '/='\n", filename, cur->line_number, token_name);
                }
                break;
            case INCR_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '++'\n", filename, cur->line_number, token_name);
                }
                break;
            case DECR_TOKEN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur->token);
                    printf("File %s Line %d Token %s Text '--'\n", filename, cur->line_number, token_name);
                }
                break;
        }
        add_lexeme(state, &cur);
    }
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
        case ELSE_DIREC_TOKEN:
            strcpy(buff, "ELSE_DIRECTIVE");
            break;
    }
}
