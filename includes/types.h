#ifndef LEX_TYPES_H
#define LEX_TYPES_H
    
    /**
     * changed these tokens to just use the characters
     * for simplicity in the parser
     * but wanted backwards compatibility 
     * with my lexer code that used these defined values
     */
    #define PLUS            '+'
    #define LPAR            '('
    #define RPAR            ')'
    #define LBRACKET        '['
    #define RBRACKET        ']'
    #define LBRACE          '{'
    #define RBRACE          '}'
    #define COMMA           ','
    #define SEMI            ';'
    #define QUEST           '?'
    #define COLON           ':'
    #define GT              '>'
    #define LT              '<'
    #define MINUS           '-'
    #define STAR            '*'
    #define SLASH           '/'
    #define MOD             '%'
    #define TILDE           '~'
    #define PIPE            '|'
    #define BANG            '!'
    #define AMP             '&'
    #define ASSIGN          '='
    #define NEWLINE         '\n'

    //variable types
    #define ERROR           0x00
    #define INT             0x01
    #define FLOAT           0x02
    #define VOID            0x04
    #define CHAR            0x08
    #define ARRAY           0x10
    #define TYPE_MASK       0x0FFFFFFF

    #define PROTO_TYPE      0x10000000
    #define DEF_TYPE        0x20000000
    #define FUNC_MASK       0x30000000

    #define STATIC          0x80000000
    #define EXTERN          0x40000000
    #define SCOPE_MASK      0xC0000000

#endif
