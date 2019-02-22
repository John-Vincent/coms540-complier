#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../../includes/parser.h"
#include "../../bin/parser/bison.h"
#include "../../includes/main.h"
#include "../../includes/lexer.h"

void yyerror(const char *error)
{
    fprintf(stderr, "%s\n", error);
}

int parse_input(int num_files, char **files)
{
    FILE *file;
    int i;

    for(i = 0; i < num_files; i++)
    {
        file = fopen(files[i], "r");
                
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
    }

    return -1;
}

ast_node_t *new_ast_node(int type, int num_children, ast_node_t *children, ast_value_t value, int array_size)
{
    char tok[20];
    if( program_options & parser_debug_option )
    {
        tok_to_str(tok, type);
        printf("creating ast node of type %s, and value %d", tok, value.i); 
    }
    ast_node_t *new;

    new = malloc(sizeof(ast_node_t));
    if(new == NULL)
    {
        fprintf(stderr, "failed to allocate memory for new ast node");
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
    int kids = 1;
    ast_node_t *cur = identifiers, ans;

    while(cur->num_children)
    {
        kids++;
        cur = cur->children;
    }
   
    ans = new_ast_node(VARIABLE, kids, NULL, scope | type, 0); 
    
    return ans;
}

void add_ast_children(ast_node_t *parent, ast_node_t *children, int num_children)
{
    void *temp;    

    temp = realloc(parent->children, (num_children + parent->num_children) * sizeof(ast_node_t));
    if(temp == NULL)
    {
        fprintf(stderr, "failed to allocate memory for new ast children");
        return;
    }

    parent->children = temp;
    memcpy(parent->children + parent->num_children, children, sizeof(ast_node_t) * num_children);
    parent->num_children = parent->num_children;
    free(children);
}
