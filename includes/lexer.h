#ifndef LEXER_H
#define LEXER_H

#define TOKEN_BUFF_SIZE 20

extern int yyline;
extern char* yytext;
extern char yytoken_name[TOKEN_BUFF_SIZE];


/**
 * function defined by lex
 */
int yylex(void);

/**
 *  function that takes a token
 *  constant as the first argument
 *  and fills the buffer with the 
 *  string representation of the constant.
 *  buffer must be at least 18 bytes
 */
void tok_to_str(char* buff, int token);

#endif
