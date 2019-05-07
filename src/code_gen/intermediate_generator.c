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

/**
 *  makes 1 pass throught the whole ast counting slots
 *  needed to store all string constants. Then a second
 *  pass through to generate the constant values.
 */
static int generate_constants(ast_node_t *parse_trees, int num_trees);

/**
 *  scans the whole ast and prints the constant information 
 *  for all string constants
 */
void print_constant_traversal(ast_node_t node, int a, void *v);

/**
 *  scans whole ast and counts the number of slots
 *  for string constants we will need
 */
void count_constant_traversal(ast_node_t node, int a, void *v);

/**
 *  counts the number of global slots needed and then writes the 
 *  instruction to reserve those slots.
 */
static int generate_globals(ast_node_t *parse_trees, int num_trees, map_t map);

/**
 *  counts the number of functions, writes the instruction to declair number
 *  of functions. then generates the function code for each function definition
 */
static int generate_functions(ast_node_t *parse_tree, int num_trees, map_t map);

/**
 *  is used by generate_functions to actually generate the code for each function
 */
static int generate_function_code(ast_node_t func, map_t map);

/**
 *  is used by generate_function_code to generate code for specific statements 
 *  in the function body
 */
static int generate_statement_code(ast_node_t func, map_t global, map_t local, char *into, char *around, int test);

/**
 *  this function is used to generate instructions for code that requires
 *  branching. Mainly used split code into logical chunks and splitting 
 *  functionality for different run options.
 */
static int generate_branching_code(ast_node_t func, map_t global, map_t local, char *into, char *around, int test);

/**
 *  generates the code for binary operations
 */
static void generate_binary_op_code(int op, int op_type, char *into, char *around, int test);

/**
 *  counts the number of slots needed by parameters, arguments or 
 *  local variables based on the root node supplied as base.
 *  while doing this it also adds the variables to the hashmap supplied
 *  as the last argument with the address code constructed from
 *  prepending the character argument with the integer argument.
 *  the integer argument is the starting point for the slot counter.
 *  returns the number of slots counted
 */
static int count_slots(ast_node_t base, char prepend, int vars, map_t map);

/**
 *  this function takes a character buffer and stores the comparision
 *  operation that is the opposite of the supplied operation
 */
static void invert_operation(char *type, int op);

/**
 *  This function is called by generate_branching_code
 *  and returns true when the expression in a loop control 
 *  segment does not generate code with jumps. That way the
 *  branching code can then compare the value placed on the stack
 *  by the expression to 0 to get its jump.
 */
static int need_comparison(ast_node_t token);

/**
 *  continually makes new labels to be used by other functions
 */
static void generate_label(char *label);

/**
 *  searches the given maps (can be null) for the key passed
 *  and returns the value associated with that key
 */
static char *get_address(map_t map, map_t map2, char *arg);

/**
 *  many instructions require a character to identify the type
 *  its operating on, this converst by type variables to the 
 *  appropriate character
 */
static char get_type_char(int type);

//function used to free all map memory
static int clear_map(void *nothing, void *str);

//variable used to store constant map because it needs to be global 
static map_t const_map;

//variable to store constant slots because recursive function
static int constant_counter = 0;

/**
 *  generates the code based on the asts passed in
 */
int generate_intermediate_code(ast_node_t *parse_trees, int num_trees)
{
    map_t global_map = hashmap_new();
    const_map = hashmap_new();

    generate_constants(parse_trees, num_trees);
    generate_globals(parse_trees, num_trees, global_map);
    generate_functions(parse_trees, num_trees, global_map);

    hashmap_iterate(global_map, &clear_map, NULL);
    hashmap_free(global_map);
    hashmap_iterate(const_map, &clear_map, NULL);
    hashmap_free(const_map);
    return 0;
}

static int generate_constants(ast_node_t *parse_trees, int num_trees)
{
    int i;
    
    for(i = 0; i < num_trees; i++)
    {
        preorder_traversal(parse_trees[i], 0, &count_constant_traversal, NULL);
    }

    printf("\n.CONSTANTS %d", constant_counter);

    for(i = 0; i < num_trees; i++)
    {
        preorder_traversal(parse_trees[i], 0, &print_constant_traversal, NULL);
    }

    printf("\n");

    return 0;
}

void print_constant_traversal(ast_node_t node, int a, void *v)
{
    int i, length, adjusted;

    if(node.token == STRCONST)
    {
        length = strlen(node.value.s);
        adjusted = length/4;
        if(length%4)
            adjusted++;
        
        for(i = adjusted*4-1; i >= 0; i--)
        {  
            if(i%4 == 3)
                printf("\n  0x");
            if(i >= length)
                printf("00");
            else
                printf("%x", node.value.s[i]);
        }
        constant_counter+=adjusted;
    }
}

