%{
#include "../../includes/types.h"
#include "../../includes/lexer.h"
int yyline=1;
char yytoken_name[TOKEN_BUFF_SIZE];
%}

TYPE        int|char|void|float
IDENT       [_a-zA-Z][_a-zA-Z0-9]*
INTCONST    [0-9]+
REALCONST   [0-9]*\.?[0-9]+(?:e-?[0-9]*)?
STRCONST    \"([^\n]|\\\n)*([^\\\n]\"|\\\n\")
INCLUDEFILE <[^>]+>
CHARCONST   '\\.'|'[^\\]' 
WHITESPACE  [ \t\r]
CCOMMENT    \/\/[^\n]*
CPPCOMSTRT  \/\*
CPPCOMEND   \*\/
INCLUDE     #include
DEFINE      #define

%x COMMENT

%%

{INCLUDE}                    { tok_to_str(yytoken_name, INCLUDE_TOKEN); return INCLUDE_TOKEN; }
{DEFINE}                     { tok_to_str(yytoken_name, DEFINE_TOKEN); return DEFINE_TOKEN; }
"void"|"char"|"int"|"float"  { tok_to_str(yytoken_name, TYPE_TOKEN); return TYPE_TOKEN; }
"for"                        { tok_to_str(yytoken_name, FOR_TOKEN); return FOR_TOKEN; }
while                        { tok_to_str(yytoken_name, WHILE_TOKEN); return WHILE_TOKEN; }
do                           { tok_to_str(yytoken_name, DO_TOKEN); return DO_TOKEN; } 
if                           { tok_to_str(yytoken_name, IF_TOKEN); return IF_TOKEN; }
else                         { tok_to_str(yytoken_name, ELSE_TOKEN); return ELSE_TOKEN; }
break                        { tok_to_str(yytoken_name, BREAK_TOKEN); return BREAK_TOKEN; }
continue                     { tok_to_str(yytoken_name, CONTINUE_TOKEN); return CONTINUE_TOKEN; }
return                       { tok_to_str(yytoken_name, RETURN_TOKEN); return RETURN_TOKEN; } 
{IDENT}                      { tok_to_str(yytoken_name, IDENT_TOKEN); return IDENT_TOKEN; }
{INTCONST}                   { tok_to_str(yytoken_name, INTCONST_TOKEN); return INTCONST_TOKEN; }
{REALCONST}                  { tok_to_str(yytoken_name, REALCONST_TOKEN); return REALCONST_TOKEN; }
{STRCONST}                   { tok_to_str(yytoken_name, STRCONST_TOKEN); return STRCONST_TOKEN; }
{CHARCONST}                  { tok_to_str(yytoken_name, CHARCONST_TOKEN); return CHARCONST_TOKEN; } 
{INCLUDEFILE}                { tok_to_str(yytoken_name, INCLUDE_FILE_TOKEN); return INCLUDE_FILE_TOKEN; }
"("                          { tok_to_str(yytoken_name, LPAR_TOKEN); return LPAR_TOKEN; }
")"                          { tok_to_str(yytoken_name, RPAR_TOKEN); return RPAR_TOKEN; }
"["                          { tok_to_str(yytoken_name, LBRACKET_TOKEN); return LBRACKET_TOKEN; }
"]"                          { tok_to_str(yytoken_name, RBRACKET_TOKEN); return RBRACKET_TOKEN; }
"{"                          { tok_to_str(yytoken_name, LBRACE_TOKEN); return LBRACE_TOKEN; }
"}"                          { tok_to_str(yytoken_name, RBRACE_TOKEN); return RBRACE_TOKEN; }
","                          { tok_to_str(yytoken_name, COMMA_TOKEN); return COMMA_TOKEN; }
";"                          { tok_to_str(yytoken_name, SEMI_TOKEN); return SEMI_TOKEN; }
"?"                          { tok_to_str(yytoken_name, QUEST_TOKEN); return QUEST_TOKEN; }
":"                          { tok_to_str(yytoken_name, COLON_TOKEN); return COLON_TOKEN; }
"=="                         { tok_to_str(yytoken_name, EQUAL_TOKEN); return EQUAL_TOKEN; }
"!="                         { tok_to_str(yytoken_name, NEQUAL_TOKEN); return NEQUAL_TOKEN; }
">"                          { tok_to_str(yytoken_name, GT_TOKEN); return GT_TOKEN; }
">="                         { tok_to_str(yytoken_name, GE_TOKEN); return GE_TOKEN; }
"<"                          { tok_to_str(yytoken_name, LT_TOKEN); return LT_TOKEN; }
"<="                         { tok_to_str(yytoken_name, LE_TOKEN); return LE_TOKEN; }
"+"                          { tok_to_str(yytoken_name, PLUS_TOKEN); return PLUS_TOKEN; }
"-"                          { tok_to_str(yytoken_name, MINUS_TOKEN); return MINUS_TOKEN; }
"*"                          { tok_to_str(yytoken_name, STAR_TOKEN); return STAR_TOKEN; }
"/"                          { tok_to_str(yytoken_name, SLASH_TOKEN); return SLASH_TOKEN; }
"%"                          { tok_to_str(yytoken_name, MOD_TOKEN); return MOD_TOKEN; }
"~"                          { tok_to_str(yytoken_name, TILDE_TOKEN); return TILDE_TOKEN; }
"|"                          { tok_to_str(yytoken_name, PIPE_TOKEN); return PIPE_TOKEN; }
"!"                          { tok_to_str(yytoken_name, BANG_TOKEN); return BANG_TOKEN; }
"&"                          { tok_to_str(yytoken_name, AMP_TOKEN); return AMP_TOKEN; }
"&&"                         { tok_to_str(yytoken_name, DAMP_TOKEN); return DAMP_TOKEN; }
"||"                         { tok_to_str(yytoken_name, DPIPE_TOKEN); return DPIPE_TOKEN; }
"="                          { tok_to_str(yytoken_name, ASSIGN_TOKEN); return ASSIGN_TOKEN; }
"+="                         { tok_to_str(yytoken_name, PLUSASSIGN_TOKEN); return PLUSASSIGN_TOKEN;}
"-="                         { tok_to_str(yytoken_name, MINUSASSIGN_TOKEN); return MINUSASSIGN_TOKEN; }
"*="                         { tok_to_str(yytoken_name, STARASSIGN_TOKEN); return STARASSIGN_TOKEN; }
"/="                         { tok_to_str(yytoken_name, SLASHASSIGN_TOKEN); return SLASHASSIGN_TOKEN; }
"++"                         { tok_to_str(yytoken_name, INCR_TOKEN); return INCR_TOKEN; }
"--"                         { tok_to_str(yytoken_name, DECR_TOKEN); return DECR_TOKEN; }
"\n"                         { yyline++; }
{CCOMMENT}                   { /* IGNORE TEXT */ }
{WHITESPACE}*                { /* IGNORE TEXT */ }
{CPPCOMSTRT}                 { BEGIN(COMMENT); }
<COMMENT>{CPPCOMEND}         { BEGIN(INITIAL); }
<COMMENT>\n                  { yyline++;}
<COMMENT>.                   { /* IGNORE TEXT */ }
.                            { tok_to_str(yytoken_name, UNKNOWN_TOKEN); return UNKNOWN_TOKEN; } 

%%
