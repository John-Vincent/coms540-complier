#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdio.h>
#include "hashmap.h"

//size copied from the c lex file
#ifndef YY_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k.
 * Moreover, YY_BUF_SIZE is 2*YY_READ_BUF_SIZE in the general case.
 * Ditto for the __ia64__ case accordingly.
 */
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif /* __ia64__ */
#endif

extern FILE* yyin;
extern char* yytext;
extern int yyline;

typedef struct yy_buffer_state *YY_BUFFER_STATE;

typedef struct lexeme 
{
    char* filename;
    int line_number;
    int token;
    //only set for some tokens
    void *value;
    struct lexeme* next;
    struct lexeme* prev;
} lexeme_t;

typedef struct def_map
{
    int size;
    lexeme_t *list;
    char *key;
} def_map_t;

typedef struct lexer_state
{
    char **file_strings;
    char **file_stack;
    char *cur_file;
    int stack_size;
    map_t def_map;
    int number_of_files;
    lexeme_t *cur;
    lexeme_t *first;
} lexer_state_t;


//functions in lex c 
YY_BUFFER_STATE yy_create_buffer ( FILE *file, int size  );
void yy_switch_to_buffer ( YY_BUFFER_STATE new_buffer  );
void yy_delete_buffer ( YY_BUFFER_STATE b  );
void yy_flush_buffer ( YY_BUFFER_STATE b  );
void yyrestart ( FILE *input_file  );
void yypush_buffer_state ( YY_BUFFER_STATE new_buffer  );
void yypop_buffer_state ( void );

/**
 * function defined by lex
 */
int yylex(void);

/**
 * lexigraphical analysis of files
 * passed in through files array
 */
lexer_state_t *lexical_analysis(int num, char** files);

/**
 *  function that will clean all memory
 *  allocated durring the lexing process
 *  this will free all lexeme_t structs 
 *  and all filename strings store within
 *  those structs
 */
void clean_lexer(lexer_state_t *state);

/**
 *  this function sets the parser value 
 *  for some tokens based on the token type.
 */
void set_lval(int token);

#endif
