%{
#include "../../includes/lexer.h"
#include "../../includes/types.h"
#include "../parser/bison.h"
int yyline=1;
%}

TYPE            int|char|void|float
IDENT           [_a-zA-Z][_a-zA-Z0-9]*
INTCONST        [0-9]+
HEXCONST        0x[0-9a-fA-F]+
REALCONST       [0-9]*\.?[0-9]+(?:e-?[0-9]*)?
STRCONST        \"([^\n"]|\\\"|\\\n)*\"
INCLUDEFILE     <[^>]+>
CHARCONST       '\\.'|'[^\\]' 
WHITESPACE      [ \t\r]
CCOMMENT        \/\/[^\n]*
CPPCOMSTRT      \/\*
CPPCOMEND       \*\/
INCLUDE         #include
DEFINE          #define[ \t\r]+
IFDEF           #ifdef
IFNDEF          #ifndef
ELIF            #elif
ELSE            #else
ENDIF           #endif
UNDEF           #undef

%x COMMENT
%s DIRECTIVE

%%

{INCLUDE}                    { BEGIN(DIRECTIVE); return INCLUDE; }
{DEFINE}                     { BEGIN(DIRECTIVE); return DEFINE; }
{UNDEF}                      { BEGIN(DIRECTIVE); return UNDEF; }
{IFDEF}                      { BEGIN(DIRECTIVE); return IFDEF; }
{IFNDEF}                     { BEGIN(DIRECTIVE); return IFNDEF; }
{ENDIF}                      { BEGIN(DIRECTIVE); return ENDIF; }
{ELIF}                       { BEGIN(DIRECTIVE); return ELIF; }
{ELSE}                       { BEGIN(DIRECTIVE); return ELSE_DIREC; }
"void"|"char"|"int"|"float"  { set_lval(TYPE); return TYPE; }
"extern"|"static"            { set_lval(SCOPE); return SCOPE; }
"for"                        { return FOR; }
while                        { return WHILE; }
do                           { return DO; } 
if                           { return IF; }
else                         { return ELSE; }
break                        { return BREAK; }
continue                     { return CONTINUE; }
return                       { return RETURN; } 
{IDENT}                      { set_lval(IDENT); return IDENT; }
{INTCONST}                   { set_lval(INTCONST); return INTCONST; }
{HEXCONST}                   { set_lval(HEXCONST); return HEXCONST; }
{REALCONST}                  { set_lval(REALCONST); return REALCONST; }
{STRCONST}                   { set_lval(STRCONST); return STRCONST; }
{CHARCONST}                  { set_lval(CHARCONST); return CHARCONST; } 
<DIRECTIVE>{INCLUDEFILE}     { set_lval(INCLUDE_FILE); return INCLUDE_FILE; }
"("                          { return LPAR; }
")"                          { return RPAR; }
"["                          { return LBRACKET; }
"]"                          { return RBRACKET; }
"{"                          { return LBRACE; }
"}"                          { return RBRACE; }
","                          { return COMMA; }
";"                          { return SEMI; }
"?"                          { return QUEST; }
":"                          { return COLON; }
"=="                         { return EQUAL; }
"!="                         { return NEQUAL; }
">"                          { return GT; }
">="                         { return GE; }
"<"                          { return LT; }
"<="                         { return LE; }
"+"                          { return PLUS; }
"-"                          { return MINUS; }
"*"                          { return STAR; }
"/"                          { return SLASH; }
"%"                          { return MOD; }
"~"                          { return TILDE; }
"|"                          { return PIPE; }
"!"                          { return BANG; }
"&"                          { return AMP; }
"&&"                         { return DAMP; }
"||"                         { return DPIPE; }
"="                          { return ASSIGN; }
"+="                         { return PLUSASSIGN;}
"-="                         { return MINUSASSIGN; }
"*="                         { return STARASSIGN; }
"/="                         { return SLASHASSIGN; }
"++"                         { return INCR; }
"--"                         { return DECR; }
<DIRECTIVE>\n                { BEGIN(INITIAL); yyline++; return NEWLINE; } 
"\n"                         { BEGIN(INITIAL); yyline++; }
{CCOMMENT}                   { /* IGNORE TEXT */ }
{WHITESPACE}*                { /* IGNORE TEXT */ }
{CPPCOMSTRT}                 { BEGIN(COMMENT); }
<COMMENT>{CPPCOMEND}         { BEGIN(INITIAL); }
<COMMENT>\n                  { yyline++;}
<COMMENT>.                   { /* IGNORE TEXT */ }
.                            { set_lval(UNKNOWN); return UNKNOWN; } 

%%