void count_constant_traversal(ast_node_t node, int a, void *v)
{
    int adjusted, length;
    char *address;

    if(node.token == STRCONST)
    {   
        address = malloc(10);
        snprintf(address, 10, "C%d", constant_counter);
        hashmap_put(const_map, node.value.s, address);
        length = strlen(node.value.s);
        adjusted = length/4;
        if(length%4)
            adjusted++;
        constant_counter+=adjusted;
    }
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
    
    func_num = malloc(2);
    func_num[0] = '0';
    func_num[1] = '\0';
    hashmap_put(map, "getchar", func_num);
    func_num = malloc(2);
    func_num[0] = '1';
    func_num[1] = '\0';
    hashmap_put(map, "putchar", func_num);
    
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
                funcs++;
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
    printf("  .params %d\n", locals);
    printf("  .return %d\n", func.type != VOID);

    //get local vars
    locals = count_slots(func.children[2], 'L', locals, local_map);
    printf("  .locals %d\n", locals);

    for(i = 0; i < func.children[3].num_children; i++)
    {
        if(generate_statement_code(func.children[3].children[i], map, local_map, NULL, NULL, 0))
            printf("    popx\n");
    }

    hashmap_iterate(local_map, &clear_map, NULL);
    hashmap_free(local_map);
    return 0;
}

static int generate_statement_code(ast_node_t cur, map_t global, map_t local, char *into, char *around, int test)
{
    char token[20], *address=NULL, t;
    tok_to_str(token, cur.token);
    int i;

    printf("    ;%s on line %d\n", token, cur.line_number);

    switch(cur.token)
    {
        case '=':
            address = get_address(local, global, cur.children[0].value.s);
            if(cur.children[0].num_children)
            {
                t = get_type_char(cur.children[0].type);
                generate_statement_code(cur.children[0].children[0], global, local, into, around, 0);
                printf("    ptrto %s\n", address);
                generate_statement_code(cur.children[1], global, local, into, around, test);
                printf("    copy\n    pop%c[]\n", t);
            }
            else
            {
                generate_statement_code(cur.children[1], global, local, into, around, 0);
                printf("    copy\n    pop %s\n", address);
            }
            break;
        case RETURN:
            if(cur.num_children)
            {
                generate_statement_code(cur.children[0], global, local, into, around, 0);
            }
                printf("    ret\n");
            break;
        case BINARY_OP:
            if(cur.value.i == DAMP || cur.value.i == DPIPE)
            {
                generate_branching_code(cur, global, local, into, around, test);
            }
            else
            {
                generate_statement_code(cur.children[0], global, local, into, around, test);
                generate_statement_code(cur.children[1], global, local, into, around, test);
                generate_binary_op_code(cur.value.i, cur.type, into, around, test);
            }
            break;
        case FUNCTION_CALL:
            address = get_address(global, NULL, cur.value.s);
            for(i=0; cur.num_children && i<cur.children[0].num_children; i++)
            {
                generate_statement_code(cur.children[0].children[i], global,local, into, around, 0);
            }
            printf("    call %s\n", address);
            break;
        case CAST:
            generate_statement_code(cur.children[0], global, local, into, around, test);
            switch(cur.type)
            {
                case INT:
                    if(cur.children[0].type == FLOAT)
                        printf("    convif\n");
                    break;
                case CHAR:
                    if(cur.children[0].type == FLOAT)
                        printf("    convif\n");
                    printf("    pushv 0xFF\n    &\n");
                    break;
                case FLOAT:
                    if(cur.children[0].type != FLOAT)
                        printf("    convfi\n");
            }
            break;
        case '-':
            t = get_type_char(cur.children[0].type);
            generate_statement_code(cur.children[0], global, local, into, around, 0);
            printf("    neg%c\n", t);
            break;
        case INCR:
            t = get_type_char(cur.type);
            //array
            if(cur.children[0].token == LVALUE && cur.children[0].num_children)
            {
                address = get_address(local, global, cur.children[0].value.s);
                generate_statement_code(cur.children[0].children[0], global, local, into, around, 0);
                printf("    ptrto %s\n", address);
                generate_statement_code(cur.children[0], global, local, into, around, 0);
                printf("    ++%c\n    copy\n    pop%c[]\n", t, t);
            }
            else if( cur.children[0].token == LVALUE)
            {
                address = get_address(local, global, cur.children[0].value.s);
                generate_statement_code(cur.children[0], global, local, into, around, 0);
                printf("    ++%c\n    copy\n    pop %s\n", t, address);
            }
            else
            {
                generate_statement_code(cur.children[0], global, local, into, around, 0);
                printf("    ++%c\n", t);
            }
            break;
        case DECR:
            t = get_type_char(cur.type);
            //array
            if(cur.children[0].token == LVALUE && cur.children[0].num_children)
            {
                address = get_address(local, global, cur.children[0].value.s);
                generate_statement_code(cur.children[0].children[0], global, local, into, around, 0);
                printf("    ptrto %s\n", address);
                generate_statement_code(cur.children[0], global, local, into, around, 0);
                printf("    --%c\n    copy\n    pop%c[]\n", t, t);
            }
            else if (cur.children[0].token == LVALUE) 
            {
                address = get_address(local, global, cur.children[0].value.s);
                generate_statement_code(cur.children[0], global, local, into, around, 0);
                printf("    --%c\n    copy\n    pop %s\n", t, address);
            }
            else
            {
                generate_statement_code(cur.children[0], global, local, into, around, 0);
                printf("    --%c\n", t);
            }
            break;
        case IF:
        case FOR:
        case WHILE:
        case DO:
        case CONTINUE:
        case BREAK:
        case ELSE:
        case TURNARY:
            //if this is the final option then run the new code else error
            if(program_options & COMPILE_OPTION)
            {
                generate_branching_code(cur, global, local, into, around, test);
            }
            else
            {
                fprintf(stderr, "branching statement on line %d not yet supported\n", cur.line_number);
            }
            break;
        case LVALUE:
            address = get_address(local, global, cur.value.s);
            printf("    push %s\n", address);
            break;
        case INTCONST:
            printf("    pushv 0x%x\n", cur.value.i);
            break;
        case CHARCONST:
            printf("    pushv 0x%x\n", cur.value.c);
            break;
        case REALCONST:
            printf("    pushv 0x%x\n", *(unsigned int*)&cur.value.f);
            break;
        case STRCONST:
            address = get_address(const_map, NULL, cur.value.s);
            printf("    push %s\n", address);
            break;
        default:
            fprintf(stderr, "collin you forgot to write a case for %s you idiot\n", token);
            return -1;
    }

    return 0;
}

