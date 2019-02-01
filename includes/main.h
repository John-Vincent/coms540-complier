#ifndef MAIN_H
#define MAIN_H

#define INITIAL_OPTION           0x0
#define LEXER_OPTION             0x1
#define PARSER_OPTION            0x2
#define TYPE_OPTION              0x4
#define INTERMEDIATE_OPTION      0x8
#define COMPILE_OPTION           0x10
#define MAIN_OPTIONS             0x1F

//runtime options for the program
extern int program_options;
//character array for passing error messages back to main
//only useful if you know your function is called directly from main or 
//the calling function checks for errors after calling.
extern char error_message[100];
//list of files to compile
extern char** file_list;
//number of files in the list
extern int files

#endif
