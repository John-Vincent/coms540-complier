#include "../../includes/types.h"
#include "../../includes/lexer.h"
#include <string.h>

void tok_to_str(char* buff, int token)
{
    switch(token)
    {
        case INCLUDE_TOKEN:
            strcpy(yytoken_name, "INCLUDE");
            break;
        case DEFINE_TOKEN:
            strcpy(yytoken_name, "DEFINE");
            break;
        case TYPE_TOKEN:
            strcpy(yytoken_name, "TYPE");
            break;
        case FOR_TOKEN:
            strcpy(yytoken_name, "FOR");
            break;
        case WHILE_TOKEN:
            strcpy(yytoken_name, "WHILE");
            break;
        case DO_TOKEN:
            strcpy(yytoken_name, "DO");
            break;
        case IF_TOKEN:
            strcpy(yytoken_name, "IF");
            break;
        case ELSE_TOKEN:
            strcpy(yytoken_name, "ELSE");
            break;
        case BREAK_TOKEN:
            strcpy(yytoken_name, "BREAK");
            break;
        case CONTINUE_TOKEN:
            strcpy(yytoken_name, "CONTINUE");
            break;
        case RETURN_TOKEN:
            strcpy(yytoken_name, "RETURN");
            break;
        case IDENT_TOKEN:
            strcpy(yytoken_name, "IDENT");
            break;
        case INTCONST_TOKEN:
            strcpy(yytoken_name, "INTCONST");
            break;
        case STRCONST_TOKEN:
            strcpy(yytoken_name, "STRCONST");
            break;
        case CHARCONST_TOKEN:
            strcpy(yytoken_name, "CHARCONST");
            break;
        case INCLUDE_FILE_TOKEN:
            strcpy(yytoken_name, "INCLUDE_FILE");
            break;
        case LPAR_TOKEN:
            strcpy(yytoken_name, "LPAR");
            break;
        case RPAR_TOKEN:
            strcpy(yytoken_name, "RPAR");
            break;
        case LBRACKET_TOKEN:
            strcpy(yytoken_name, "LBRACKET");
            break;
        case RBRACKET_TOKEN:
            strcpy(yytoken_name, "RBRACKET");
            break;
        case LBRACE_TOKEN:
            strcpy(yytoken_name, "LBRACE");
            break;
        case RBRACE_TOKEN:
            strcpy(yytoken_name, "RBRACE");
            break;
        case COMMA_TOKEN:
            strcpy(yytoken_name, "COMMA");
            break;
        case SEMI_TOKEN:
            strcpy(yytoken_name, "SEMI");
            break;
        case QUEST_TOKEN:
            strcpy(yytoken_name, "QUEST");
            break;
        case COLON_TOKEN:
            strcpy(yytoken_name, "COLON");
            break;
        case EQUAL_TOKEN:
            strcpy(yytoken_name, "EQUAL");
            break;
        case NEQUAL_TOKEN:
            strcpy(yytoken_name, "NEQUAL");
            break;
        case GT_TOKEN:
            strcpy(yytoken_name, "GT");
            break;
        case GE_TOKEN:
            strcpy(yytoken_name, "GE");
            break;
        case LT_TOKEN:
            strcpy(yytoken_name, "LT");
            break;
        case LE_TOKEN:
            strcpy(yytoken_name, "LE");
            break;
        case PLUS_TOKEN:
            strcpy(yytoken_name, "PLUS");
            break;
        case MINUS_TOKEN:
            strcpy(yytoken_name, "MINUS");
            break;
        case STAR_TOKEN:
            strcpy(yytoken_name, "STAR");
            break;
        case SLASH_TOKEN:
            strcpy(yytoken_name, "SLASH");
            break;
        case MOD_TOKEN:
            strcpy(yytoken_name, "MOD");
            break;
        case TILDE_TOKEN:
            strcpy(yytoken_name, "TILDE");
            break;
        case PIPE_TOKEN:
            strcpy(yytoken_name, "PIPE");
            break;
        case BANG_TOKEN:
            strcpy(yytoken_name, "BANG");
            break;
        case AMP_TOKEN:
            strcpy(yytoken_name, "AMP");
            break;
        case DAMP_TOKEN:
            strcpy(yytoken_name, "DAMP");
            break;
        case DPIPE_TOKEN:
            strcpy(yytoken_name, "DPIPE");
            break;
        case ASSIGN_TOKEN:
            strcpy(yytoken_name, "ASSIGN");
            break;
        case PLUSASSIGN_TOKEN:
            strcpy(yytoken_name, "PLUSASSIGN");
            break;
        case MINUSASSIGN_TOKEN:
            strcpy(yytoken_name, "MINUSASSIGN");
            break;
        case STARASSIGN_TOKEN:
            strcpy(yytoken_name, "STARASSIGN");
            break;
        case SLASHASSIGN_TOKEN:
            strcpy(yytoken_name, "SLASHASSIGN");
            break;
        case INCR_TOKEN:
            strcpy(yytoken_name, "INCR");
            break;
        case DECR_TOKEN:
            strcpy(yytoken_name, "DECR");
            break;
        case UNKNOWN_TOKEN:
            strcpy(yytoken_name, "UNKNOWN");
            break;
        case REALCONST_TOKEN:
            strcpy(yytoken_name, "REALCONST");
            break;
    }
}
