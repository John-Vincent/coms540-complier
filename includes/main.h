#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

#define INITIAL_OPTION           0x0
#define LEXER_OPTION             0x1
#define PARSER_OPTION            0x2
#define TYPE_OPTION              0x4
#define INTERMEDIATE_OPTION      0x8
#define COMPILE_OPTION           0x10
#define MAIN_OPTIONS             0x1F
#define OPTION_ERROR             0xFF
#define LEXER_DEBUG_OPTION       0x20
#define LEXER_SAVE_OPTION        0x40
//runtime options for the program
extern uint64_t program_options;

#endif