static int generate_branching_code(ast_node_t cur, map_t global, map_t local, char *into, char *around, int test)
{
    char label[20], label1[20], label2[20], token[20];
    ast_node_t cur2;
    tok_to_str(token, cur.token);
    int i;
    
    switch(cur.token)
    {
        case IF:
            generate_label(label);
            generate_label(label1);
            cur2 = cur.children[0];
            generate_statement_code(cur2.children[0], global, local, label, label1, 1);
            if(need_comparison(cur2.children[0]))
                generate_binary_op_code(ZEQUAL, cur2.children[0].type, label1, label, 1);
            printf("  %s:\n", label);
            if(cur2.children[1].token == STATEMENT_BLOCK)
            {
                for(i = 0; i < cur2.children[1].num_children; i++)
                {
                    generate_statement_code(cur2.children[1].children[i], global, local, into, around, 0);
                }
            }
            else
            {
                generate_statement_code(cur2.children[1], global, local, into, around, 0);
            }

            if(cur.num_children == 2)
            {
                generate_label(label);
                printf("    goto %s\n  %s:\n", label, label1);
                cur2 = cur.children[1];
                if(cur2.children[0].token == STATEMENT_BLOCK)
                {
                    for(i = 0; i < cur2.children[0].num_children; i++)
                    {
                        generate_statement_code(cur2.children[0].children[i], global, local, into, around, 0);
                    }
                }
                else
                {
                    generate_statement_code(cur2.children[0], global, local, into, around, 0);
                }
                printf("  %s:\n", label);
            }
            else
            {
                printf("  %s:\n", label);
            }
            break;
        case FOR:
            generate_label(label);
            generate_label(label1);
            generate_statement_code(cur.children[0], global, local, NULL, NULL, 0);
            printf("  %s:\n", label);
            generate_statement_code(cur.children[1], global, local, NULL, label1, 1);
            if(need_comparison(cur.children[1]))
                generate_binary_op_code(ZEQUAL, cur.children[1].type, label1, NULL, 1);
            if(cur.children[3].token == STATEMENT_BLOCK)
            {
                for(i = 0; i < cur.children[3].num_children; i++)
                {
                    generate_statement_code(cur.children[3].children[i], global, local, label, label1, 0);
                }
            }
            else
            {
                generate_statement_code(cur.children[3], global, local, label, label1, 0);
            }
            generate_statement_code(cur.children[2], global, local, label, label1, 0);
            printf("    goto %s\n  %s:", label, label1);
            break;
        case WHILE:
            generate_label(label);
            generate_label(label1);
            printf("  %s:\n", label);
            generate_statement_code(cur.children[0], global, local, NULL, label1, 1);
            if(need_comparison(cur.children[0]))
                generate_binary_op_code(ZEQUAL, cur.children[0].type, label1, NULL, 1);
            if(cur.children[1].token == STATEMENT_BLOCK)
            {
                for(i = 0; i < cur.children[1].num_children; i++)
                {
                    generate_statement_code(cur.children[1].children[i], global, local, label,  label1, 0);
                }
            }
            else
            {
                generate_statement_code(cur.children[1], global, local, label, label1, 0);
            }
            printf("    goto %s\n  %s:\n", label, label1);
            break;
        case DO:
            generate_label(label);
            generate_label(label1);
            generate_label(label2);
            printf("  %s:\n", label);
            if(cur.children[1].token == STATEMENT_BLOCK)
            {
                for(i = 0; i < cur.children[1].num_children; i++)
                {
                    generate_statement_code(cur.children[1].children[i], global, local, label2, label1, 0);
                }
            }
            else
            {
                generate_statement_code(cur.children[1], global, local, label2, label1, 0);
            }
            printf("  %s:\n", label2);
            generate_statement_code(cur.children[0], global, local, label, label1, 1);
            if(need_comparison(cur.children[0]))
                generate_binary_op_code(ZNEQUAL, cur.children[0].type, label, NULL, 1);
            printf("  %s:\n", label1);
            break;
        case CONTINUE:
            if(into)
                printf("    goto %s\n", into);
            break;
        case BREAK:
            if(around)
                printf("    goto %s\n", around);
            break;
        case ELSE:
            fprintf(stderr, "somehow i needed to process an ELSE by itself");
            break;
        case TURNARY:
            generate_label(label);
            generate_label(label1);
            generate_statement_code(cur.children[0], global, local, NULL, label, 1);
            if(need_comparison(cur.children[0]))
                generate_binary_op_code(ZEQUAL, cur.children[0].type, label, NULL, 1);
            generate_statement_code(cur.children[1], global, local, into, around, 0);
            printf("    goto %s\n  %s:\n", label1, label);
            generate_statement_code(cur.children[2], global, local, into, around, 0);
            printf("  %s:\n", label1);
            break;
        case BINARY_OP:
            if(cur.value.i == DAMP)
            {
                if(test && around)
                {
                    generate_statement_code(cur.children[0], global, local, NULL, around, 1);
                    if(need_comparison(cur.children[0]))
                        generate_binary_op_code(ZEQUAL, cur.children[0].type, around, NULL, 1);
                    generate_statement_code(cur.children[1], global, local, into, around, 1);
                    if(need_comparison(cur.children[1]))
                        generate_binary_op_code(ZEQUAL, cur.children[1].type, around, into, 1);
                }
                else if(test && into)
                {
                    generate_label(label);
                    generate_statement_code(cur.children[0], global, local, NULL, label, 1);
                    if(need_comparison(cur.children[0]))
                        generate_binary_op_code(ZEQUAL, cur.children[0].type, label, NULL, 1);
                    generate_statement_code(cur.children[1], global, local, into, NULL, 1);
                    if(need_comparison(cur.children[1]))
                        generate_binary_op_code(ZEQUAL, cur.children[1].type, around, into, 1);
                    printf("  %s:\n", label);
                }
                else
                {
                    generate_label(label);
                    generate_label(label1);
                    generate_statement_code(cur.children[0], global, local, NULL, label1, 1);
                    if(need_comparison(cur.children[0]))
                        generate_binary_op_code(ZEQUAL, cur.children[0].type,label1, NULL, 1);
                    generate_statement_code(cur.children[1], global, local, NULL, label1, 1);
                    if(need_comparison(cur.children[1]))
                        generate_binary_op_code(ZEQUAL, cur.children[1].type, label1, NULL, 1);
                    printf("    pushv 0x1\n    goto %s\n  %s:\n    pushv 0x0\n  %s:\n", label, label1, label);
                }
                break;
            }
            else
            {
                if(test && into)
                {
                    generate_statement_code(cur.children[0], global, local, into, NULL, 1);
                    if(need_comparison(cur.children[0]))
                        generate_binary_op_code(ZNEQUAL, cur.children[0].type, into, NULL, 1);
                    generate_statement_code(cur.children[1], global, local, into, around, 1);
                    if(need_comparison(cur.children[1]))
                        generate_binary_op_code(ZNEQUAL, cur.children[1].type, into, around, 1);
                }
                else if(test && around)
                {
                    generate_label(label);
                    generate_statement_code(cur.children[0], global, local, label, NULL, 1);
                    if(need_comparison(cur.children[0]))
                        generate_binary_op_code(ZNEQUAL, cur.children[0].type, label, NULL, 1);
                    generate_statement_code(cur.children[1], global, local, NULL, around, 1);
                    if(need_comparison(cur.children[1]))
                        generate_binary_op_code(ZEQUAL, cur.children[1].type, around, NULL, 1);
                    printf("  %s:\n", label);
                }
                else
                {
                    generate_label(label);
                    generate_label(label1);
                    generate_statement_code(cur.children[0], global, local, label, NULL, test);
                    if(need_comparison(cur.children[0]))
                        generate_binary_op_code(ZNEQUAL, cur.children[0].type, label, NULL, 1);
                    generate_statement_code(cur.children[1], global, local, label, NULL, test);
                    if(need_comparison(cur.children[1]))
                        generate_binary_op_code(ZNEQUAL, cur.children[1].type, label, NULL, 1);
                    printf("    pushv 0x0\n    goto %s\n  %s:\n    pushv 0x1\n  %s:\n", label1, label, label1);
                }
                break;
            }
        default:
            fprintf(stderr, "token %s doesn't belong in branching block\n", token);
    }
    return 0;
}

