#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

static symbol_table_t global_symbols = NULL;

static symbol_table_t local_symbols = NULL;

static int return_type = 0;

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

    if( hashmap_get(local_symbols, symbol, (void**)&sym) == MAP_OK )
        return sym->type;
    if( hashmap_get(global_symbols, symbol, (void**)&sym) == MAP_OK )
        return sym->type;
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

    if(!(program_options & TYPE_OPTION))
        return 0;
    
    tok_to_str(op_str, op);
    type_to_str(t1, type1);
    type_to_str(t2, type2);
    
    switch(op)
    {
        case '-':
            break;
        case EQUAL:
            break;
        case NEQUAL:
            break;
        case '>':
            break;
        case GE:
            break;
        case '<':
            break;
        case LE:
            break;
        case '+':
            break;
        case '*':
            break;
        case '/':
            break;
        case '%':
            break;
        case '|':
            break;
        case '&':
            break;
        case DPIPE:
            break;
        case DAMP:
            break;
        case '=':
            break;
        case PLUSASSIGN:
            break;
        case MINUSASSIGN:
            break;
        case STARASSIGN:
            break;
        case SLASHASSIGN:
            break;
    }

    return ERROR;
}

int resolve_uop_type(int op, int type)
{
    char op_str[3], t[20];    

    if(!(program_options & TYPE_OPTION))
        return 0;

    tok_to_str(op_str, op);
    type_to_str(t, type);

    switch(op)
    {
        case '[':
            break;
    }

    return ERROR;
}

int resolve_turnary_type(int t1, int t2, int t3)
{
    if(!(program_options & TYPE_OPTION))
        return 0;

    return 0;
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

    if( (type & TYPE_MASK) != (return_type & TYPE_MASK) )
        fprintf(stderr, "Error in %s line %d:\n\treturn type: %s, does not match function return type of %s",
            parse_file_string, yyline, t1, t2
        ); 
}

void print_expression_type(int type)
{
    if(!(program_options & TYPE_OPTION))
        return;

}

static void print_type_error(char* cur_file, char* old_file, char* symbol, int type, int old_line, int new_line)
{
    char temp[20];

    type_to_str(temp, type);
    fprintf(stderr, "Error in %s line %d:\n\tlocal variable %s already declared as:\n\t  %s %s %s (near line %d in file %s)\n",
         cur_file, new_line, symbol, temp, symbol, 
         type & ARRAY ? ARRAY_STRING : "", 
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
        fprintf(stderr, "failed to allocate memory for symbol");
        return NULL;
    }
    
    while(!(params[length] & FUNC_MASK))
        length++;
    length++;
    
    sym->params = malloc(sizeof(int) * length);

    for(i = 0; i < length; i++)
        sym->params[i] = params[i];

    sym->symbol = symbol;
    sym->type = type;
    sym->line = line;
    sym->file = file;
    sym->params = params;
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

#ifdef TEST_SYMBOL_TABLE

int main()
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
