#include <stdio.h>
#include "../../includes/types.h"

extern char* yytext;
extern int yyline;

int main(int argc, char** argv)
{
    int token;
    token = yylex();
    while(token)
    {   
        printf("line: %d, token: %d, string: %s\n", yyline, token, yytext);
        token = yylex();
    }
    return 0;
}

void parse_args(int argc, char** argv)
{
    
}
