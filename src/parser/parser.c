#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../../includes/symbol_table.h"
#include "../../includes/parser.h"
#include "../../bin/parser/bison.h"
#include "../../includes/types.h"
#include "../../includes/utils.h"
#include "../../includes/main.h"
#include "../../includes/lexer.h"

//have to have this for the yyerror function to print the file
char* parse_file_string;

/*
 * this function takes a main program node and then
 * searches through its children in the ast to print
 * the information required for this assignment
 */
static void print_parser_output(ast_node_t node);

static void free_memory(ast_node_t node, int depth, void *arg);

void yyerror(const char *error)
{
    fprintf(stderr, "Syntax error in %s, line %d:\n\t%s\n", parse_file_string, yyline, error);
}

ast_node_t *parse_input(int num_files, char **files)
{
    FILE *file;
    int i;
    ast_node_t *units;

    units = calloc(num_files, sizeof(ast_node_t));

    i = init_symbol_table();

    if(i)
    {
        fprintf(stderr, "failed to initalize symbol table\n");
        return NULL;
    }

    if(units == NULL)
    {
        fprintf(stderr, "failed to allcate space for parse trees\n");
        return NULL;
    }
    
    yyast.token = PROGRAM;

    for(i = 0; i < num_files; i++)
    {
        file = fopen(files[i], "r");
        parse_file_string = files[i];

        if(!file)
        {
            fprintf(stderr, "Error opening file %s:\n %s\n", files[i], strerror(errno));
            continue;
        }

        yyline = 1;
        
        yyrestart(file);
        yyparse();
        fclose(file);
        yypop_buffer_state();
        units[i] = yyast;
        yyast.num_children = 0;
        yyast.children = NULL;
        if( program_options & PARSER_TREE_OPTION )
            preorder_traversal(units[i], 1, &print_node, NULL);
        if( program_options & PARSER_OUTPUT_OPTION )
            print_parser_output(units[i]);
    }
    
    //don't continue if type errors exist
    if(has_type_error())
    {
        for(i = 0; i < num_files; i++)
        {
            free_tree_memory(units[i]);
        }
        free(units);
        return NULL;
    }

    return units;
}

ast_node_t *new_ast_node(int token, int type, int num_children, ast_node_t *children, ast_value_t value, int array_size)
{
    char tok[20];
    if( program_options & PARSER_DEBUG_OPTION )
    {
        tok_to_str(tok, token);
        printf("creating ast node of type %s, value %d, array size %d\n", tok, value.i, array_size); 
    }
    ast_node_t *new;

    new = malloc(sizeof(ast_node_t));
    if(new == NULL)
    {
        fprintf(stderr, "failed to allocate memory for new ast node\n");
        return NULL;
    }
    new->num_children = num_children;
    new->token = token;
    new->value = value;
    new->type = type;
    new->array_size = array_size;
    new->line_number = yyline;
    if(num_children && !children)
    {
        new->children = calloc(num_children, sizeof(ast_node_t));
    }
    else
    {
        new->children = children;
    }
    return new;
}

ast_node_t *new_variable_node(int scope, int type, ast_node_t *identifiers)
{
    int kids = 0, i;
    ast_node_t *cur = identifiers, *ans, *last;
    ast_value_t value;

    if( program_options & PARSER_DEBUG_OPTION )
    {
        printf("creating variable of type %d, scope %d\n", type, scope>>30); 
    }

    while(cur)
    {
        kids++;
        cur = cur->children;
    }

    value.i = 0; 

    ans = new_ast_node(VARIABLE, type, kids, NULL, value, 0);
    cur = identifiers;
    i = 0; 

    while(cur)
    {
        ans->children[i] = *cur;
        if(ans->children[i].array_size)
            ans->children[i].type = type | ARRAY;
        else
            ans->children[i].type = type;
        last = cur;
        cur = cur->children;
        ans->children[i].num_children = 0;
        ans->children[i].children = NULL;
        free(last);
        i++;
    }
    ans->line_number = yyline; 
    return ans;
}

