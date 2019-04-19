#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../includes/hashmap.h"
#include "../../bin/parser/bison.h"
#include "../../includes/intermediate_generator.h"
#include "../../includes/types.h"
#include "../../includes/main.h"
#include "../../includes/utils.h"
#define FUNC_OFFSET 2

static int generate_constants(ast_node_t *parse_trees, int num_trees, map_t map);

static int generate_globals(ast_node_t *parse_trees, int num_trees, map_t map);

static int generate_functions(ast_node_t *parse_tree, int num_trees, map_t map);

static int generate_function_code(ast_node_t func, map_t map);

static int generate_statement_code(ast_node_t func, map_t global, map_t local);

static void generate_binary_op_code(char op, int op_type);

static int count_slots(ast_node_t base, char prepend, int vars, map_t map);

static int clear_map(void *nothing, void *str);

int generate_intermediate_code(ast_node_t *parse_trees, int num_trees)
{
    map_t global_map = hashmap_new();

    generate_constants(parse_trees, num_trees, global_map);
    generate_globals(parse_trees, num_trees, global_map);
    generate_functions(parse_trees, num_trees, global_map);

    hashmap_iterate(global_map, &clear_map, NULL);
    hashmap_free(global_map);
    return 0;
}

static int generate_constants(ast_node_t *parse_trees, int num_trees, map_t map)
{
    int constants = 0;

    printf("\n.CONSTANTS %d\n", constants);
    return 0;
}

static int generate_globals(ast_node_t *parse_trees, int num_trees, map_t map)
{
    int i, vars=0;
    
    //for each file
    for(i = 0; i < num_trees; i++)
    {
        vars = count_slots(parse_trees[i], 'G', vars, map); 
    }

    if(program_options & INTERMEDIATE_OUTPUT)
    {
        printf("\n.GLOBALS %d\n", vars);
    }

    return 0;
}

static int generate_functions(ast_node_t *parse_trees, int num_trees, map_t map)
{
    int i, j, funcs = 0;
    char *func_num;
    ast_node_t cur;
    
    //for each file
    for(i = 0; i < num_trees; i++)
    {
        //for each root node
        for(j = 0; j < parse_trees[i].num_children; j++)
        {
            cur = parse_trees[i].children[j];
            //count number of functions and assign stack identifier
            if(cur.token == FUNCTION_DEF)
            {
                func_num = malloc(10);
                snprintf(func_num, 10, "%d", funcs + FUNC_OFFSET);
                hashmap_put(map, cur.children[0].value.s, func_num);
                funcs++;
            }
        }
    }
    printf("\n.FUNCTIONS %d\n", funcs);

    funcs = FUNC_OFFSET;
    
    //for each file
    for(i = 0; i < num_trees; i++)
    {
        //for each root node
        for(j = 0; j < parse_trees[i].num_children; j++)
        {
            cur = parse_trees[i].children[j];
            //generate function code
            if(cur.token == FUNCTION_DEF)
            {
                printf("\n.FUNC %d %s\n", funcs, cur.children[0].value.s);
                generate_function_code(cur, map);
                printf(".end FUNC\n");
            }
        }
    }

    return 0;
}

static int generate_function_code(ast_node_t func, map_t map)
{
    int locals = 0, i;
    //ast_node_t cur;
    map_t local_map = hashmap_new();

    //set params
    locals = count_slots(func.children[1], 'L', locals, local_map);
    printf(".params %d\n", locals);
    printf(".return %d\n", func.type != VOID);

    //get local vars
    locals = count_slots(func.children[2], 'L', locals, local_map);
    printf(".locals %d\n", locals);

    for(i = 0; i < func.children[3].num_children; i++)
    {
        generate_statement_code(func.children[3].children[i], map, local_map);
    }

    hashmap_iterate(local_map, &clear_map, NULL);
    hashmap_free(local_map);
    return 0;
}

static int generate_statement_code(ast_node_t func, map_t global, map_t local)
{
    char token[20];
    tok_to_str(token, func.token);

    printf(";%s on line %d\n", token, func.line_number);

    switch(func.token)
    {
        case '=':

            break;
        case RETURN:
            break;
        case BINARY_OP:
            generate_statement_code(func.children[0], global, local);
            generate_statement_code(func.children[1], global, local);
            generate_binary_op_code(func.value.c, func.type);
            break;
        case FUNCTION_CALL:
            break;
        case IF:
        case FOR:
        case WHILE:
        case DO:
        case CONTINUE:
        case BREAK:
        case ELSE:
        case TURNARY:
            fprintf(stderr, "branching statement on line %d not yet supported\n", func.line_number);
            break;
        default:
            fprintf(stderr, "collin you forgot to write a case for %s you idiot\n", token);
            return -1;
    }

    return 0;
}

static void generate_binary_op_code(char op, int op_type)
{
    char code;
    switch(op_type)
    {
        case INT:
            code = 'i';
            break;
        case CHAR:
            code = 'c';
            break;
        case FLOAT:
            code = 'f';
    }
    switch(op)
    {
        case '+':
        case '-':
        case '%':
            printf("%c%c\n", op, code);
            break;
    }
}

static int count_slots(ast_node_t base, char prepend, int vars, map_t map)
{
    int i, j;
    char *var_name;
    ast_node_t cur;

    //for each variable node
    for(i = 0; i < base.num_children; i++)
    {
        cur = base.children[i];
        if(cur.token == VARIABLE)
        {
            //for each identifier add up slots and add to global total
            for(j = 0; j < cur.num_children; j++)
            {
                var_name = malloc(sizeof(char) * 10);
                snprintf(var_name, 10, "%c%d", prepend, vars);
                hashmap_put(map, cur.children[j].value.s, var_name);
                vars++;
                if(cur.children[j].type & ARRAY)
                    vars += cur.children[j].array_size -1;
            }
        }
        else if(cur.token == TYPE_NAME)
        {
            var_name = malloc(sizeof(char) * 10);
            snprintf(var_name, 10, "%c%d", prepend, vars);
            hashmap_put(map, cur.value.s, var_name);
            vars++;
            if(cur.type & ARRAY)
                vars += cur.array_size - 1;
        }
    }

    return vars;
}

static int clear_map(void *nothing, void *str)
{
    free(str);
    return 0;
}
