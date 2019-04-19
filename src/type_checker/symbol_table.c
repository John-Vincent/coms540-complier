#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../bin/parser/bison.h"
#include "../../includes/symbol_table.h"
#include "../../includes/hashmap.h"
#include "../../includes/utils.h"
#include "../../includes/types.h"
#include "../../includes/main.h"

#define ARRAY_STRING    "[]"
#define FUNC_STRING     "()"
#define PARAM_SIZE      10

typedef struct symbol
{
    char *symbol;
    char *file;
    int line;
    int type;
    int num_params;
    int *params;
} symbol_imp_t;

extern int yyline;

extern char *parse_file_string;

static void print_type_error(char* cur_file, char* old_file, char* symbol, int type, int old_line, int new_line);

static symbol_imp_t *create_symbol(char *symbol, int type, int line, char *file, int *params);

static int update_params(symbol_imp_t *sym, int *params);

static int type_coerce(int t1, int t2);

static symbol_table_t global_symbols = NULL;

static symbol_table_t local_symbols = NULL;

static int return_type = 0;

static int error_in_types = 0;

static char* BUILT_IN_FUNC= "built in";

int init_symbol_table()
{
    int i, params[2];
    params[0]  = DEF_TYPE;
    i = global_scope_add("getchar", INT, BUILT_IN_FUNC, 0, params);
    if(i == 0)
    {
        params[0] = INT;
        params[1] = DEF_TYPE;
        i = global_scope_add("putchar", INT, BUILT_IN_FUNC, 0, params);
    }
    return i;
}

int has_type_error()
{
    return error_in_types;
}

int global_scope_add(char* symbol, int type, char* file, int line, int* params)
{
    if(!(program_options & TYPE_OPTION))
        return 0;

    symbol_imp_t *sym;

    if(global_symbols == NULL)
        global_symbols = hashmap_new();
    
    if( hashmap_get(global_symbols, symbol, (void**)&sym) == MAP_OK )
    {
        if(!update_params(sym, params))
        {
            print_type_error(file, sym->file, symbol, sym->type, sym->line, line);
            return -1;
        }
    }
    else
    {
        sym = create_symbol(symbol, type, line, file, params);
        hashmap_put(global_symbols, symbol, sym);
    }
    return 0;
}

/**
 *  This will attempt to add a symbol into the local
 *  scope. It will return -1 if the symbol already exist
 *  as well as print an error.
 */
int local_scope_add(char* symbol, int type, char* file, int line)
{
    if(!(program_options & TYPE_OPTION))
        return 0;
    
    symbol_imp_t *sym;

    if(local_symbols == NULL)
        local_symbols = hashmap_new();
    if( hashmap_get(local_symbols, symbol, (void**)&sym) == MAP_OK ) 
    {
        print_type_error(file, sym->file, symbol, sym->type, sym->line, line);
        return -1;
    }
    else
    {
        sym = create_symbol(symbol, type, line, file, NULL);
        hashmap_put(local_symbols, symbol, sym);
    }

    return 0; 
}

/**
 *  returns the current local symbol table
 *  and sets the static symbol table variable 
 *  to NULL so that a new hashmap is created 
 *  on the next attempted insert into the local
 *  symbol table
 */
symbol_table_t close_scope()
{
    if(!(program_options & TYPE_OPTION))
        return NULL;

    symbol_table_t ans = local_symbols;
    local_symbols = NULL;
    return ans;
}

/**
 *  returns the current global symbol table
 *  and sets the static symbol table variable 
 *  to NULL so that a new hashmap is created 
 *  on the next attempted insert into the global
 *  symbol table
 */
symbol_table_t close_global_scope()
{
    if(!(program_options & TYPE_OPTION))
        return NULL;

    symbol_table_t ans = global_symbols;
    global_symbols = NULL;
    return ans;
}

/**
 *  Attempts to find a symbol in the current
 *  local and global symbol table.
 *  if the symbol is found then it is returned otherwise
 *  ERROR type is returned 
 */
