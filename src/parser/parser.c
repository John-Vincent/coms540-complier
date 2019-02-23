#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../../includes/parser.h"
#include "../../bin/parser/bison.h"
#include "../../includes/types.h"
#include "../../includes/main.h"
#include "../../includes/lexer.h"

//have to have this for the yyerror function to print the file
static char* file_string;

/*
 * this function takes a main program node and then
 * searches through its children in the ast to print
 * the information required for this assignment
 */
static void print_parser_output(ast_node_t node);

void yyerror(const char *error)
{
    fprintf(stderr, "Syntax error in %s, line %d:\n\t%s\n", file_string, yyline, error);
}

ast_node_t *parse_input(int num_files, char **files)
{
    FILE *file;
    int i;
    ast_node_t *units;

    units = calloc(num_files, sizeof(ast_node_t));

    if(units == NULL)
    {
        fprintf(stderr, "failed to allcate space for parse trees");
        return NULL;
    }
    
    yyast.type = PROGRAM;

    for(i = 0; i < num_files; i++)
    {
        file = fopen(files[i], "r");
        file_string = files[i];

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

    return units;
}

ast_node_t *new_ast_node(int type, int num_children, ast_node_t *children, ast_value_t value, int array_size)
{
    char tok[20];
    if( program_options & PARSER_DEBUG_OPTION )
    {
        tok_to_str(tok, type);
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
    new->type = type;
    new->value = value;
    new->array_size = array_size;
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
    
    value.i = scope | type;

    ans = new_ast_node(VARIABLE, kids, NULL, value, 0);
    cur = identifiers;
    i = 0; 

    while(cur)
    {
        ans->children[i] = *cur;
        last = cur;
        cur = cur->children;
        ans->children[i].num_children = 0;
        ans->children[i].children = NULL;
        free(last);
        i++;
    }
    
    return ans;
}

ast_node_t *make_node_list(int type, ast_node_t *list)
{
    ast_node_t *cur = list, next, *ans;
    int kids = 1, i;

    while(cur->type == NODE_LIST)
    {
        kids++;
        cur = cur->children + 1;
    }

    ans = new_ast_node(type, kids, NULL, (ast_value_t)0, 0);
    cur = list;

    for(i = 0; i < kids; i++)
    {
        ans->children[i] = cur->children[0];
        if(cur->type == NODE_LIST)
            cur = cur->children + 1;
    }

    cur = list->children;
    next = *list;

    while(next.type == NODE_LIST)
    {
        next = cur[1];
        free(cur);
        cur = next.children;
    }

    free(cur);
    free(list);

    return ans;
}

void add_ast_children(ast_node_t *parent, ast_node_t *children, int num_children)
{
    void *temp;    
    char tok[20];

    if(program_options & PARSER_DEBUG_OPTION)
    {
        tok_to_str(tok, parent->type);
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

void print_node(ast_node_t node, int depth, void *arg)
{
    char tok[20], type[20];

    tok_to_str(tok, node.type);

    switch(node.type)
    {
        case TYPE_NAME:
        case IDENT:
            if(node.array_size)
                printf("%*ctype: %s, value: %s, array size %d, children %d\n", 2*depth, ' ', tok, node.value.s, node.array_size, node.num_children);
            else
                printf("%*ctype: %s, value: %s, children %d\n", 2*depth, ' ', tok, node.value.s, node.num_children);
            break;
        case TYPE:
        case VARIABLE:
            type_to_str(type, node.value.i);
            printf("%*ctype: %s, var type: %s, children %d\n", 2*depth, ' ', tok, type, node.num_children);
            break;
        default:
            printf("%*ctype: %s, children %d\n", 2*depth, ' ', tok, node.num_children);
            break;
    }
}

static void print_parser_output(ast_node_t node)
{
    int i, j;
    ast_node_t cur;

    printf("Global Variables: ");
    //loops through main program children to find global variables
    for(i = 0; i < node.num_children; i++)
    {
        if(node.children[i].type == VARIABLE)
        {
            cur = node.children[i];
            for(j = 0; j < cur.num_children-1; j++)
            {
                printf("%s, ", cur.children[j].value.s);
            }
            if(cur.num_children != 0)
                printf("%s", cur.children[j].value.s);
        }
    }

    printf("\n\n");
    //finds all functions in main program then prints the data in their children
    for(i = 0; i < node.num_children; i++)
    {
        cur = node.children[i];
        if( cur.type == FUNCTION_PROTO || cur.type == FUNCTION_DEF)
        {
            printf("Function: %s\n", cur.children[0].value.s);
            printf("\tParameters: ");

            cur = node.children[i].children[1];
            for(j = 0; j < cur.num_children-1; j++)
            {
                printf("%s, ", cur.children[j].value.s);
            }
            if(cur.num_children != 0)
                printf("%s, ", cur.children[j].value.s);
            
            printf("\n\tLocal vars: ");
            if( cur.type == FUNCTION_DEF)
            {
                cur = node.children[i].children[2];
                for(j = 0; j < cur.num_children-1; j++)
                {
                    printf("%s, ", cur.children[j].value.s);
                }
                if(cur.num_children != 0)
                    printf("%s", cur.children[j].value.s);
            }
            printf("\n\n");
        }
    }

}
