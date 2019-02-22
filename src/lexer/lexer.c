#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <libgen.h>
#include "../../bin/parser/bison.h"
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
static int add_lexeme(lexer_state_t *state, lexeme_t lexeme);

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

void set_lval(int token)
{
    
}

static int fill_state(lexer_state_t *state, char *file)
{
    lexeme_t curtok;
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

    curtok.token = yylex();
    while(curtok.token)
    {  
        curtok.line_number = yyline;
        curtok.filename = file_copy;
        curtok.value = NULL;

        if(!process_token(state, &curtok))
        {
            add_lexeme(state, curtok);
        }
        curtok.token = yylex();
    }
    state->file_stack[i] = NULL;
    return 0;
}

static int add_definition(lexer_state_t *state)
{
    int token, i, j, size;
    char *dup;
    lexeme_t *list, *temp;
    def_map_t *map;

    token = yylex();
    if(token != IDENT)
    {
        fprintf(stderr, "expected identifier for definition but got %s\nignoring definition\n", yytext);
        while(token != NEWLINE)
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
    while(token != NEWLINE)
    {
        list[i].token = token;
        list[i].filename = NULL;
        list[i].value = NULL;
        list[i].line_number = yyline;
        if( token == DEFINE ||
            token == UNDEF ||
            token == IFNDEF ||
            token == ENDIF ||
            token == ELSE_DIREC ||
            token == INCLUDE)
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

static int handle_ifdef(lexer_state_t *state, int ndef, char* file)
{
    int token, boolean;
    def_map_t *map;
    lexeme_t cur;
    
    token = yylex();
    if(token != IDENT)
    {
        fprintf(stderr, "expexted identified but got %s, at %d in %s", yytext, yyline, file);
        return -1;
    }

    hashmap_get(state->def_map, yytext, (void**)&map);

    boolean = ndef && map==NULL;
    while(1)
    {
        token = yylex();
        if(token == ENDIF)
            break;
        else if (token == ELSE_DIREC)
            boolean = !boolean;
        else if (token == ELIF)
        {
            token = yylex();
            if(token != IDENT)
            {
                fprintf(stderr, "expexted identified but got %s, at %d in %s\n", yytext, yyline, file);
                continue;
            }
            boolean = hashmap_get(state->def_map, yytext, (void**)&map) == MAP_OK;
        }
        else if(boolean)
        {
            cur.value = NULL;           
            cur.token = token;
            cur.line_number = yyline;
            cur.filename = file;
            if(!process_token(state, &cur))
            {
                add_lexeme(state, cur);
            }
        }
    }
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
        case STRCONST:
            dup = (char*) malloc(sizeof(char)*strlen(yytext)+1);
            if(!dup)
            {
                fprintf(stderr, "failed to allocate memory\n");
            }
            strcpy(dup, yytext);
            token->value = dup;
            break;
        case INTCONST:
            token->value = (void*) malloc(sizeof(int));
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory\n");
            }
            *(int*)(token->value) = atoi(yytext);
            break;
        case HEXCONST:
            token->value = malloc(sizeof(int));
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory\n");
            }
            *(int*)(token->value) = strtol(yytext, NULL, 0);
            break;
        case REALCONST:
            token->value = (void*) malloc(sizeof(float));
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory\n");
            }
            *(float*)(token->value) = strtof(yytext, NULL);
            break;
        case CHARCONST:
            token->value = (void*) malloc(4);
            if(!token->value)
            {
                fprintf(stderr, "could not allocate memory\n");
            }
            strcpy(token->value, yytext);
            break;
        case INCLUDE:
            line = yyline;
            yyline = 1;
            token_num = yylex();
            if(token_num != STRCONST && token_num != INCLUDE_FILE && token_num != NEWLINE)
            {
                fprintf(stderr, "invalid import on line %d: %s\n", line, yytext);
                return -1;
            }
            else if(token_num == NEWLINE)
            {
                fprintf(stderr, "%s invalid import on line %d: no file provided\n",token->filename, line);
                return -1;
            }
            else if(token_num == STRCONST)
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
        case INCLUDE_FILE:
            fprintf(stderr, "unrecognized token %s, line %d\n", yytext, yyline);
            return -1;
        case DEFINE:
            add_definition(state);
            return -1;
        case UNDEF:
            token_num = yylex();
            if(token_num != IDENT)
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
        case ENDIF:
            fprintf(stderr, "#endif found with no related #ifdef or #ifndef, %s:%d\n", state->cur_file, yyline);
            return -1;
        case ELIF:
            fprintf(stderr, "#elif found with no related #ifdef or #ifndef, %s:%d\n", state->cur_file, yyline);
            return -1;
        case IFDEF:
            handle_ifdef(state, 0, token->filename);
            return -1;
        case IFNDEF:
            handle_ifdef(state, 1, token->filename);
            return -1;
        case ELSE_DIREC:
            fprintf(stderr, "#else found with no related #ifdef or #ifndef, %s:%d\n", state->cur_file, yyline);
            return -1;
        case TYPE:
            token->value = malloc(5);
            strcpy(token->value, yytext);
            break;
        case NEWLINE:
            return -1;
        case IDENT:
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

static int add_lexeme(lexer_state_t *state, lexeme_t lexeme)
{
    lexeme_t *pointer;

    if(program_options & LEXER_SAVE_OPTION)
    {
        pointer = calloc(1, sizeof(lexeme_t));
        if(pointer == NULL)
        {
            fprintf(stderr, "failed to allocated memory for lexeme_t");
            return -1;
        }
        memcpy(pointer, &lexeme, sizeof(lexeme_t));
        pointer->next = NULL;
        pointer->prev = NULL;

        if(!state->first)
            state->first = pointer;
        if(state->cur)
        {
            state->cur->next = pointer;
            pointer->prev = state->cur;
            state->cur = pointer;
        }
        else
        {
            state->cur = pointer;
            pointer->prev = NULL;
        }
    } 
    else
    {
        if(lexeme.value)
            free(lexeme.value);
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
    lexeme_t cur;
    def_map_t *nest;

    for(i = 0; i < map->size; i++)
    {
        cur.filename = filename;
        cur.line_number = yyline;
        cur.token = map->list[i].token;
        cur.value = NULL;
        switch(cur.token)
        {
            case INTCONST:
                cur.value = malloc(sizeof(int));
                *(int*)cur.value = *(int*) map->list[i].value;
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text ''%d''\n", 
                        filename, cur.line_number, token_name, *(int*)cur.value
                    );
                }
                break;
            case CHARCONST:
                cur.value = malloc(4);
                *(int*)cur.value = *(int*) map->list[i].value;
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '%s'\n", 
                        filename, cur.line_number, token_name, (char*)cur.value 
                    );
                }
                break;
            case REALCONST:
                cur.value = malloc(sizeof(float));
                *(float*)cur.value = *(float*) map->list[i].value;
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '%f'\n", 
                        filename, cur.line_number, token_name, *(float*)cur.value 
                    );
                }
                break;
            case STRCONST:
                cur.value = malloc(strlen((char*)map->list[i].value));
                strcpy((char*)cur.value, (char*)map->list[i].value);
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '%s'\n", 
                        filename, cur.line_number, token_name, (char*)cur.value 
                    );
                }
                break;
            case HEXCONST:
                cur.value = malloc(sizeof(int));
                *(int*)cur.value = *(int*)map->list[i].value;
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '%#x'\n", 
                        filename, cur.line_number, token_name, *(int*)cur.value
                    );
                }
                break;
            case TYPE:
                cur.value = malloc(5);
                strcpy(cur.value, map->list[i].value);
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '%s'\n", filename, cur.line_number, token_name, (char*)cur.value);
                }
                break;
            case FOR:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text 'for'\n", filename, cur.line_number, token_name);
                }
                break;
            case WHILE:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text 'while'\n", filename, cur.line_number, token_name);
                }
                break;
            case DO:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text 'do'\n", filename, cur.line_number, token_name);
                }
                break;
            case IF:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text 'if'\n", filename, cur.line_number, token_name);
                }
                break; 
            case ELSE:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text 'else'\n", filename, cur.line_number, token_name);
                }
                break;
            case BREAK:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text 'break'\n", filename, cur.line_number, token_name);
                }
                break;
            case CONTINUE:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text 'continue'\n", filename, cur.line_number, token_name);
                }
                break;
            case RETURN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text 'return'\n", filename, cur.line_number, token_name);
                }
                break;
            case IDENT:
                if(hashmap_get(state->def_map, map->list[i].value, (void**)&nest) == MAP_OK)
                {
                    resolve_ident(state, nest, filename);
                    continue;
                }
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    cur.value = malloc(strlen((char*)map->list[i].value));
                    strcpy(cur.value, map->list[i].value);
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '%s'\n", filename, cur.line_number, token_name, (char*)cur.value);
                }
                break;
            case LPAR:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '('\n", filename, cur.line_number, token_name);
                }
                break;
            case RPAR:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text ')'\n", filename, cur.line_number, token_name);
                }
                break;
            case LBRACKET:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '['\n", filename, cur.line_number, token_name);
                }
                break;
            case RBRACKET:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text ']'\n", filename, cur.line_number, token_name);
                }
                break;
            case LBRACE:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '{'\n", filename, cur.line_number, token_name);
                }
                break;
            case RBRACE:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '}'\n", filename, cur.line_number, token_name);
                }
                break;
            case COMMA:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text ','\n", filename, cur.line_number, token_name);
                }
                break;
            case SEMI:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text ';'\n", filename, cur.line_number, token_name);
                }
                break;
            case QUEST:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '?'\n", filename, cur.line_number, token_name);
                }
                break;
            case COLON:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text ':'\n", filename, cur.line_number, token_name);
                }
                break;
            case EQUAL:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '=='\n", filename, cur.line_number, token_name);
                }
                break;
            case NEQUAL:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '!='\n", filename, cur.line_number, token_name);
                }
                break;
            case GT:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '>'\n", filename, cur.line_number, token_name);
                }
                break;
            case GE:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '>='\n", filename, cur.line_number, token_name);
                }
                break;
            case LT:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '<'\n", filename, cur.line_number, token_name);
                }
                break;
            case LE:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '<='\n", filename, cur.line_number, token_name);
                }
                break;
            case PLUS:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '+'\n", filename, cur.line_number, token_name);
                }
                break;
            case MINUS:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '-'\n", filename, cur.line_number, token_name);
                }
                break;
            case STAR:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '*'\n", filename, cur.line_number, token_name);
                }
                break;
            case SLASH:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '/'\n", filename, cur.line_number, token_name);
                }
                break;
            case MOD:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '%%'\n", filename, cur.line_number, token_name);
                }
                break;
            case TILDE:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '~'\n", filename, cur.line_number, token_name);
                }
                break;
            case PIPE:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '|'\n", filename, cur.line_number, token_name);
                }
                break;
            case BANG:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '!'\n", filename, cur.line_number, token_name);
                }
                break;
            case AMP:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '&'\n", filename, cur.line_number, token_name);
                }
                break;
            case DAMP:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '&&'\n", filename, cur.line_number, token_name);
                }
                break;
            case DPIPE:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '||'\n", filename, cur.line_number, token_name);
                }
                break;
            case ASSIGN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '='\n", filename, cur.line_number, token_name);
                }
                break;
            case PLUSASSIGN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '+='\n", filename, cur.line_number, token_name);
                }
                break;
            case MINUSASSIGN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '-='\n", filename, cur.line_number, token_name);
                }
                break;
            case STARASSIGN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '*='\n", filename, cur.line_number, token_name);
                }
                break;
            case SLASHASSIGN:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '/='\n", filename, cur.line_number, token_name);
                }
                break;
            case INCR:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '++'\n", filename, cur.line_number, token_name);
                }
                break;
            case DECR:
                if(program_options & LEXER_DEBUG_OPTION && filename != NULL) 
                {
                    tok_to_str(token_name, cur.token);
                    printf("File %s Line %d Token %s Text '--'\n", filename, cur.line_number, token_name);
                }
                break;
        }
        add_lexeme(state, cur);
    }
}



