%code requires
{

#include "../../includes/parser.h"
extern ast_node_t yyast;

}

%{

#include "../../includes/lexer.h"
#include "../../includes/parser.h"

ast_node_t yyast;
ast_value_t yyempty_value;
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
%token ENDIF ELIF ELSE_DIREC HEXCONST UNDEF SCOPE LVALUE EMPTY PARAM_LIST STATEMENT

%type<a> program variable statement identifiers identifier statement_block l_value program_statement
%type<a> statements if else do while for expression expressions constant optional_expression
%type<a> function_proto function_def type_name param_list params statement_list variables variable_list
%type<v> assignment binary unary 
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
        { $$ = $1; }
    | function_proto 
        { $$ = $1; }
    | function_def
        { $$ = $1; }
    ;

variables: 
    { $$ = new_ast_node(VARIABLE_LIST, 0, NULL, yyempty_value, 0); }
    | variable_list
    { $$ = make_node_list(VARIABLE_LIST, $1); } 

variable_list: variable
            { $$ = new_ast_node(END_NODE_LIST, 1, $1, yyempty_value, 0); } 
        | variable variable_list
            { $$ = new_ast_node(NODE_LIST, 1, $1, yyempty_value, 0); 
              add_ast_children($$, $2, 1);
            }

variable: SCOPE TYPE identifiers ';'
        { $$ = new_variable_node($1.i, $2.i, $3); }
    | TYPE identifiers ';'
        { $$ = new_variable_node(0, $1.i, $2); }
    ;

function_proto: type_name '(' params ')' ';'
        { $$ = new_ast_node(FUNCTION_PROTO, 1, $1, yyempty_value, 0);
          add_ast_children($$, $3, 1);
        }
    ;

function_def: type_name '(' params ')' '{' variables statements '}'
        { $$ = new_ast_node(FUNCTION_DEF, 1, $1, yyempty_value, 0);
          add_ast_children($$, $3, 1);
          add_ast_children($$, $6, 1);
          add_ast_children($$, $7, 1);
        }
    ;

params: 
        { $$ = new_ast_node(PARAM_LIST, 0, NULL, yyempty_value, 0); }
    | param_list
        { $$ = make_node_list(PARAM_LIST, $1); }

param_list: type_name
        { $$ = new_ast_node(END_NODE_LIST, 1, $1, yyempty_value, 0); }
    | type_name ',' param_list %prec COMMA_FAKE
        { $$ = new_ast_node(NODE_LIST, 1, $1, yyempty_value, 0); 
          add_ast_children($$, $3, 1);
        }
    ;

type_name: TYPE IDENT
    { $$ = new_ast_node(TYPE, 0, NULL, $1, 0); 
      $$ = new_ast_node(TYPE_NAME, 1, $$, $2, 0); 
    }
    ;

statement_block: '{' statements '}'
        { $$ = $2; }
    ;

statements:
    { $$ = new_ast_node(STATEMENT_BLOCK, 0, NULL, yyempty_value, 0); }
    | statement_list
    { $$ = make_node_list(STATEMENT_BLOCK, $1); } 


statement_list: statement
        { $$ = new_ast_node(END_NODE_LIST, 1, $1, yyempty_value, 0); } 
    | statement statement_list
        { $$ = new_ast_node(NODE_LIST, 1, $1, yyempty_value, 0); 
          add_ast_children($$, $2, 1);
        }
    ;

statement: ';'
        { $$ = new_ast_node(EMPTY, 0, NULL, yyempty_value, 0); } 
    | BREAK ';'
        { $$ = new_ast_node(BREAK, 0, NULL, yyempty_value, 0); } 
    | CONTINUE ';'
        { $$ = new_ast_node(CONTINUE, 0, NULL, yyempty_value, 0); } 
    | RETURN ';'
        { $$ = new_ast_node(RETURN, 0, NULL, yyempty_value, 0); }
    | RETURN expression ';'
        { $$ = new_ast_node(RETURN, 1, $2, yyempty_value, 0); }
    | expression ';'
        { $$ = $1; }
    | if %prec NOELSE
        { $$ = new_ast_node(IF, 1, $1, yyempty_value, 0); }
    | if else %prec ELSE
        { $$ = new_ast_node(IF, 1, $1, yyempty_value, 0); 
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
        { $$ = $1; } 
    | expression ',' expressions %prec ','
        { $$ = new_ast_node(EXPRESSION_LIST, 1, $1, yyempty_value, 0); 
          add_ast_children($$, $3, 1);
        } 
    ;

optional_expression:
        { $$ = new_ast_node(EMPTY, 0, NULL, yyempty_value, 0); }
    | expression
        { $$ = $1; }
    ;

expression: constant
        { $$ = $1; } 
    | IDENT
        { $$ = new_ast_node(IDENT, 0, NULL, $1, 0); }
    | IDENT '(' expressions ')'
        { $$ = new_ast_node(FUNCTION_CALL, 1, $3, $1, 0); }
    | l_value assignment expression %prec '='
        {$$ = new_ast_node($2.i, 1, $1, yyempty_value, 0);
         add_ast_children($$, $3, 1);
        }
    | l_value INCR %prec UNARY
        {$$ = new_ast_node(INCR, 1, $1, yyempty_value, 0);}
    | l_value DECR %prec UNARY
        {$$ = new_ast_node(DECR, 1, $1, yyempty_value, 0);}
    | unary expression %prec UNARY
        {$$ = new_ast_node($1.i, 1, $2, yyempty_value, 0);}
    | expression binary expression %prec EQUALITY
        {
            $$ = new_ast_node(BINARY_OP, 1, $1, $2, 0);
            add_ast_children($$, $3, 1);
        }
    | expression '?' expression ':' expression %prec TURNARY
        { $$ = new_ast_node(TURNARY, 1, $1, yyempty_value, 0);
          add_ast_children($$, $3, 1);
          add_ast_children($$, $5, 1);
        }
    | '(' TYPE ')' expression %prec PAREN
        { $$ = new_ast_node(CAST, 1, $4, $2, 0); } 
    | '(' expression ')' %prec PAREN
        { $$ = $2; } 
    ;

l_value: IDENT '[' expression ']' %prec BRACK
        { $$ = new_ast_node(LVALUE, 1, $3, $1, 0); }
    | IDENT
        { $$ = new_ast_node(LVALUE, 0, NULL, $1, 0); } 
    ;

unary: '-' 
        { $$.i =  '-' ; }
    | '!' 
        { $$.i = '!' ; }
    | '~'
        { $$.i = '~'; }
    ;

binary: EQUAL %prec EQUALITY
        { $$.i = EQUAL; }
    | NEQUAL %prec EQUALITY
        { $$.i = NEQUAL; }
    | '>' %prec INEQUALITY
        { $$.i = '>'; } 
    | GE %prec INEQUALITY
        { $$.i = GE; }
    | '<' %prec INEQUALITY
        { $$.i = '<'; }
    | LE %prec INEQUALITY
        { $$.i = LE; }
    | '+' %prec '+'
        { $$.i = '+'; }
    | '-' %prec '-'
        { $$.i = '-'; }
    | '*' %prec '*'
        { $$.i = '*'; }
    | '/' %prec '/'
        { $$.i = '/'; }
    | '%' %prec '%'
        { $$.i = '%'; }
    | '|' %prec '|'
        { $$.i = '|'; }
    | '&' %prec '&'
        { $$.i = '&'; }
    | DPIPE %prec DPIPE
        { $$.i = DPIPE; }
    | DAMP %prec DAMP
        { $$.i = DAMP; }
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
        { $$ = new_ast_node(INTCONST, 0, NULL, $1, 0); }
    | STRCONST
        { $$ = new_ast_node(STRCONST, 0, NULL, $1, 0); }
    | CHARCONST
        { $$ = new_ast_node(CHARCONST, 0, NULL, $1, 0); }
    | REALCONST
        { $$ = new_ast_node(REALCONST, 0, NULL, $1, 0); }
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

do: DO statement ';' WHILE '(' expression ')' ';'
        { $$ = new_ast_node(DO, 1, $6, yyempty_value, 0);
          add_ast_children($$, $2, 1);
        }
    | DO statement_block WHILE '(' expression ')' ';'
        { $$ = new_ast_node(DO, 1, $5, yyempty_value, 0);
          add_ast_children($$, $2, 1);
        }
    ;

while: WHILE '(' expression ')' statement ';'
        { $$ = new_ast_node(WHILE, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
        }
    | WHILE '(' expression ')' statement_block
        { $$ = new_ast_node(WHILE, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
        }
    ;

if: IF '(' expression ')' statement ';' 
        { $$ = new_ast_node(IF, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
        }
    | IF '(' expression ')' statement_block
        { $$ = new_ast_node(IF, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
        }
    ;

else: ELSE statement ';'
        { $$ = new_ast_node(ELSE, 1, $2, yyempty_value, 0); }
    | ELSE statement_block
        { $$ = new_ast_node(ELSE, 1, $2, yyempty_value, 0); }
    ;

for: FOR '(' optional_expression ';' optional_expression ';' optional_expression ')' statement ';'
        { $$ = new_ast_node(FOR, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
          add_ast_children($$, $7, 1);
          add_ast_children($$, $9, 1);
        }
    | FOR '(' optional_expression ';' optional_expression ';' optional_expression ')' statement_block
        { $$ = new_ast_node(FOR, 1, $3, yyempty_value, 0);
          add_ast_children($$, $5, 1);
          add_ast_children($$, $7, 1);
          add_ast_children($$, $9, 1);
        }
    ;