static void generate_binary_op_code(int op, int op_type, char *into, char *around, int test)
{
    char code, label1[20], label2[20], type[20];

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
        case '/':
        case '*':
            printf("    %c%c\n", op, code);
            break;
        case '&':
        case '|':
            printf("    %c\n", op);
            break;
        case ZEQUAL:
        case ZNEQUAL:
        case EQUAL:
        case NEQUAL:
        case GE:
        case LE:
        case '>':
        case '<':
            tok_to_str(type, op);
            if(test && into && around)
            {
                printf("    %s%c %s\n", type, code, into);
                printf("    goto %s\n", around);
            }
            else if(test && into)
            {

                printf("    %s%c %s\n", type, code, into);
            }
            else if(test && around)
            {
                invert_operation(type, op);
                printf("    %s%c %s\n", type, code, around);
            }
            else
            {
                generate_label(label1);
                generate_label(label2);
                printf("    %s%c %s\n    pushv 0x0\n    goto %s\n", type, code, label1, label2);
                printf("  %s:\n    pushv 0x1\n  %s:\n", label1, label2);
            }
            break;
    }
}

static char *get_address(map_t map, map_t map2, char *arg)
{
    char *address=NULL;
    if(map && hashmap_get(map, arg, (void**)&address) == MAP_OK)
    {
        return address;
    }
    else if(map2 && hashmap_get(map2, arg, (void**)&address) == MAP_OK)
    {
        return address;
    }
    return NULL;
}