int find_type(char* symbol)
{
    if(!(program_options & TYPE_OPTION))
        return 0;

    symbol_imp_t *sym;

    if( local_symbols && hashmap_get(local_symbols, symbol, (void**)&sym) == MAP_OK )
        return sym->type;
    if( global_symbols && hashmap_get(global_symbols, symbol, (void**)&sym) == MAP_OK )
        return sym->type;

    printf("Error in %s line %d:\n\tImplicit declaration of variable \"%s\"\n", 
        parse_file_string, yyline, symbol
    ); 
    return ERROR;
}

/**
 *  This will be for searching a symbol table that is  
 *  no longer the current global or local static symbol table
 */
int find_type_in_table(symbol_table_t t, char* symbol)
{
    if(!(program_options & TYPE_OPTION))
        return 0;

    symbol_imp_t *sym;

    if( hashmap_get(t, symbol, (void**)&sym) == MAP_OK )
        return sym->type;
    return ERROR;
}

int resolve_bop_type(int op, int type1, int type2)
{
    char op_str[20], t1[20], t2[20];
    int ans;

    if(!(program_options & TYPE_OPTION))
        return 0;
    
    switch(op)
    {
        case '-':
        case '+':
        case '*':
        case '/':
            if( (type1 & (INT | FLOAT | CHAR)) && (type2 & (INT | FLOAT | CHAR)) )
                ans = type_coerce(type1, type2);
            else
                ans = ERROR;
            if(ans != ERROR)
                return ans;
            break;
        case '%':
        case '|':
        case '&':
            if( (type1 & (INT | CHAR)) && (type2 & (INT | CHAR)) )
                ans = type_coerce(type1, type2);
            else
                ans = ERROR;
            if(ans != ERROR)
                return ans;
            break;
        case EQUAL:
        case NEQUAL:
        case '>':
        case GE:
        case '<':
        case LE:
        case DPIPE:
        case DAMP:
            if( (type1 & (INT | FLOAT | CHAR)) && (type2 & (INT | FLOAT |  CHAR)) &&
                !(type1 & ARRAY) && !(type2 & ARRAY)
            )
                ans = CHAR;
            else
                ans = ERROR;
            if(ans != ERROR)
                return ans;
            break;
        case '=':
        case PLUSASSIGN:
        case MINUSASSIGN:
        case STARASSIGN:
        case SLASHASSIGN:
            if( (type1 & (INT | FLOAT | CHAR)) && (type2 & (INT | FLOAT | CHAR)) )
                //make sure the coerced type is the assignment type so storage size is large enough
                ans = type1 & type_coerce(type1, type2);
            else
                ans = ERROR;
            
            if(ans != ERROR)
                return ans;
            break;
        case '[':
            if(type1 & ARRAY && type_coerce(INT, type2))
                return type1 ^ ARRAY;
            break;
    }
        
    tok_to_str(op_str, op);
    type_to_str(t1, type1);
    type_to_str(t2, type2);
    fprintf(stderr, "Error in %s line %d:\n\tOperation not supported \"%s %s %s\"\n",
        parse_file_string, yyline, t1, op_str, t2
    );
    error_in_types = 1;

    return ERROR;
}

int resolve_uop_type(int op, int type)
{
    char op_str[20], t[20];    

    if(!(program_options & TYPE_OPTION))
        return 0;

    switch(op)
    {
        case '[':
            //if its not an array its an error
            if(!(type & ARRAY))
                break;
            else 
                //return the same type with the scope, array, and function types masked off
                return (~(ARRAY | FUNC_MASK | SCOPE_MASK)) & type;
        case '~':
            if( (type & (CHAR | INT)) && !(type & ARRAY) )
                return TYPE_MASK & type;
            else
                break;
        case '-':
        case INCR:
        case DECR:
            if( (type & (CHAR | FLOAT | INT)) && !(type & ARRAY) )
                return (TYPE_MASK & type);
            else
                break;
        case '!':
            if( (type & (CHAR | FLOAT | INT)) && !(type & ARRAY) )
                return CHAR;
            else
                break;
        case '&':
            if( (type & (CHAR | FLOAT | INT)) && !(type & ARRAY) )
                return (TYPE_MASK & type) | ARRAY;
            else
                break;
    }

    tok_to_str(op_str, op);
    type_to_str(t, type);
    fprintf(stderr, "Error in %s line %d:\n\tUnary Operator \"%s\" cannot be applied to type %s\n",
        parse_file_string, yyline, op_str, t 
    );
    error_in_types = 1;

    return ERROR;
}

