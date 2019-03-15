#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
    
    typedef void *symbol_table_t;

    typedef void *symbol_t;

    int global_scope_add(char* symbol, int type, char* file, int line, int* params);

    int local_scope_add(char* symbol, int type, char* file, int line);

    symbol_table_t close_scope();

    symbol_table_t close_global_scope();

    int find_type(char* symbol);

    int find_type_in_table(symbol_table_t t, char* symbol);

    int resolve_bop_type(int op, int type1, int type2);

    int resolve_uop_type(int op, int type);

    int resolve_turnary_type(int t1, int t2, int t3);
    
    void set_return_type(int type);

    void check_return_type(int type);

    void print_expression_type(int type);

#endif