void tok_to_str(char* buff, int token)
{
    switch(token)
    {
        case INCLUDE:
            strcpy(buff, "INCLUDE");
            break;
        case DEFINE:
            strcpy(buff, "DEFINE");
            break;
        case UNDEF:
            strcpy(buff, "UNDEF");
            break;
        case TYPE:
            strcpy(buff, "TYPE");
            break;
        case FOR:
            strcpy(buff, "FOR");
            break;
        case WHILE:
            strcpy(buff, "WHILE");
            break;
        case DO:
            strcpy(buff, "DO");
            break;
        case IF:
            strcpy(buff, "IF");
            break;
        case ELSE:
            strcpy(buff, "ELSE");
            break;
        case BREAK:
            strcpy(buff, "BREAK");
            break;
        case CONTINUE:
            strcpy(buff, "CONTINUE");
            break;
        case RETURN:
            strcpy(buff, "RETURN");
            break;
        case IDENT:
            strcpy(buff, "IDENT");
            break;
        case INTCONST:
            strcpy(buff, "INTCONST");
            break;
        case HEXCONST:
            strcpy(buff, "HEXCONST");
            break;
        case STRCONST:
            strcpy(buff, "STRCONST");
            break;
        case CHARCONST:
            strcpy(buff, "CHARCONST");
            break;
        case INCLUDE_FILE:
            strcpy(buff, "INCLUDE_FILE");
            break;
        case LPAR:
            strcpy(buff, "LPAR");
            break;
        case RPAR:
            strcpy(buff, "RPAR");
            break;
        case LBRACKET:
            strcpy(buff, "LBRACKET");
            break;
        case RBRACKET:
            strcpy(buff, "RBRACKET");
            break;
        case LBRACE:
            strcpy(buff, "LBRACE");
            break;
        case RBRACE:
            strcpy(buff, "RBRACE");
            break;
        case COMMA:
            strcpy(buff, "COMMA");
            break;
        case SEMI:
            strcpy(buff, "SEMI");
            break;
        case QUEST:
            strcpy(buff, "QUEST");
            break;
        case COLON:
            strcpy(buff, "COLON");
            break;
        case EQUAL:
            strcpy(buff, "EQUAL");
            break;
        case NEQUAL:
            strcpy(buff, "NEQUAL");
            break;
        case GT:
            strcpy(buff, "GT");
            break;
        case GE:
            strcpy(buff, "GE");
            break;
        case LT:
            strcpy(buff, "LT");
            break;
        case LE:
            strcpy(buff, "LE");
            break;
        case PLUS:
            strcpy(buff, "PLUS");
            break;
        case MINUS:
            strcpy(buff, "MINUS");
            break;
        case STAR:
            strcpy(buff, "STAR");
            break;
        case SLASH:
            strcpy(buff, "SLASH");
            break;
        case MOD:
            strcpy(buff, "MOD");
            break;
        case TILDE:
            strcpy(buff, "TILDE");
            break;
        case PIPE:
            strcpy(buff, "PIPE");
            break;
        case BANG:
            strcpy(buff, "BANG");
            break;
        case AMP:
            strcpy(buff, "AMP");
            break;
        case DAMP:
            strcpy(buff, "DAMP");
            break;
        case DPIPE:
            strcpy(buff, "DPIPE");
            break;
        case ASSIGN:
            strcpy(buff, "ASSIGN");
            break;
        case PLUSASSIGN:
            strcpy(buff, "PLUSASSIGN");
            break;
        case MINUSASSIGN:
            strcpy(buff, "MINUSASSIGN");
            break;
        case STARASSIGN:
            strcpy(buff, "STARASSIGN");
            break;
        case SLASHASSIGN:
            strcpy(buff, "SLASHASSIGN");
            break;
        case INCR:
            strcpy(buff, "INCR");
            break;
        case DECR:
            strcpy(buff, "DECR");
            break;
        case UNKNOWN:
            strcpy(buff, "UNKNOWN");
            break;
        case REALCONST:
            strcpy(buff, "REALCONST");
            break;
        case IFNDEF:
            strcpy(buff, "IFNDEF");
            break;
        case IFDEF: 
            strcpy(buff, "IFDEF");
            break;
        case ENDIF:
            strcpy(buff, "ENDIF");
            break;
        case ELIF:
            strcpy(buff, "ELIF");
            break;
        case ELSE_DIREC:
            strcpy(buff, "ELSE_DIRECTIVE");
            break;
    }
}