int resolve_turnary_type(int t1, int t2, int t3)
{
    int ans; 
    char c1[20], c2[20], c3[20];

    if(!(program_options & TYPE_OPTION))
        return 0;
    
    if(!(t1 & ARRAY) && (t1 & (INT | CHAR | FLOAT)) )
    {
        switch(t2 & TYPE_MASK)
        {
            case CHAR:
            case INT:
            case FLOAT:
                ans = type_coerce(t2, t3);
                if(ans != ERROR)
                    return ans;
                break;
            case CHAR | ARRAY:
            case FLOAT | ARRAY:
            case INT | ARRAY:
                if( (t2 & TYPE_MASK) == (t3 & TYPE_MASK) )
                    return t2 & TYPE_MASK;
        }
    }

    type_to_str(c1, t1);
    type_to_str(c2, t2);
    type_to_str(c3, t3);

    fprintf(stderr, "Error in %s line %d:\n\tTurnary not supported with types \"%s ? %s : %s\"\n",
        parse_file_string, yyline, c1, c2, c3
    );
    error_in_types = 1;

    return ERROR;
}

int match_params(char *symbol, int *params, int length)
{
    symbol_imp_t *sym;
    int i, j, matches = 0;
    if(!(program_options & TYPE_OPTION) || global_symbols == NULL)
        return -1;
    
    if(hashmap_get(global_symbols, symbol, (void**) &sym) != MAP_OK)
        return -1;
    for(i = j = 0; i < sym->num_params; i++)
    {
        //test against arg params
        for(j = 0; j < length && i < sym->num_params; j++)
        {
            //if the current test is ending
            if(sym->params[i] & FUNC_MASK)
            {
                //if a prototype is getting a definition
                if(sym->params[i] == PROTO_TYPE && params[j] == DEF_TYPE)
                {
                    matches++;
                }//if there is a conflict
                else if(sym->params[i] == DEF_TYPE && params[j] == DEF_TYPE)
                {
                    matches++;
                }//if its a redundant prototype
                else if(params[j] == PROTO_TYPE)
                {
                    matches++;
                }//the current test has ended
                else
                {
                    break;
                }
            }//we are still testing types
            else
            {
                //move to next test if there is a mismatch
                if(type_coerce(sym->params[i], params[j]) == ERROR)
                    break;
                i++;
            }
        }
        //set up next test
        while(i < sym->num_params && !(sym->params[i] & FUNC_MASK))
            i++;
    }

    if(matches > 1)
    {
        error_in_types = 1;
        fprintf(stderr, "Error in file %s line %d:\n\tambiguous function call multiple defintions match argument types\n", parse_file_string, yyline);
    }
    else if(matches == 0)
    {
        error_in_types = 1;
        fprintf(stderr, "Error in file %s line %d:\n\tno definition for function matches argument types\n", parse_file_string, yyline);
    }
    else
        return 0;
    return -1;
}

void set_return_type(int type)
{
    if(!(program_options & TYPE_OPTION))
        return;
    return_type = type;
}

void check_return_type(int type)
{
    char t1[20], t2[20];

    if(!(program_options & TYPE_OPTION))
        return;

    type_to_str(t1, return_type);
    type_to_str(t2, type);

    if( type_coerce(return_type, type) == ERROR )
    {
        error_in_types = 1;
        fprintf(stderr, "Error in %s line %d:\n\treturn type: %s cannot be coerced into function return type of %s",
            parse_file_string, yyline, t1, t2
        ); 
    }
}

void print_expression_type(int type)
{
    char c[20];

    if( !(program_options & TYPE_OPTION) || !(program_options & TYPE_OUTPUT_OPTION) || type == ERROR )
        return;
    type_to_str(c, type);
    printf("Expression in file %s at line %d has type %s\n", parse_file_string, yyline, c);
}

static void print_type_error(char* cur_file, char* old_file, char* symbol, int type, int old_line, int new_line)
{
    char temp[20];

    type_to_str(temp, type);
    error_in_types = 1;
    fprintf(stderr, "Error in %s line %d:\n\tlocal variable %s already declared as:\n\t  %s %s (near line %d in file %s)\n",
         cur_file, new_line, symbol, temp, symbol, 
         old_line, old_file 
    );   
}

