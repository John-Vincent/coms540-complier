%code requires
{

#include "../../includes/parser.h"
extern ast_node_t yyast;

}

%{

#include "../../includes/lexer.h"
#include "../../includes/parser.h"
#include "../../includes/types.h"

ast_node_t yyast;
ast_value_t yyempty_value;
extern char* parse_file_string;
%}

%define parse.error verbose

%union 
{
    ast_value_t v;
    ast_node_t *a;
}

%token INCLUDE DEFINE TYPE FOR WHILE DO IF ELSE BREAK CONTINUE RETURN VARIABLE NODE_LIST
%token IDENT INTCONST STRCONST CHARCONST INCLUDE_FILE FUNCTION_CALL STATEMENT_BLOCK
%token EQUAL NEQUAL GE LE FUNCTION EXPRESSION_LIST BINARY_OP UNARY_OP FUNCTION_DEF
%token DAMP DPIPE PLUSASSIGN MINUSASSIGN CAST PROGRAM END_NODE_LIST FUNCTION_PROTO
%token STARASSIGN SLASHASSIGN INCR DECR UNKNOWN REALCONST IFNDEF IFDEF TYPE_NAME VARIABLE_LIST
%token ENDIF ELIF ELSE_DIREC HEXCONST UNDEF SCOPE LVALUE EMPTY PARAM_LIST STATEMENT ZEQUAL ZNEQUAL

%type<a> program variable statement identifiers identifier statement_block l_value program_statement
%type<a> statements if else do while for expression expressions constant optional_expression func_type_name
%type<a> function_proto function_def type_name param_list params statement_list variables variable_list
%type<a> function_sig
%type<v> assignment unary 
%type<v> SCOPE TYPE IDENT INTCONST STRCONST CHARCONST REALCONST

%nonassoc NOELSE
%nonassoc ELSE
%left COMMA_FAKE
%right '=' PLUSASSIGN STARASSIGN MINUSASSIGN SLASHASSIGN
%right TURNARY '?' ':'
%left DPIPE
%left DAMP
%left '|'
%left '&'
%left EQUALITY EQUAL NEQUAL
%left INEQUALITY '<' GE '>' LE
%left '+' '-'
%left '*' '/' '%'
%right UNARY '!' '~' 
%left PAREN BRACK

%%

program: program_statement 
        { add_ast_children(&yyast, $1, 1); }
    | program program_statement 
        { add_ast_children(&yyast, $2, 1); }
    ;

program_statement: variable
        { $$ = $1; process_declaration($1, 0); }
    | function_proto 
        { $$ = $1; }
    | function_def
        { $$ = $1; }
    ;

variables: 
    { $$ = new_ast_node(VARIABLE_LIST, 0, 0, NULL, yyempty_value, 0); }
    | variable_list
    { $$ = invert_list($1); process_declaration($1, 1); }

variable_list: variable
            { $$ = new_ast_node(VARIABLE_LIST, 0, 1, $1, yyempty_value, 0); } 
        | variable variable_list
            { $$ = $2;
              add_ast_children($$, $1, 1);
            }

variable: SCOPE TYPE identifiers ';'
        { $$ = new_variable_node($1.i, $2.i, $3); }
    | TYPE identifiers ';'
        { $$ = new_variable_node(0, $1.i, $2); }
    ;

function_proto: func_type_name '(' params ')' ';'
        { $$ = make_function_sig($1, $3, FUNCTION_PROTO); }
    ;

function_def: function_sig '{' variables statements '}'
        { $$ = $1;
          add_ast_children($$, $3, 1);
          add_ast_children($$, $4, 1);
          $$->value.t = close_scope();
        }
    ;

function_sig: func_type_name '(' params ')'
    { $$ = make_function_sig($1, $3, FUNCTION_DEF); }
    ;

params: 
        { $$ = new_ast_node(PARAM_LIST, 0, 0, NULL, yyempty_value, 0); }
    | param_list
        { $$ = invert_list($1); } 

param_list: type_name
        { $$ = new_ast_node(PARAM_LIST, 0, 1, $1, yyempty_value, 0); }
    | type_name ',' param_list %prec COMMA_FAKE
        { $$ = $3;
          add_ast_children($$, $1, 1);
        }
    ;

type_name: TYPE IDENT
    {
        $$ = new_ast_node(TYPE_NAME, $1.i, 0, NULL, $2, 0);
    }
    | TYPE IDENT '[' INTCONST ']'
    {
        $$ = new_ast_node(TYPE_NAME, $1.i | ARRAY, 0, NULL, $2, $4.i);
    }
    ;

func_type_name: TYPE IDENT
        { $$ = new_ast_node(TYPE_NAME, $1.i, 0, NULL, $2, 0); }
    | TYPE '*' IDENT
        { $$ = new_ast_node(TYPE_NAME, $1.i | ARRAY, 0, NULL, $3, 0); }
    ;

statement_block: '{' statements '}'
        { $$ = $2; }
    ;

