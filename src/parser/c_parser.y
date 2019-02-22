%code requires
{

#include "../../includes/parser.h"

}

%{

#include "../../includes/lexer.h"
#include "../../includes/parser.h"
#include "../../includes/types.h"

ast_node_t ast;

%}

%define parse.error verbose

%union 
{
    ast_value_t v;
    ast_node_t *a;
}

%token INCLUDE DEFINE TYPE FOR WHILE DO IF ELSE BREAK CONTINUE RETURN
%token IDENT INTCONST STRCONST CHARCONST INCLUDE_FILE 
%token EQUAL NEQUAL GE LE 
%token DAMP DPIPE PLUSASSIGN MINUSASSIGN 
%token STARASSIGN SLASHASSIGN INCR DECR UNKNOWN REALCONST IFNDEF IFDEF
%token ENDIF ELIF ELSE_DIREC HEXCONST UNDEF SCOPE 

%type<a> program variable statement identifiers identifier
%type<v> SCOPE TYPE INTCONST IDENT

%%

program: statement 
        { add_ast_children(&ast, $1, 1); }
    | statement program
        { add_ast_children(&ast, $1, 1); }
    ;

statement: variable
        { $$ = $1; }
    /*| function_proto 
        { $$ = $1; }
    | function_declar
        { $$ = $1; }*/
    ;

variable: SCOPE TYPE identifiers ';'
        {$$ = new_variable_node($1.i, $2.i, $3); }
    | TYPE identifiers ';'
        {$$ = new_variable_node(0, $1.i, $2); }
    ;

identifiers: identifier
        { $$ = $1; } 
    | identifier ',' identifiers
        { add_ast_children($1, $3, 1); $$ = $1; }
    ;

identifier:  IDENT
        { $$ = new_ast_node(IDENT, 0, NULL, $1, 0); }
    | IDENT '[' INTCONST ']'
        { $$ = new_ast_node(IDENT, 0, NULL, $1, $3.i); }
    ;