static symbol_imp_t *create_symbol(char *symbol, int type, int line, char *file, int *params)
{
    symbol_imp_t *sym;
    int i, length = 0;
    
    sym =  calloc(1, sizeof(symbol_imp_t));
    if(sym == NULL)
    {
        fprintf(stderr, "failed to allocate memory for symbol\n");
        return NULL;
    }
    
    if(params != NULL)
    {
        while(!(params[length] & FUNC_MASK))
            length++;
        length++;
        
        sym->params = malloc(sizeof(int) * length);
        sym->num_params = length;

        for(i = 0; i < length; i++)
            sym->params[i] = params[i];
    }
    else
    {
        sym->params = NULL;
    }
    
    sym->symbol = symbol;
    sym->type = type;
    sym->line = line;
    sym->file = file;
    return sym;
}


/**
 *  attemps to update the params array to contain the new 
 *  param list. If sucessful then return 0 if there is a 
 *  conflict return -1, if there is a memory allocation
 *  error return -2 invalid args returns -3
 */
static int update_params(symbol_imp_t *sym, int *params)
{
    int *temp;
    int length=0;
    int i, j;

    if(sym->params == NULL || params == NULL)
        return -3;

    while(!(params[length] & FUNC_MASK))
        length++;
    length++;
    
    for(i = j = 0; i < sym->num_params; i++)
    {
        //test against arg params
        for(j = 0; j < length && i < sym->num_params; j++)
        {
            //if the current test is ending
            if(sym->params[i] & FUNC_MASK)
            {
                //if a prototype is getting a definition
                if(sym->params[i] == PROTO_TYPE && params[j] == DEF_TYPE)
                {
                    sym->params[i] = DEF_TYPE;
                    return 0;
                }//if there is a conflict
                else if(sym->params[i] == DEF_TYPE && params[j] == DEF_TYPE)
                {
                    return -1;
                }//if its a redundant prototype
                else if(params[j] == PROTO_TYPE)
                {
                    return 0;
                }//the current test has ended
                else
                {
                    break;
                }
            }//we are still testing types
            else
            {
                //move to next test if there is a mismatch
                if( (sym->params[i] & TYPE_MASK) != (params[j] & TYPE_MASK) )
                    break;
                i++;
            }
        }
        //set up next test
        while(i < sym->num_params && !(sym->params[i] & FUNC_MASK))
            i++;
    }

    temp = realloc(sym->params, (sym->num_params + length) * sizeof(int));

    sym->params = temp;

    for(i = 0; i < length; i++)
    {
        sym->params[i+sym->num_params]  = params[i];
    }
    sym->num_params += length;
    return 0;
}

static int type_coerce(int t1, int t2)
{
    //cannot coerce arrays
    if( (t1 & ARRAY) || (t2 & ARRAY) )
        return ERROR;
    //if one is void they both must be void
    else if( (t1 & VOID) || (t2 & VOID) )
        return TYPE_MASK & t1 & t2;
    //if not void or array then must be int float or char
    //so pick the largest present
    else if( (t1 & FLOAT) || (t2 & FLOAT) )
        return FLOAT;
    else if( (t1 & INT) || (t2 & INT) )
        return INT;
    else if( (t1 & CHAR) || (t2 & CHAR) )
        //this should be able be else but would rather have error than incorrect char
        return CHAR;
    else
        //shouldn't be reachable but just in case
        return ERROR;
}

#ifdef TEST_SYMBOL_TABLE

char *parse_file_string = "TEST";
int yyline = 0;
unsigned long int program_options = TYPE_OPTION;

void update_params_test();
void type_coerce_test();
void bop_test();
void uop_test();
void turnary_test();

int main(int argc, char **argv)
{
    int i;

    for(i = 0; i < argc; i++) 
    {
        if(!strcmp("--bop", argv[i]))
            bop_test();
        else if(!strcmp("--param", argv[i]))
            update_params_test();
        else if(!strcmp("--uop", argv[i]))
            uop_test();
        else if(!strcmp("--coerce", argv[i]))
            type_coerce_test();
        else if(!strcmp("--turnary", argv[i]))
            turnary_test();
    }
}

