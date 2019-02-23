#ifndef LEX_TYPES_H
#define LEX_TYPES_H
    
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
    #define INT             0x01
    #define FLOAT           0x02
    #define VOID            0x04
    #define CHAR            0x08
    #define TYPE_MASK       0x3FFFFFFF
    #define STATIC          0x80000000
    #define EXTERN          0x40000000
    #define SCOPE_MASK      0xC0000000

#endif