static char get_type_char(int type)
{
    char t = ' ';
    //mask off array
    switch(((type | ARRAY) ^ ARRAY) & TYPE_MASK)
    {
        case INT:
            t = 'i';
            break;
        case FLOAT: 
            t = 'f';
            break;
        case CHAR:
            t = 'c';
    }
    return t;
}

static void generate_label(char *ans)
{
    static int label_number = 0;

    snprintf(ans, 20, "I%d", label_number);
    label_number++;
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

static int need_comparison(ast_node_t token)
{

    if(token.token == BINARY_OP)
    {
        switch(token.value.i)
        {
            case EQUAL:
            case NEQUAL:
            case GE:
            case LE:
            case '>':
            case '<':
            case DAMP:
            case DPIPE:
            case ZEQUAL:
            case ZNEQUAL:
                return 0;
            default:
                return 1;
        }
    }
    return 1;
}

static void invert_operation(char *type, int op)
{
    switch(op)
    {
        case EQUAL:
            tok_to_str(type, NEQUAL);
            break;
        case NEQUAL:
            tok_to_str(type, EQUAL);
            break;
        case GE:
            tok_to_str(type, '<');
            break;
        case LE:
            tok_to_str(type, '>');
            break;
        case '>':
            tok_to_str(type, LE);
            break;
        case '<':
            tok_to_str(type, GE);
            break;
        case ZEQUAL:
            tok_to_str(type, ZNEQUAL);
            break;
        case ZNEQUAL:
            tok_to_str(type, ZEQUAL);
            break;
    }
}

static int clear_map(void *nothing, void *str)
{
    free(str);
    return 0;
}