void turnary_test()
{
    int i, j, k;
    int type[7] = { CHAR, CHAR | ARRAY, INT, INT | ARRAY, FLOAT, FLOAT | ARRAY, VOID };
    char c1[20], c2[20], c3[20], ans[20];

    printf("==========TURNARY TEST==========\n");
    
    for(i = 0; i < 7; i++)
    {    
        type_to_str(c1, type[i]);
        for(j = 0; j < 7; j++)
        {
            type_to_str(c2, type[j]);
            for(k = 0; k < 7; k++)
            {
                type_to_str(c3, type[k]);
                type_to_str(ans, resolve_turnary_type(type[i], type[j], type[k]));
                printf("\"%s ? %s : %s\" = %s\n", c1, c2, c3, ans);
            }
            printf("\n");
        }
        printf("\n");
    }
}

//test all 144 combinations of types and boolean operator classes
void bop_test()
{
    int i, j, k;
    char co[20], c1[20], c2[20], ans[20];
    int op[4] = { '=', '+', EQUAL, '&' };
    int type[7] = { CHAR, CHAR | ARRAY, INT, INT | ARRAY, FLOAT, FLOAT | ARRAY, VOID };

    printf("==========BOP TEST==========\n"); 

    for(i = 0; i < 4; i++)
    {
        tok_to_str(co, op[i]);
        for(j = 0; j < 7; j++)
        {
            type_to_str(c1, type[j]);
            for(k = 0; k < 7; k++)
            {
                type_to_str(c2, type[k]);
                type_to_str(ans, resolve_bop_type(op[i], type[j], type[k]));
                printf("\"%s %s %s\" is type: %s\n", c1, co, c2, ans);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void uop_test()
{
    int i, j;
    char co[20], c[20], ans[20];
    int op[5] = { '-', '!', '[', '&', '~'};
    int type[7] = { CHAR, CHAR | ARRAY, INT, INT | ARRAY, FLOAT, FLOAT | ARRAY, VOID };
    
    printf("==========UOP TEST==========\n"); 

    for(i = 0; i < 5; i++)
    {
        tok_to_str(co, op[i]);
        for(j = 0; j < 7; j++)
        {
            type_to_str(c, type[j]);
            type_to_str(ans, resolve_uop_type(op[i], type[j]));
            printf("\"%s %s\" is type: %s\n", co, c, ans);
        }
        printf("\n");
    }
}

void type_coerce_test()
{
    int i;
    char c1[20], c2[20], ans[20];
    int t1[5] = { INT | ARRAY, INT | DEF_TYPE, CHAR | EXTERN, VOID, FLOAT };
    int t2[5] = { CHAR, FLOAT, CHAR, VOID, VOID };

    printf("==========COERCE TEST==========\n"); 

    for(i = 0; i < 5; i++)
    {
        type_to_str(ans, type_coerce(t1[i],t2[i]));
        type_to_str(c1, t1[i]);
        type_to_str(c2, t2[i]);
        printf("coerce(%s,%s) = %s\n", c1,c2,ans);
    }
}

void update_params_test()
{
    symbol_imp_t sym;
    char temp[20];
    int i; 
    int params[3] = {INT, CHAR | ARRAY, DEF_TYPE};
    int params2[20] = {
        INT, CHAR, DEF_TYPE, 
        PROTO_TYPE, 
        INT | ARRAY, CHAR, DEF_TYPE,
        INT, INT, INT, CHAR, INT, DEF_TYPE,
        FLOAT, PROTO_TYPE,
        INT, FLOAT, PROTO_TYPE,
        CHAR, DEF_TYPE
    };
    
    printf("==========PARAM TEST==========\n");

    sym.num_params = 20;
    sym.params = malloc(sizeof(int) * 20);
    for(i = 0; i < 20; i++)
    {
        sym.params[i] = params2[i];
    }

    printf("update return value %d: symbols:\n\t", update_params(&sym, params));
    for(i = 0; i < sym.num_params; i++)
    {
        type_to_str(temp, sym.params[i]);
        printf("%s, ", temp);
    }

    printf("\n");
}


#endif
