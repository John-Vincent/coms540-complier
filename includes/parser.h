#ifndef PARSER_H
#define PARSER_H

    typedef union ast_value
    {
        long l;
        int i;
        float f;
        char c;
        char *s;
    } ast_value_t;

    typedef struct ast_node
    {
        int type;
        int num_children;
        struct ast_node *children;
        ast_value_t value;
        int array_size;
    } ast_node_t;

    void yyerror(const char *error);

    int parse_input(int num_files, char **files);

    ast_node_t *new_ast_node(int type, int num_children, ast_node_t *children, ast_value_t value, int array_size);

    ast_node_t *new_variable_node(int scope, int type, ast_node_t *indedifiers);

    void add_ast_children(ast_node_t *parent, ast_node_t *children, int num_children);

#endif
