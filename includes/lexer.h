#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#define TOKEN_BUFF_SIZE 20

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
extern int yyline;
extern char* yytext;
extern char yytoken_name[TOKEN_BUFF_SIZE];

typedef struct yy_buffer_state *YY_BUFFER_STATE;
typedef struct lexeme 
{
    char* filename;
    int line_number;
    uint8_t token;
    struct lexeme* next;
    struct lexeme* prev;
} lexeme_t;

//functions in lex c 
YY_BUFFER_STATE yy_create_buffer ( FILE *file, int size  );
void yy_switch_to_buffer ( YY_BUFFER_STATE new_buffer  );
void yy_delete_buffer ( YY_BUFFER_STATE b  );
void yy_flush_buffer ( YY_BUFFER_STATE b  );
void yyrestart ( FILE *input_file  );

/**
 * function defined by lex
 */
int yylex(void);

/**
 * lexigraphical analysis of files
 * passed in through files array
 */
lexeme_t *lexical_analysis(int num, char** files);

/**
 *  function that takes a token
 *  constant as the first argument
 *  and fills the buffer with the 
 *  string representation of the constant.
 *  buffer must be at least 18 bytes
 */
void tok_to_str(char* buff, int token);

/**
 *  function that will clean all memory
 *  allocated durring the lexing process
 *  this will free all lexeme_t structs 
 *  and all filename strings store within
 *  those structs
 */
void clean_lexer();

#endif
