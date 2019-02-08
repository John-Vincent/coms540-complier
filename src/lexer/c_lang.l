%{
#include "../../includes/types.h"
#include "../../includes/lexer.h"
int yyline=1;
%}

TYPE            int|char|void|float
IDENT           [_a-zA-Z][_a-zA-Z0-9]*
INTCONST        [0-9]+
HEXCONST        0x[0-9a-fA-F]+
REALCONST       [0-9]*\.?[0-9]+(?:e-?[0-9]*)?
STRCONST        \"([^\n]|\\\n)*([^\\\n]\"|\\\n\")
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
%x DIRECTIVE

%%

{INCLUDE}                    { BEGIN(DIRECTIVE); return INCLUDE_TOKEN; }
{DEFINE}                     { BEGIN(DIRECTIVE); return DEFINE_TOKEN; }
{UNDEF}                      { BEGIN(DIRECTIVE); return UNDEF_TOKEN; }
{IFDEF}                      { BEGIN(DIRECTIVE); return IFDEF_TOKEN; }
{IFNDEF}                     { BEGIN(DIRECTIVE); return IFNDEF_TOKEN; }
{ENDIF}                      { BEGIN(DIRECTIVE); return ENDIF_TOKEN; }
{ELIF}                       { BEGIN(DIRECTIVE); return ELIF_TOKEN; }
{ELSE}                       { BEGIN(DIRECTIVE); return ElSE_DIREC_TOKEN; }
"void"|"char"|"int"|"float"  { return TYPE_TOKEN; }
"for"                        { return FOR_TOKEN; }
while                        { return WHILE_TOKEN; }
do                           { return DO_TOKEN; } 
if                           { return IF_TOKEN; }
else                         { return ELSE_TOKEN; }
break                        { return BREAK_TOKEN; }
continue                     { return CONTINUE_TOKEN; }
return                       { return RETURN_TOKEN; } 
{IDENT}                      { return IDENT_TOKEN; }
{INTCONST}                   { return INTCONST_TOKEN; }
{HEXCONST}                   { return HEXCONST_TOKEN; }
{REALCONST}                  { return REALCONST_TOKEN; }
{STRCONST}                   { return STRCONST_TOKEN; }
{CHARCONST}                  { return CHARCONST_TOKEN; } 
{INCLUDEFILE}                { return INCLUDE_FILE_TOKEN; }
"("                          { return LPAR_TOKEN; }
")"                          { return RPAR_TOKEN; }
"["                          { return LBRACKET_TOKEN; }
"]"                          { return RBRACKET_TOKEN; }
"{"                          { return LBRACE_TOKEN; }
"}"                          { return RBRACE_TOKEN; }
","                          { return COMMA_TOKEN; }
";"                          { return SEMI_TOKEN; }
"?"                          { return QUEST_TOKEN; }
":"                          { return COLON_TOKEN; }
"=="                         { return EQUAL_TOKEN; }
"!="                         { return NEQUAL_TOKEN; }
">"                          { return GT_TOKEN; }
">="                         { return GE_TOKEN; }
"<"                          { return LT_TOKEN; }
"<="                         { return LE_TOKEN; }
"+"                          { return PLUS_TOKEN; }
"-"                          { return MINUS_TOKEN; }
"*"                          { return STAR_TOKEN; }
"/"                          { return SLASH_TOKEN; }
"%"                          { return MOD_TOKEN; }
"~"                          { return TILDE_TOKEN; }
"|"                          { return PIPE_TOKEN; }
"!"                          { return BANG_TOKEN; }
"&"                          { return AMP_TOKEN; }
"&&"                         { return DAMP_TOKEN; }
"||"                         { return DPIPE_TOKEN; }
"="                          { return ASSIGN_TOKEN; }
"+="                         { return PLUSASSIGN_TOKEN;}
"-="                         { return MINUSASSIGN_TOKEN; }
"*="                         { return STARASSIGN_TOKEN; }
"/="                         { return SLASHASSIGN_TOKEN; }
"++"                         { return INCR_TOKEN; }
"--"                         { return DECR_TOKEN; }
<DIRECTIVE>"\n"              { yyline++; BEGIN(INITIAL); return NEWLINE_TOKEN; }
"\n"                         { yyline++; }
{CCOMMENT}                   { /* IGNORE TEXT */ }
{WHITESPACE}*                { /* IGNORE TEXT */ }
{CPPCOMSTRT}                 { BEGIN(COMMENT); }
<COMMENT>{CPPCOMEND}         { BEGIN(INITIAL); }
<COMMENT>\n                  { yyline++;}
<COMMENT>.                   { /* IGNORE TEXT */ }
.                            { return UNKNOWN_TOKEN; } 

%%