statements:
    { $$ = new_ast_node(STATEMENT_BLOCK, 0, 0, NULL, yyempty_value, 0); }
    | statement_list
    { $$ = invert_list($1); }


statement_list: statement
        { $$ = new_ast_node(STATEMENT_BLOCK, 0, 1, $1, yyempty_value, 0); } 
    | statement statement_list
        { $$ = $2;
          add_ast_children($$, $1, 1);
        }
    ;

statement: ';'
        { $$ = new_ast_node(EMPTY, 0, 0, NULL, yyempty_value, 0); } 
    | BREAK ';'
        { $$ = new_ast_node(BREAK, 0, 0, NULL, yyempty_value, 0); } 
    | CONTINUE ';'
        { $$ = new_ast_node(CONTINUE, 0, 0, NULL, yyempty_value, 0); } 
    | RETURN ';'
        { $$ = new_ast_node(RETURN, VOID, 0, NULL, yyempty_value, 0); check_return_type(VOID);}
    | RETURN expression ';'
        { $$ = new_ast_node(RETURN, $2->type, 1, $2, yyempty_value, 0); check_return_type($2->type);}
    | expression ';'
        { $$ = $1; print_expression_type($1->type);}
    | if %prec NOELSE
        { $$ = new_ast_node(IF, 0, 1, $1, yyempty_value, 0); }
    | if else %prec ELSE
        { $$ = new_ast_node(IF, 0, 1, $1, yyempty_value, 0); 
          add_ast_children($$, $2, 1);
        }
    | for
        { $$ = $1; } 
    | while
        { $$ = $1; }
    | do
        { $$ = $1; } 
    ;

expressions: expression
        { $$ = new_ast_node(EXPRESSION_LIST, 0, 1, $1, yyempty_value, 0); } 
    | expression ',' expressions %prec ','
        { $$ = $3;
          add_ast_children($$, $1, 1);
        } 
    ;

optional_expression:
        { $$ = new_ast_node(EMPTY, 0, 0, NULL, yyempty_value, 0); }
    | expression
        { $$ = $1; }
    ;

expression: constant
        { $$ = $1; } 
    | l_value
        { $$ = $1; }
    | IDENT '(' expressions ')'
        { $$ = new_ast_node(FUNCTION_CALL, find_type($1.s), 1, invert_list($3), $1, 0); 
            check_function_params($$);  
        }
    | IDENT '(' ')'
        { $$ = new_ast_node(FUNCTION_CALL, find_type($1.s), 0, NULL, $1, 0); 
            check_function_params($$); 
        }
    | l_value assignment expression %prec '='
        {$$ = new_ast_node($2.i, resolve_bop_type($2.i, $1->type, $3->type), 1, $1, yyempty_value, 0);
         add_ast_children($$, $3, 1);
        }
    | l_value INCR %prec UNARY
        {$$ = new_ast_node(INCR, $1->type, 1, $1, yyempty_value, 0);}
    | l_value DECR %prec UNARY
        {$$ = new_ast_node(DECR, $1->type, 1, $1, yyempty_value, 0);}
    | unary expression %prec UNARY
        {$$ = new_ast_node($1.i, resolve_uop_type($1.i, $2->type), 1, $2, yyempty_value, 0);}
    | expression EQUAL expression %prec EQUALITY
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type(EQUAL, $1->type, $3->type), 1, $1, (ast_value_t)EQUAL, 0);
            add_ast_children($$, $3, 1);
        }
    | expression NEQUAL expression %prec EQUALITY
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type(NEQUAL, $1->type, $3->type), 1, $1, (ast_value_t)NEQUAL, 0);
            add_ast_children($$, $3, 1);
        }
    | expression '>' expression %prec INEQUALITY
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type('>', $1->type, $3->type), 1, $1, (ast_value_t)'>', 0);
            add_ast_children($$, $3, 1);
        }
    | expression GE expression %prec INEQUALITY
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type(GE, $1->type, $3->type), 1, $1, (ast_value_t)GE, 0);
            add_ast_children($$, $3, 1);
        }
    | expression '<' expression %prec INEQUALITY
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type('<', $1->type, $3->type), 1, $1, (ast_value_t)'<', 0);
            add_ast_children($$, $3, 1);
        }
    | expression LE expression %prec INEQUALITY
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type(LE, $1->type, $3->type), 1, $1, (ast_value_t)LE, 0);
            add_ast_children($$, $3, 1);
        }
    | expression '+' expression %prec '+'
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type('+', $1->type, $3->type), 1, $1, (ast_value_t)'+', 0);
            add_ast_children($$, $3, 1);
        }
    | expression '-' expression %prec '-'
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type('-', $1->type, $3->type), 1, $1, (ast_value_t)'-', 0);
            add_ast_children($$, $3, 1);
        }
    | expression '*' expression %prec '*'
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type('*', $1->type, $3->type), 1, $1, (ast_value_t)'*', 0);
            add_ast_children($$, $3, 1);
        }
    | expression '/' expression %prec '/'
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type('/', $1->type, $3->type), 1, $1, (ast_value_t)'/', 0);
            add_ast_children($$, $3, 1);
        }
    | expression '%' expression %prec '%'
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type('%', $1->type, $3->type), 1, $1, (ast_value_t)'%', 0);
            add_ast_children($$, $3, 1);
        }
    | expression '|' expression %prec '|'
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type('|', $1->type, $3->type), 1, $1, (ast_value_t)'|', 0);
            add_ast_children($$, $3, 1);
        }
    | expression '&' expression %prec '&'
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type('&', $1->type, $3->type), 1, $1, (ast_value_t)'&', 0);
            add_ast_children($$, $3, 1);
        }
    | expression DPIPE expression %prec DPIPE
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type(DPIPE, $1->type, $3->type), 1, $1, (ast_value_t)DPIPE, 0);
            add_ast_children($$, $3, 1);
        }
    | expression DAMP expression %prec DAMP
        {
            $$ = new_ast_node(BINARY_OP, resolve_bop_type(DAMP, $1->type, $3->type), 1, $1, (ast_value_t)DAMP, 0);
            add_ast_children($$, $3, 1);
        }
    | expression '?' expression ':' expression 
        { $$ = new_ast_node(TURNARY, resolve_turnary_type($1->type, $3->type, $5->type), 1, $1, yyempty_value, 0);
          add_ast_children($$, $3, 1);
          add_ast_children($$, $5, 1);
        }
    | '(' TYPE ')' expression %prec PAREN
        { $$ = new_ast_node(CAST, $2.i, 1, $4, $2, 0); } 
    | '(' expression ')' %prec PAREN
        { $$ = $2; } 
    ;