ast_node_t *make_function_sig(ast_node_t *type_name, ast_node_t *params, int type)
{
    int *p, i;
    ast_node_t *func = type_name;
    
    type_name = realloc(params, sizeof(ast_node_t) * 2);

    if(type_name == NULL)
    {
        fprintf(stderr, "failed to allocate memory for function identifier node\n"); 
        exit(-1);
    }

    func->num_children = 2;
    func->children = type_name;
    params = type_name + 1;
    *params = *type_name;
    type_name->token = IDENT;
    type_name->num_children = 0;
    type_name->children = NULL;
    type_name->value.s = func->value.s;
    type_name->type = CHAR | ARRAY;
    type_name->line_number = yyline;

    p = malloc(sizeof(int) * (params->num_children + 1));

    for(i = 0; i < params->num_children; i++)
    {
        p[i] = params->children[i].type;
        local_scope_add(params->children[i].value.s, params->children[i].type, parse_file_string, yyline);
    }

    func->token = type;

    if(type == FUNCTION_PROTO)
        p[i] = PROTO_TYPE;
    else
        p[i] = DEF_TYPE;

    global_scope_add(type_name->value.s, func->type, parse_file_string, yyline, p);
    set_return_type(func->type);
    free(p);

    return func;
}

ast_node_t *invert_list(ast_node_t *node)
{
    int i;
    ast_node_t temp;

    for(i = 0; i < node->num_children/2; i++)
    {
        temp = node->children[i];
        node->children[i] = node->children[node->num_children-i-1];
        node->children[node->num_children-i-1] = temp;
    }

    return node;
}

void check_function_params(ast_node_t *node)
{
    int i, length = 1;
    int *params;
    
    
    if(node->children != NULL)
    {
        length += node->children[0].num_children;
        params = malloc(sizeof(int) * length);

        for(i = 0; i < node->children[0].num_children; i++)
        {
            params[i] = node->children[0].children[i].type;    
        }
    }
    else
        params = malloc(sizeof(int));

    params[node->num_children] = DEF_TYPE;

    if(match_params(node->value.s, params, length))
        node->type = ERROR;
}

void process_declaration(ast_node_t *node, int local) 
{
    int i;

    if(node->token == VARIABLE_LIST)
    {
        for(i = 0; i < node->num_children; i++)
        {
            process_declaration(node->children + i, local);
        }
    }
    else
    {
        for(i = 0; i < node->num_children; i++)
        {
            if(local)
                local_scope_add(
                    node->children[i].value.s, 
                    node->children[i].type, 
                    parse_file_string, 
                    node->children[i].line_number
                );
            else
                global_scope_add(
                    node->children[i].value.s, 
                    node->children[i].type, 
                    parse_file_string, 
                    node->children[i].line_number,
                    NULL
                );
        }
    }
}

void add_ast_children(ast_node_t *parent, ast_node_t *children, int num_children)
{
    void *temp;    
    char tok[20];

    if(program_options & PARSER_DEBUG_OPTION)
    {
        tok_to_str(tok, parent->token );
        printf("adding %d children to node of type %s\n", num_children, tok);
    }

    temp = realloc(parent->children, (num_children + parent->num_children) * sizeof(ast_node_t));
    if(temp == NULL)
    {
        fprintf(stderr, "failed to allocate memory for new ast children\n");
        return;
    }

    parent->children = temp;
    memcpy(parent->children + parent->num_children, children, sizeof(ast_node_t) * num_children);
    parent->num_children = parent->num_children + num_children;
    free(children);
}

void preorder_traversal(ast_node_t node, int depth, void (*func)(ast_node_t, int, void*), void *arg)
{
    int i; 

    func(node, depth, arg);
    for(i = 0; i < node.num_children; i++)
    {
        preorder_traversal(node.children[i], depth+1, func, arg);
    }
}

void postorder_traversal(ast_node_t node, int depth, void (*func)(ast_node_t, int, void*), void *arg)
{
    int i;

    for(i = 0; i < node.num_children; i++)
    {
        postorder_traversal(node.children[i], depth+1, func, arg);
    }
    func(node, depth, arg);
}

