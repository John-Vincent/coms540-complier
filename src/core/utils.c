#include <stdio.h>
#include <string.h>
#include "../../includes/types.h"
#include "../../bin/parser/bison.h"

void type_to_str(char *buff, int type)
{
    switch(type & SCOPE_MASK)
    {
        case STATIC:
            strcpy(buff, "STATIC ");
            break;
        case EXTERN:
            strcpy(buff, "EXTERN ");
            break;
        default:
            strcpy(buff, "");
            break;
    }
    
    switch(type & (TYPE_MASK | FUNC_MASK))
    {
        case INT:
            strcat(buff, "INT");
            break;
        case VOID:
            strcat(buff, "VOID");
            break;
        case FLOAT:
            strcat(buff, "FLOAT");
            break;
        case CHAR:
            strcat(buff, "CHAR");
            break;
        case INT | ARRAY:
            strcat(buff, "INT[]");
            break;
        case VOID | ARRAY:
            strcat(buff, "VOID[]");
            break;
        case FLOAT | ARRAY:
            strcat(buff, "FLOAT[]");
            break;
        case CHAR | ARRAY:
            strcat(buff, "CHAR[]");
            break;
        case PROTO_TYPE:
            strcat(buff, "PROTO_TYPE");
            break;
        case DEF_TYPE:
            strcat(buff, "DEF_TYPE");
            break;
        case ERROR:
            strcat(buff, "TYPE ERROR");
            break;
        default:
            strcat(buff, "UNKNOWN");
            break;
    }
}

void tok_to_str(char* buff, int token)
{
    switch(token)
    {
        case INCLUDE:
            strcpy(buff, "INCLUDE");
            break;
        case DEFINE:
            strcpy(buff, "DEFINE");
            break;
        case UNDEF:
            strcpy(buff, "UNDEF");
            break;
        case TYPE:
            strcpy(buff, "TYPE");
            break;
        case FOR:
            strcpy(buff, "FOR");
            break;
        case WHILE:
            strcpy(buff, "WHILE");
            break;
        case DO:
            strcpy(buff, "DO");
            break;
        case IF:
            strcpy(buff, "IF");
            break;
        case ELSE:
            strcpy(buff, "ELSE");
            break;
        case BREAK:
            strcpy(buff, "BREAK");
            break;
        case CONTINUE:
            strcpy(buff, "CONTINUE");
            break;
        case RETURN:
            strcpy(buff, "RETURN");
            break;
        case IDENT:
            strcpy(buff, "IDENT");
            break;
        case INTCONST:
            strcpy(buff, "INTCONST");
            break;
        case HEXCONST:
            strcpy(buff, "HEXCONST");
            break;
        case STRCONST:
            strcpy(buff, "STRCONST");
            break;
        case CHARCONST:
            strcpy(buff, "CHARCONST");
            break;
        case INCLUDE_FILE:
            strcpy(buff, "INCLUDE_FILE");
            break;
        case LPAR:
            strcpy(buff, "(");
            break;
        case RPAR:
            strcpy(buff, ")");
            break;
        case LBRACKET:
            strcpy(buff, "[");
            break;
        case RBRACKET:
            strcpy(buff, "]");
            break;
        case LBRACE:
            strcpy(buff, "{");
            break;
        case RBRACE:
            strcpy(buff, "}");
            break;
        case COMMA:
            strcpy(buff, ",");
            break;
        case SEMI:
            strcpy(buff, ";");
            break;
        case QUEST:
            strcpy(buff, "?");
            break;
        case COLON:
            strcpy(buff, ":");
            break;
        case EQUAL:
            strcpy(buff, "==");
            break;
        case NEQUAL:
            strcpy(buff, "!=");
            break;
        case GT:
            strcpy(buff, ">");
            break;
        case GE:
            strcpy(buff, ">=");
            break;
        case LT:
            strcpy(buff, "<");
            break;
        case LE:
            strcpy(buff, "<=");
            break;
        case PLUS:
            strcpy(buff, "+");
            break;
        case MINUS:
            strcpy(buff, "-");
            break;
        case STAR:
            strcpy(buff, "*");
            break;
        case SLASH:
            strcpy(buff, "/");
            break;
        case MOD:
            strcpy(buff, "%");
            break;
        case TILDE:
            strcpy(buff, "~");
            break;
        case PIPE:
            strcpy(buff, "|");
            break;
        case BANG:
            strcpy(buff, "!");
            break;
        case AMP:
            strcpy(buff, "&");
            break;
        case DAMP:
            strcpy(buff, "&&");
            break;
        case DPIPE:
            strcpy(buff, "||");
            break;
        case ASSIGN:
            strcpy(buff, "=");
            break;
        case PLUSASSIGN:
            strcpy(buff, "+=");
            break;
        case MINUSASSIGN:
            strcpy(buff, "-=");
            break;
        case STARASSIGN:
            strcpy(buff, "*=");
            break;
        case SLASHASSIGN:
            strcpy(buff, "/=");
            break;
        case INCR:
            strcpy(buff, "++");
            break;
        case DECR:
            strcpy(buff, "--");
            break;
        case UNKNOWN:
            strcpy(buff, "UNKNOWN");
            break;
        case REALCONST:
            strcpy(buff, "REALCONST");
            break;
        case IFNDEF:
            strcpy(buff, "IFNDEF");
            break;
        case IFDEF: 
            strcpy(buff, "IFDEF");
            break;
        case ENDIF:
            strcpy(buff, "ENDIF");
            break;
        case ELIF:
            strcpy(buff, "ELIF");
            break;
        case ELSE_DIREC:
            strcpy(buff, "ELSE_DIRECTIVE");
            break;
        case SCOPE:
            strcpy(buff, "SCOPE");
            break;
        case PROGRAM:
            strcpy(buff, "PROGRAM");
            break;
        case STATEMENT: 
            strcpy(buff, "STATEMENT");
            break;
        case VARIABLE:
            strcpy(buff, "VARIABLE");
            break;
        case BINARY_OP:
            strcpy(buff, "BINARY_OP");
            break;
        case CAST: 
            strcpy(buff, "CAST");
            break;
        case VARIABLE_LIST:
            strcpy(buff, "VARIABLE_LIST");
            break;
        case EMPTY: 
            strcpy(buff, "EMPTY");
            break;
        case PARAM_LIST:
            strcpy(buff, "PARAM_LIST");
            break;
        case STATEMENT_BLOCK:
            strcpy(buff, "STATEMENT_LIST");
            break;
        case TURNARY:
            strcpy(buff, "TURNARY");
            break;
        case EXPRESSION_LIST: 
            strcpy(buff, "EXRPESSION_LIST:");
            break;
        case LVALUE:
            strcpy(buff, "LVALUE");
            break;
        case FUNCTION_DEF:
            strcpy(buff, "FUNCTION_DEF");
            break;
        case TYPE_NAME:
            strcpy(buff, "TYPE_NAME");
            break;
        case FUNCTION_PROTO:
            strcpy(buff, "FUNCTION_PROTO");
            break;
        case FUNCTION_CALL:
            strcpy(buff, "FUNCTION_CALL");
            break;
        case ZEQUAL:
            strcpy(buff, "==0");
            break;
        case ZNEQUAL:
            strcpy(buff, "!=0");
            break;
        default:
            sprintf(buff, "%d", token); 
    }
}