l_value: IDENT '[' expression ']' %prec BRACK
        { $$ = new_ast_node(LVALUE, resolve_bop_type('[', find_type($1.s), $3->type), 1, $3, $1, 0); }
    | IDENT
        { $$ = new_ast_node(LVALUE, find_type($1.s), 0, NULL, $1, 0); } 
    ;

unary: '-'
        { $$.i =  '-' ; }
    | '!'
        { $$.i = '!' ; }
    | '~'
        { $$.i = '~'; }
    ;

assignment: '='
    { $$.i = '='; }
    | PLUSASSIGN
        { $$.i = PLUSASSIGN; }
    | MINUSASSIGN
        { $$.i = MINUSASSIGN; }
    | STARASSIGN
        { $$.i = STARASSIGN; }
    | SLASHASSIGN
        { $$.i = SLASHASSIGN; }
    ;

constant: INTCONST
        { $$ = new_ast_node(INTCONST, INT, 0, NULL, $1, 0); }
    | STRCONST
        { $$ = new_ast_node(STRCONST, CHAR | ARRAY, 0, NULL, $1, 0); }
    | CHARCONST
        { $$ = new_ast_node(CHARCONST, CHAR, 0, NULL, $1, 0); }
    | REALCONST
        { $$ = new_ast_node(REALCONST, FLOAT, 0, NULL, $1, 0); }
    ;

identifiers: identifier
        { $$ = $1; } 
    | identifier ',' identifiers
        { add_ast_children($1, $3, 1); $$ = $1; }
    ;

identifier:  IDENT
        { $$ = new_ast_node(IDENT, 0, 0, NULL, $1, 0); }
    | IDENT '[' INTCONST ']'
        { $$ = new_ast_node(IDENT, 0, 0, NULL, $1, $3.i); }
    ;

do: DO statement WHILE '(' expression ')' ';'
        { $$ = new_ast_node(DO, 0, 1, $5, yyempty_value, 0);
          add_ast_children($$, $2, 1);
        }
    | DO statement_block WHILE '(' expression ')' ';'
        { $$ = new_ast_node(DO, 0, 1, $5, yyempty_value, 0);
          add_ast_children($$, $2, 1);
        }
    ;

while: WHILE '(' expression ')' statement
        { $$ = new_ast_node(WHILE, 0, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
        }
    | WHILE '(' expression ')' statement_block
        { $$ = new_ast_node(WHILE, 0, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
        }
    ;

if: IF '(' expression ')' statement 
        { $$ = new_ast_node(IF, 0, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
        }
    | IF '(' expression ')' statement_block
        { $$ = new_ast_node(IF, 0, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
        }
    ;

else: ELSE statement
        { $$ = new_ast_node(ELSE, 0, 1, $2, yyempty_value, 0); }
    | ELSE statement_block
        { $$ = new_ast_node(ELSE, 0, 1, $2, yyempty_value, 0); }
    ;

for: FOR '(' optional_expression ';' optional_expression ';' optional_expression ')' statement
        { $$ = new_ast_node(FOR, 0, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
          add_ast_children($$, $7, 1);
          add_ast_children($$, $9, 1);
        }
    | FOR '(' optional_expression ';' optional_expression ';' optional_expression ')' statement_block
        { $$ = new_ast_node(FOR, 0, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
          add_ast_children($$, $7, 1);
          add_ast_children($$, $9, 1);
        }
    ;

