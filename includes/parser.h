#ifndef PARSER_H
#define PARSER_H

    #include "./symbol_table.h"
    
    /**
     * this union contains
     * all types that my ast_node notes will
     * need to store data from the parsing
     */
    typedef union ast_value
    {
        long l;
        int i;
        float f;
        char c;
        char *s;
        symbol_table_t t;
    } ast_value_t;

    /**
     * this is the basis for my parse tree
     * each node has an array that contains 
     * all of its children
     * the type is the token type
     * the array size might be used for different things 
     * depending on the node but its mainly used for storing
     * the array size of declared variables as well as the index
     * of access for expressions
     */
    typedef struct ast_node
    {
        int token;
        int type;
        int num_children;
        struct ast_node *children;
        ast_value_t value;
        int array_size;
    } ast_node_t;

    /**
     * function that is called by bison to print parsing errors
     */
    void yyerror(const char *error);

    /**
     *  this function is called from main. it gets the file list from the 
     *  and then runs the parser on each file generating an ast for each file
     *  it then returns the ast array
     */
    ast_node_t *parse_input(int num_files, char **files);

    /**
     *  this function takes the different values of a ast_node, allocates a new node,
     *  and assigns the values to the new node. 
     *  if you call with a non 0 number of children and a null pointer for children
     *  then an empty children array will be allocated
     */
    ast_node_t *new_ast_node(int token, int type, int num_children, ast_node_t *children, ast_value_t value, int array_size);
    
    /**
     *  at the start I couldn't think of a better way of creating my variable nodes so i made this
     */
    ast_node_t *new_variable_node(int scope, int type, ast_node_t *indedifiers);

    ast_node_t *make_function_sig(ast_node_t *type_name, ast_node_t *params, int type);

    /**
     *  This function will take an existing ast node and array of ast nodes, and the 
     *  length of the array, and concatinate the array onto the existing nodes children.
     */
    void add_ast_children(ast_node_t *parent, ast_node_t *children, int num_children);

    /**
     *   This function does a preorder traversal of the ast so i can print it. it takes a starting node,
     *   an arbitrary depth starting point, a function pointer and a optional argument to supply to the 
     *   function. It then traverses the tree incrementing depth for each recursive call.
     */
    void preorder_traversal(ast_node_t node, int depth, void (*func)(ast_node_t, int, void*), void *arg);

    /**
     *
     */
    void postorder_traversal(ast_node_t node, int depth, void (*func)(ast_node_t, int, void*), void *arg);
    
    /**
     *   This is the function that i use as an argument to the traversal
     *   it just prints the basic information about each node adding
     *   2 * depth spaces before printing so its easier to visualize
     */
    void print_node(ast_node_t node, int depth, void *arg);

    /**
     *   This function is used to free the children of an ast_node_t 
     *   created by the parser. You can free whole tree by passing
     *   in the root node then calling free on the root node itself, 
     *   or some subtree by passing the parent of the children you
     *   want to free
     */
    void free_tree_memory(ast_node_t node);

#endif