void print_node(ast_node_t node, int depth, void *arg)
{
    char tok[20], type[20];

    tok_to_str(tok, node.token);

    switch(node.token)
    {
        //nodes with int values
        case INTCONST: 
            printf("%*ctype: %s, value: %d, children %d\n", 2*depth, ' ', tok, node.value.i, node.num_children);
            break;
        //nodes with char values
        case CHARCONST:
            printf("%*ctype: %s, value: %c, children %d\n", 2*depth, ' ', tok, node.value.c, node.num_children);
            break;
        //ndoes with real values
        case REALCONST:
            printf("%*ctype: %s, value: %f, children %d\n", 2*depth, ' ', tok, node.value.f, node.num_children);
            break;
        //nodes with string values
        case STRCONST:
        case TYPE_NAME:
        case IDENT:
        case FUNCTION_CALL:
        case LVALUE:
            type_to_str(type, node.type);
            if(node.array_size)
                printf("%*ctoken: %s, value: %s, type: %s, array size %d, children %d\n", 
                    2*depth, ' ', tok, node.value.s, type, node.array_size, node.num_children
                );
            else
                printf("%*ctoken: %s, value: %s, type: %s, children %d\n", 
                    2*depth, ' ', tok, node.value.s, type, node.num_children
                );
            break;
        //special case node with type rep as value
        case TYPE:
        case VARIABLE:
            type_to_str(type, node.type);
            printf("%*ctoken: %s, var type: %s, children %d\n", 2*depth, ' ', tok, type, node.num_children);
            break;
        //sepcial case node with op type as value
        case BINARY_OP:
            tok_to_str(type, node.value.i);
            printf("%*ctoken: %s, op: %s, children %d\n", 2*depth, ' ', tok, type, node.num_children);
            break;
        default:
            type_to_str(type, node.type);
            printf("%*ctoken: %s, type: %s, children %d\n", 2*depth, ' ', tok, type, node.num_children);
            break;
    }
}

static void print_parser_output(ast_node_t node)
{
    int i, j, k, p=0;
    ast_node_t cur, cur_2;

    printf("Global Variables: ");
    //loops through main program children to find global variables
    for(i = 0; i < node.num_children; i++)
    {
        if(node.children[i].token == VARIABLE)
        {
            cur = node.children[i];
            for(j = 0; j < cur.num_children; j++)
            {
                if(p!=0)
                    printf(", ");
                printf("%s", cur.children[j].value.s);
                p++;
            }
        }
    }

    printf("\n\n");
    //finds all functions in main program then prints the data in their children
    for(i = 0; i < node.num_children; i++)
    {
        cur = node.children[i];
        if( cur.token == FUNCTION_PROTO || cur.token == FUNCTION_DEF)
        {
            printf("Function: %s\n", cur.children[0].value.s);
            printf("\tParameters: ");

            cur = node.children[i].children[1];
            for(j = 0; j < cur.num_children-1; j++)
            {
                printf("%s, ", cur.children[j].value.s);
            }
            if(cur.num_children != 0)
                printf("%s", cur.children[j].value.s);
            
            if( node.children[i].token == FUNCTION_DEF)
            {
                printf("\n\tLocal vars: ");
                cur = node.children[i].children[2];
                for(j = 0; j < cur.num_children; j++)
                {
                    cur_2 = cur.children[j];
                    for(k = 0; k < cur_2.num_children; k++)
                    {
                        if(k == 0 && j == 0) 
                            printf("%s", cur_2.children[k].value.s);
                        else
                            printf(", %s", cur_2.children[k].value.s);
                    }
                }
            }
            printf("\n\n");
        }
    }
}

static void free_memory(ast_node_t node, int depth, void *arg)
{
    if(node.num_children && node.children)
        free(node.children);
    
    if(node.token == TYPE_NAME || node.token == IDENT || node.token == FUNCTION_CALL || node.token == LVALUE)
        free(node.value.s);
}

void free_tree_memory(ast_node_t node)
{
    postorder_traversal(node, 1, &free_memory, NULL); 
}

