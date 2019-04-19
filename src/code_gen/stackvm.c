
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// #define DEBUG_PARSER
// #define DEBUG_LABELS
// #define SHOW_PROGRAM
// #define SHOW_EXECUTION
// #define SHOW_STACK

void runtimeError(const char* e);

/*
  Conversion
*/
int u2i(unsigned x)
{
  return *((int*)(&x));
}

float u2f(unsigned x)
{
  return *((float*)(&x));
}

unsigned i2u(int x)
{
  return *((unsigned*)(&x));
}

unsigned f2u(float x)
{
  return *((unsigned*)(&x));
}

/*
  Memory segments
*/

typedef struct {
  unsigned* mem_base;
  unsigned* data;
  size_t size;
} segment;

void initSegment(segment* S, size_t slots)
{
  S->size = slots;
  S->mem_base = (unsigned*) malloc (slots * sizeof(unsigned));
  if ( (slots>0) && (0==S->mem_base) ) {
    fprintf(stderr, "Error - couldn't allocate memory segment\n");
    exit(2);
  }
  S->data = S->mem_base;
}

void makeSubSegment(const segment* mainSeg, size_t off, size_t cap, segment* piece)
{
  assert(off <= cap);
  assert(cap <= mainSeg->size);

  piece->mem_base = mainSeg->mem_base;
  piece->data = mainSeg->data + off;
  piece->size = cap - off;
}

unsigned addr2ptr(const segment* S, unsigned addr)
{
  assert(S);
  assert(S->data >= S->mem_base);
  return (S->data - S->mem_base) + addr;
}

void showSegment(segment* S)
{
  unsigned i;
  for (i=0; i<S->size; i++) {
    printf("%4u: %08x\n", i, S->data[i]);
  }
}

/*
  Stack functions
*/

typedef struct {
  size_t size;
  unsigned* data;
  size_t top;
} stack;

void initStack(const segment* seg, size_t off, stack* S)
{
  S->size = seg->size - off;
  S->data = seg->data + off;
  S->top = 0;
}

void makeSubStack(stack* ss, const stack* S)
{
  ss->size = S->size - S->top;
  ss->data = S->data + S->top;
  ss->top = 0;
}

unsigned top(stack* S)
{
  if (S->top) {
    return S->data[S->top-1];
  } else {
    runtimeError("Stack underflow");
    return 0;
  }
}

unsigned* topptr(stack* S)
{
  if (S->top) {
    return S->data + S->top - 1;
  } else {
    runtimeError("Stack overflow");
    return 0;
  }
}

unsigned pop(stack* S)
{
  if (S->top) {
    return S->data[--S->top];
  } else {
    runtimeError("Stack underflow");
    return 0;
  }
}

void push(stack* S, unsigned d)
{
  if (S->top >= S->size) {
    runtimeError("Stack overflow");
  } else {
    S->data[S->top++] = d;
  }
}

void move(stack* S, unsigned slots)
{
  if (0==slots) return;
  if (S->top <= slots) {
    runtimeError("Stack underflow");
  }
  unsigned* ptr = S->data + S->top - 1;
  unsigned top = *ptr;
  while (slots--) {
    *ptr = *(ptr-1);
    ptr--;
  }
  *ptr = top;
}

void showStack(FILE* out, const char* name, stack* S)
{
  unsigned i;
  fprintf(out, "%s", name);
  for (i=0; i<S->top; i++) {
    if (i) fprintf(out, ", ");
    fprintf(out, "%x", S->data[i]);
  }
  fprintf(out, "\n");
}

/*
  Code
*/

/*
  Instruction types
*/
typedef enum {
  PUSH, PTRTO, PUSHv, PUSHc, PUSHi, PUSHf, 
  COPY, MOVE,
  POPX, POP, POPc, POPi, POPf,

  CALL, RET,

  INCc, INCi, INCf,
  DECc, DECi, DECf,
  NEGc, NEGi, NEGf,
  FLIP, CONVif, CONVfi,

  PLUSc, PLUSi, PLUSf,
  MINUSc, MINUSi, MINUSf,
  STARc, STARi, STARf,
  SLASHc, SLASHi, SLASHf,
  MODc, MODi,
  AND, OR,

  GOTO, 
  IFZc, IFZi, IFZf,
  IFNZc, IFNZi, IFNZf,
  IFEQc, IFEQi, IFEQf,
  IFNEc, IFNEi, IFNEf,
  IFLTc, IFLTi, IFLTf,
  IFLEc, IFLEi, IFLEf,
  IFGTc, IFGTi, IFGTf,
  IFGEc, IFGEi, IFGEf,
  NONE,
  ERROR
} opcode;

/*
  Address types
*/
typedef enum {
  CONST='C', GLOBAL='G', LOCAL='L', LABEL=':', FNUM='f', VALUE='v', UNUSED=' ', MOVEDIST='m'
} address_type;

/*
  Complete instructions
*/
typedef struct {
  opcode op;
  address_type atype;
  unsigned addr;
} instruction;

void showInstruction(FILE* out, instruction I)
{
  switch (I.op) {
    case PUSH:    fprintf(out, "push %c%u", I.atype, I.addr); return;
    case PTRTO:   fprintf(out, "ptrto %c%u", I.atype, I.addr); return;
    case PUSHv:   fprintf(out, "pushv %u", I.addr); return;
    case PUSHc:   fprintf(out, "pushc[]"); return;
    case PUSHi:   fprintf(out, "pushi[]"); return;
    case PUSHf:   fprintf(out, "pushf[]"); return;

    case COPY:    fprintf(out, "copy"); return;
    case MOVE:    fprintf(out, "move %u", I.addr);  return;

    case POPX:    fprintf(out, "popx");  return;
    case POP:     fprintf(out, "pop %c%u", I.atype, I.addr); return;
    case POPc:    fprintf(out, "popc[]"); return;
    case POPi:    fprintf(out, "popi[]"); return;
    case POPf:    fprintf(out, "popf[]"); return;

    case CALL:    fprintf(out, "call (%c) %u", I.atype, I.addr); return;
    case RET:     fprintf(out, "ret"); return;

    case INCc:    fprintf(out, "++c"); return;
    case INCi:    fprintf(out, "++i"); return;
    case INCf:    fprintf(out, "++f"); return;
    case DECc:    fprintf(out, "--c"); return;
    case DECi:    fprintf(out, "--i"); return;
    case DECf:    fprintf(out, "--f"); return;
    case NEGc:    fprintf(out, "negc"); return;
    case NEGi:    fprintf(out, "negi"); return;
    case NEGf:    fprintf(out, "negf"); return;
    case FLIP:    fprintf(out, "flip"); return;
    case CONVif:  fprintf(out, "convif"); return;
    case CONVfi:  fprintf(out, "convfi"); return;

    case PLUSc:   fprintf(out, "+c"); return;
    case PLUSi:   fprintf(out, "+i"); return;
    case PLUSf:   fprintf(out, "+f"); return;
    case MINUSc:  fprintf(out, "-c"); return;
    case MINUSi:  fprintf(out, "-i"); return;
    case MINUSf:  fprintf(out, "-f"); return;
    case STARc:   fprintf(out, "*c"); return;
    case STARi:   fprintf(out, "*i"); return;
    case STARf:   fprintf(out, "*f"); return;
    case SLASHc:  fprintf(out, "/c"); return;
    case SLASHi:  fprintf(out, "/i"); return;
    case SLASHf:  fprintf(out, "/f"); return;
    case MODc:    fprintf(out, "%%c"); return;
    case MODi:    fprintf(out, "%%i"); return;
    case AND:     fprintf(out, "&");  return;
    case OR:      fprintf(out, "|");  return;

    case GOTO:    fprintf(out, "goto %u", I.addr); return;
    case IFZc:    fprintf(out, "==0c %u", I.addr); return;
    case IFZi:    fprintf(out, "==0i %u", I.addr); return;
    case IFZf:    fprintf(out, "==0f %u", I.addr); return;
    case IFNZc:   fprintf(out, "!=0c %u", I.addr); return;
    case IFNZi:   fprintf(out, "!=0i %u", I.addr); return;
    case IFNZf:   fprintf(out, "!=0f %u", I.addr); return;
    case IFEQc:   fprintf(out, "==c %u", I.addr); return;
    case IFEQi:   fprintf(out, "==i %u", I.addr); return;
    case IFEQf:   fprintf(out, "==f %u", I.addr); return;
    case IFNEc:   fprintf(out, "!=c %u", I.addr); return;
    case IFNEi:   fprintf(out, "!=i %u", I.addr); return;
    case IFNEf:   fprintf(out, "!=f %u", I.addr); return;
    case IFLTc:   fprintf(out, "<c %u", I.addr); return;
    case IFLTi:   fprintf(out, "<i %u", I.addr); return;
    case IFLTf:   fprintf(out, "<f %u", I.addr); return;
    case IFLEc:   fprintf(out, "<=c %u", I.addr); return;
    case IFLEi:   fprintf(out, "<=i %u", I.addr); return;
    case IFLEf:   fprintf(out, "<=f %u", I.addr); return;
    case IFGTc:   fprintf(out, ">c %u", I.addr); return;
    case IFGTi:   fprintf(out, ">i %u", I.addr); return;
    case IFGTf:   fprintf(out, ">f %u", I.addr); return;
    case IFGEc:   fprintf(out, ">=c %u", I.addr); return;
    case IFGEi:   fprintf(out, ">=i %u", I.addr); return;
    case IFGEf:   fprintf(out, ">=f %u", I.addr); return;

    case NONE:    fprintf(out, "no-op"); return;
    case ERROR:   fprintf(out, "error"); return;
  }
}

/*
  Function data
*/

typedef struct {
  char* name;
  unsigned parameter_slots;
  unsigned return_slots;
  unsigned local_slots;
  instruction* code;
  unsigned code_length;
} function;

void zeroFunc(function *F)
{
  assert(F);
  F->name = 0;
  F->parameter_slots = 0;
  F->return_slots = 0;
  F->local_slots = 0;
  F->code = 0;
  F->code_length = 0;
}

const unsigned BUILTIN_FUNCTIONS = 2;

void initBuiltins(function* F)
{
  assert(F);
  /*
    Builtin 0: int getchar()
  */
  F[0].name = strdup("getchar");
  F[0].return_slots = 1;
  /*
    Builtin 1: int putchar(int)
  */
  F[1].name = strdup("putchar");
  F[1].parameter_slots = 1;
  F[1].return_slots = 1;
}

/*
  probably also:

void callBuiltin(unsigned fnum, stack?);

*/

void showFunction(FILE* out, unsigned fnum, function* F)
{
  if (0==F) return;
  if (0==F->name) return;
  fprintf(out, "Func %u: %s\n", fnum, F->name);
  fprintf(out, "    .params %u\n", F->parameter_slots);
  fprintf(out, "    .return %u\n", F->return_slots);
  fprintf(out, "    .locals %u\n", F->local_slots);
  if (fnum < BUILTIN_FUNCTIONS) {
    fprintf(out, "    built-in function\n");
    return;
  }
  fprintf(out, "    instructions:\n");
  unsigned i;
  for (i=0; i<F->code_length; i++) {
    fprintf(out, "\t%4u ", i);
    showInstruction(out, F->code[i]);
    fprintf(out, "\n");
  }
}

/*

  Input file parsing

*/

int lineno;

void compileError(const char* s)
{
  fprintf(stderr, "Error line %d: %s\n", lineno, s);
  exit(1);
}

void compileError2(const char* s, const char* s2)
{
  fprintf(stderr, "Error line %d: %s%s\n", lineno, s, s2);
  exit(1);
}

void compileError3(const char* s, const char* s2, const char* s3)
{
  fprintf(stderr, "Error line %d: %s%s%s\n", lineno, s, s2, s3);
  exit(1);
}

void compileError3u(const char* s, unsigned s2, const char* s3)
{
  fprintf(stderr, "Error line %d: %s%u%s\n", lineno, s, s2, s3);
  exit(1);
}

void compileError4(const char* s, const char* s2, const char* s3, const char* s4)
{
  fprintf(stderr, "Error line %d: %s%s%s%s\n", lineno, s, s2, s3, s4);
  exit(1);
}

void skipWS(FILE* in)
{
  char in_comment = 0;
  for (;;) {
    int c = fgetc(in);
    if ('\n' == c) {
      lineno++;
      in_comment = 0;
      continue;
    }
    if (in_comment) continue;
    if (' ' == c) continue;
    if ('\t' == c) continue;
    if ('\r' == c) continue;
    if (';' == c) {
      in_comment = 1;
      continue;
    }
    /* Actual character or EOF, put it back */
    ungetc(c, in);
    return;
  }
}

void matchKeyword(FILE* in, const char* keyword)
{
  skipWS(in);
  while (*keyword) {
    int c = fgetc(in); 
    if (c == *keyword) {
      keyword++;
      continue;
    }
    compileError2("expected ", keyword);
  }
}

unsigned readHex(FILE* in)
{
  skipWS(in);
  unsigned out;
  int ok = fscanf(in, "%x", &out);
  if (ok != 1) {
    compileError("expected hex value");
  }
  return out;
}

unsigned readUnsigned(FILE* in, const char* what)
{
  skipWS(in);
  unsigned out;
  int ok = fscanf(in, "%u", &out);
  if (ok != 1) {
    compileError2("expected ", what);
  }
  return out;
}

unsigned readLabel(FILE* in)
{
  skipWS(in);
  unsigned out;
  int ok = fscanf(in, "I%u:", &out);
  if (ok != 1) {
    compileError("expected label");
  }
  return out; 
}

void readAddress(FILE* in, instruction *I)
{
  assert(I);
  skipWS(in);
  int c = fgetc(in);
  switch (c) {
    case CONST:
    case GLOBAL:
    case LOCAL:
        I->atype = c;
        I->addr = readUnsigned(in, "address");
        return;

    default:
        compileError("expected address type C, G, or L");
  }
}

/*
  Return true if we fit into the buffer
*/
char readToken(FILE* in, char* buffer, int bufsize)
{
  skipWS(in);
  int i;
  for (i=0; i<bufsize; i++) {
    int c = fgetc(in);
    if ( (EOF==c) || (' '==c) || ('\n'==c) || ('\t'==c) || ('\r'==c) || (';'==c)) {
      ungetc(c, in);
      buffer[i] = 0;
      return 1;
    }
    buffer[i] = c;
  }
  return 0;
}

char* readIdent(FILE* in)
{
  skipWS(in);
  const int bufsize = 256;  // max identifier length
  char buffer[bufsize];
  int i;
  for (i=0; i<bufsize; i++) {
    int c = fgetc(in);
    if ( ('_' == c) || (c>='a' && c<='z') || (c>='A' && c<='Z') )
    {
      buffer[i] = c;
      continue;
    }
    ungetc(c, in);
    buffer[i] = 0;
    return strdup(buffer);
  }
  compileError("identifier too long");
  return 0; // keep compilers happy
}

opcode string2opcode(const char* buffer)
{
  /*
    Abandon hope all ye who enter here.
  */

  if (0==buffer) return ERROR;
  switch (buffer[0]) {
    case 0: return NONE;  // empty string

    case 'c': 
        // call, convif, convfi, copy
        switch (buffer[1]) {
          case 'a': return strcmp("ll", buffer+2) ? ERROR : CALL;
          case 'o':
              switch (buffer[2]) {
                case 'p': return strcmp("y", buffer+3) ? ERROR : COPY;
                case 'n':
                    if ('v' != buffer[3]) return ERROR;
                    switch (buffer[4]) {
                      case 'f': return strcmp("i", buffer+5) ? ERROR : CONVfi;
                      case 'i': return strcmp("f", buffer+5) ? ERROR : CONVif;
                      default : return ERROR;
                    }
                default : return ERROR;
              }
          default : return ERROR;
        }
        return ERROR;

    case 'f':
        // flip
        return strcmp("lip", buffer+1) ? ERROR : FLIP;

    case 'g':
        // goto
        return strcmp("oto", buffer+1) ? ERROR : GOTO;

    case 'm':
        // move
        return strcmp("ove", buffer+1) ? ERROR : MOVE;

    case 'n':
        // neg<type>
        if ( ('e'==buffer[1]) && ('g'==buffer[2]) ) {
          switch (buffer[3]) {
            case 'c': return NEGc;
            case 'i': return NEGi;
            case 'f': return NEGf;
            default : return ERROR;
          }
        }
        return ERROR;

    case 'p':
        // pop, popx, popc[], popi[], popf[]
        if (('o' == buffer[1]) && ('p'==buffer[2])) {
          // one of the pops
          switch (buffer[3]) {
              case 0:   return POP;
              case 'c': return strcmp("[]", buffer+4) ? ERROR : POPc;
              case 'i': return strcmp("[]", buffer+4) ? ERROR : POPi;
              case 'f': return strcmp("[]", buffer+4) ? ERROR : POPf;
              case 'x': return buffer[4] ? ERROR : POPX;
              default : return ERROR;
          }
          // sanity check
          return ERROR;
        }
        // ptrto
        if ('t' == buffer[1]) return strcmp("rto", buffer+2) ? ERROR : PTRTO;
        // push, pushc, pushi, pushf
        if (('u' == buffer[1]) && ('s'==buffer[2]) && ('h'==buffer[3])) {
          // one of the pushes
          switch (buffer[4]) {
              case 0:   return PUSH;
              case 'v': return buffer[5] ? ERROR : PUSHv;
              case 'c': return strcmp("[]", buffer+5) ? ERROR : PUSHc;
              case 'i': return strcmp("[]", buffer+5) ? ERROR : PUSHi;
              case 'f': return strcmp("[]", buffer+5) ? ERROR : PUSHf;
              default : return ERROR;
          }
        }
        return ERROR;

    case 'r':
        // ret
        return strcmp("et", buffer+1) ? ERROR : RET;

    case '+':
        // +t or ++t
        switch (buffer[1]) {
          case 'c': return PLUSc;
          case 'i': return PLUSi;
          case 'f': return PLUSf;
          case '+':
              switch (buffer[2]) {
                case 'c': return INCc;
                case 'i': return INCi;
                case 'f': return INCf;
                default : return ERROR;
              }
          default : return ERROR;
        }
        // sanity check
        return ERROR;

    case '-':
        // -t or --t
        switch (buffer[1]) {
          case 'c': return MINUSc;
          case 'i': return MINUSi;
          case 'f': return MINUSf;
          case '-':
              switch (buffer[2]) {
                case 'c': return DECc;
                case 'i': return DECi;
                case 'f': return DECf;
                default : return ERROR;
              }
          default : return ERROR;
        }
        // sanity check
        return ERROR;

    case '*':
        // *t
        switch (buffer[1]) {
          case 'c': return STARc;
          case 'i': return STARi;
          case 'f': return STARf;
          default : return ERROR;
        }
        // sanity check
        return ERROR;

    case '/':
        // /t
        switch (buffer[1]) {
          case 'c': return SLASHc;
          case 'i': return SLASHi;
          case 'f': return SLASHf;
          default : return ERROR;
        }
        // sanity check
        return ERROR;

    case '%':
        // %t
        switch (buffer[1]) {
          case 'c': return MODc;
          case 'i': return MODi;
          default : return ERROR;
        }
        // sanity check
        return ERROR;

    case '&':
        return (0==buffer[1]) ? AND : ERROR;

    case '|':
        return (0==buffer[1]) ? OR  : ERROR;

    case '=':
        // ==0t or ==t
        if (buffer[1] != '=') return ERROR;
        switch (buffer[2]) {
          case 'c': return IFEQc;
          case 'i': return IFEQi;
          case 'f': return IFEQf;
          case '0':
              switch (buffer[3]) {
                case 'c': return IFZc;
                case 'i': return IFZi;
                case 'f': return IFZf;
                default : return ERROR;
              }
          default : return ERROR;
        }
        // sanity check
        return ERROR;

    case '!':
        // !=0t or !=t
        if (buffer[1] != '=') return ERROR;
        switch (buffer[2]) {
          case 'c': return IFNEc;
          case 'i': return IFNEi;
          case 'f': return IFNEf;
          case '0':
              switch (buffer[3]) {
                case 'c': return IFNZc;
                case 'i': return IFNZi;
                case 'f': return IFNZf;
                default : return ERROR;
              }
          default : return ERROR;
        }
        // sanity check
        return ERROR;

    case '<':
        // <t or <=t
        switch (buffer[1]) {
          case 'c': return IFLTc;
          case 'i': return IFLTi;
          case 'f': return IFLTf;
          case '=':
              switch (buffer[2]) {
                case 'c': return IFLEc;
                case 'i': return IFLEi;
                case 'f': return IFLEf;
                default : return ERROR;
              }
          default : return ERROR;
        }
        // sanity check
        return ERROR;

    case '>':
        // >t or >=t
        switch (buffer[1]) {
          case 'c': return IFGTc;
          case 'i': return IFGTi;
          case 'f': return IFGTf;
          case '=':
              switch (buffer[2]) {
                case 'c': return IFGEc;
                case 'i': return IFGEi;
                case 'f': return IFGEf;
                default : return ERROR;
              }
          default : return ERROR;
        }
        // sanity check
        return ERROR;

    default:
        return ERROR;
  }
  // sanity check
  return ERROR;
}


unsigned readConstants(FILE* in, segment* Mem)
{
  matchKeyword(in, ".CONSTANTS");
  unsigned slots = readUnsigned(in, "#constants");
  unsigned i;
  for (i=0; i<slots; i++) {
    Mem->data[i] = readHex(in);
  }
  return slots;
}

unsigned readGlobals(FILE* in)
{
  matchKeyword(in, ".GLOBALS");
  return readUnsigned(in, "#globals");
}

unsigned readFunctions(FILE* in)
{
  matchKeyword(in, ".FUNCTIONS");
  return readUnsigned(in, "#functions");
}

instruction finishReading(FILE* in, function* f, unsigned nc, unsigned ng, opcode op)
{
  assert(f);

  instruction I;
  I.op = op;

  switch (op) {
    /*
      Cases that require an address
    */
    case PUSH:
    case PTRTO:
        readAddress(in, &I);
        switch (I.atype) {
          case CONST:   if (I.addr >= nc) compileError("constant index too large");
                        break;
          case GLOBAL:  if (I.addr >= ng) compileError("global index too large");
                        break;
          case LOCAL:   if (I.addr >= f->parameter_slots + f->local_slots) {
                          compileError("local index too large");
                        }
                        break;
          default:
                        /* Should have caught this error already */
                        fprintf(stderr, "Internal error 1\n");
                        exit(1);
        }; /* switch */
        break;

    case POP:
        readAddress(in, &I);
        switch (I.atype) {
          case CONST:   compileError("target of pop cannot be a constant");
                        break;
          case GLOBAL:  if (I.addr >= ng) compileError("global index too large");
                        break;
          case LOCAL:   if (I.addr >= f->parameter_slots + f->local_slots) {
                          compileError("local index too large");
                        }
                        break;
          default:
                        /* Should have caught this error already */
                        fprintf(stderr, "Internal error 2\n");
                        exit(1);
        }; /* switch */
        break;

    /*
      Cases that require an unsigned integer
    */
    case CALL:
        I.atype = FNUM;
        I.addr = readUnsigned(in, "function number");
        break;

    case MOVE:
        I.atype = MOVEDIST;
        I.addr = readUnsigned(in, "move distance");
        break;

    /*
      Cases that require a label
    */
    case GOTO:
    case IFZc:
    case IFZi:
    case IFZf:
    case IFNZc:
    case IFNZi:
    case IFNZf:
    case IFEQc:
    case IFEQi:
    case IFEQf:
    case IFNEc:
    case IFNEi:
    case IFNEf:
    case IFLTc:
    case IFLTi:
    case IFLTf:
    case IFLEc:
    case IFLEi:
    case IFLEf:
    case IFGTc:
    case IFGTi:
    case IFGTf:
    case IFGEc:
    case IFGEi:
    case IFGEf:
        I.atype = LABEL;
        I.addr = readLabel(in);
        break;

    /*
      Cases that require a hex value
    */
    case PUSHv:
        I.atype = VALUE;
        I.addr = readHex(in);
        break;

    /*
      Default cases: no operand
    */
    default:
        I.atype = UNUSED;
        I.addr = 0;
  }
  return I;
}

unsigned readFuncNum(FILE* in)
{
  matchKeyword(in, ".FUNC");
  return readUnsigned(in, "function number");
}

void readFunc(FILE* in, function* F, unsigned nc, unsigned ng)
{
  assert(F);

  const int maxlabel = 5000;
  const char* maxlabelstring = "5000";
  unsigned labelmap[maxlabel];
  unsigned i;
  for (i=0; i<maxlabel; i++) {
    labelmap[i] = 0;
  }

  F->name = readIdent(in);
#ifdef DEBUG_PARSER
  printf("Parsing function %s\n", F->name);
#endif
  matchKeyword(in, ".params");
  F->parameter_slots = readUnsigned(in, "#slots");
  matchKeyword(in, ".return");
  F->return_slots = readUnsigned(in, "#slots");
  matchKeyword(in, ".locals");
  F->local_slots = readUnsigned(in, "#slots");
#ifdef DEBUG_PARSER
  printf("#param_slots %u #return_slots %u #locals %u\n",
    F->parameter_slots, F->return_slots, F->local_slots
  );
#endif

  unsigned codemax = 1024;
  instruction* CODE = malloc(codemax * sizeof(instruction));

  for (F->code_length=0; ; F->code_length++) {
    char token[10];   // in case of really, really long instruction numbers
    
    if (!readToken(in, token, 10)) {
      token[9] = 0;
      compileError3("token '", token, "' is too long\n(expecting instruction, label, or .end)");
      exit(1);
    }
    if (0==strcmp(".end", token)) break;

    if ('I'==token[0]) {
      /* This is a label. */
      long lnum = atol(token+1);
      if (lnum<0 || lnum >= maxlabel) {
        compileError4("label ", token, " exceeds maximum allowed: ", maxlabelstring);
      }
      if (labelmap[lnum]) {
        compileError3("label ", token, " already used in this function");
        exit(1);
      }
      labelmap[lnum] = 1+F->code_length--; /* +1 because 0 reserved for "not assigned" */
#ifdef DEBUG_LABELS
      printf("Label %s mapped to instruction %u\n", token, labelmap[lnum]);
#endif
      continue;
    }

    opcode OP = string2opcode(token);
    if (ERROR == OP) {
      compileError2("unknown instruction ", token);
    }

    if (F->code_length >= codemax) {
      // enlarge
      codemax *= 2;
      CODE = realloc(CODE, codemax * sizeof(instruction));
    }

    CODE[F->code_length] = finishReading(in, F, nc, ng, OP);

#ifdef DEBUG_PARSER
    printf("Instruction %u: ", F->code_length);
    showInstruction(stdout, CODE[F->code_length]);
    printf("\n");
#endif
  }

  // saw .end
  matchKeyword(in, "FUNC");

  /*
    Go through the code, replace labels with instruction numbers
  */
  for (i=0; i<F->code_length; i++) {
    if (LABEL==CODE[i].atype) {
      unsigned instrno = labelmap[CODE[i].addr];
      if (0==instrno) {
        compileError3u("label I", CODE[i].addr, " not found in this function");
      }
      instrno--;
      if (instrno >= F->code_length) {
        compileError3u("label I", CODE[i].addr, " past end of function");
      }
      CODE[i].addr = instrno;
    }
  }

  /*
    Shrink CODE array, store in F
  */
  F->code = realloc(CODE, F->code_length * sizeof(instruction));

#ifdef DEBUG_PARSER
  printf("Done parsing function %s\n", F->name);
#endif
}

/*
  Code execution
*/

const function* Fexecuting;
unsigned Finstruction;

void runtimeError(const char* e)
{
  fprintf(stderr, "Runtime error");
  if (Fexecuting) {
    fprintf(stderr, " in function %s instruction %u", 
      Fexecuting->name, Finstruction);
  }
  fprintf(stderr, ":\n%s\n", e);
  exit(2);
}

void runtimeError3(const char* e, unsigned e2, const char* e3)
{
  fprintf(stderr, "Runtime error");
  if (Fexecuting) {
    fprintf(stderr, " in function %s instruction %u", 
      Fexecuting->name, Finstruction);
  }
  fprintf(stderr, ":\n%s%u %s\n", e, e2, e3);
  exit(2);
}

typedef struct {
  segment constants;
  segment globals;
  function* F;
  unsigned nf;
} program;

void callBuiltin(unsigned fnum, segment* mylocals, stack* mystack)
{
  int c;
  switch (fnum) {
    case 0:   /* int getchar() */
      push(mystack, i2u(getchar()));    
      return;

    case 1:   /* int putchar(int c) */
      c = u2i(mylocals->data[0]);
      fputc(c, stdout);
      push(mystack, i2u(c));
      return;

    default:
      runtimeError3("Internal error 3: ", fnum, "not a builtin function");
  }
}

void callFunction(program* P, unsigned fnum, segment* locals, stack* compstack)
{
  assert(P);
  assert(locals);
  assert(compstack);

  if (fnum >= P->nf) {
    runtimeError3("target function number ", fnum, " is too large");
  }
  function* F = P->F + fnum;

#ifdef SHOW_EXECUTION
  fprintf(stderr, "Calling %s\n", F->name);
#endif
#ifdef SHOW_STACK
  showStack(stderr, "incoming stack: ", compstack);
#endif

  if (locals->size < F->parameter_slots + F->local_slots) {
    runtimeError3("local variable stack overflow\n    in call to function #", fnum, F->name);
  }

  /*
    Copy parameters from top of compstack to locals
  */
  if (compstack->top < F->parameter_slots) {
    runtimeError3("not enough parameters on computation stack\n    in call to function #", fnum, F->name);
  }

  unsigned i;
  unsigned slot = F->parameter_slots-1;
  for (i=0; i<F->parameter_slots; i++) {
    locals->data[slot--] = pop(compstack);
  }
#ifdef SHOW_EXECUTION
  fprintf(stderr, "params: ");
  for (i=0; i<F->parameter_slots; i++) {
    if (i) fprintf(stderr, ", ");
    fprintf(stderr, "%u", locals->data[i]);
  }
  fprintf(stderr, "\n");
#endif

  /*
    Make safe segments for our execution and for function calls below us
  */
  stack mystack;
  segment sublocals;

  makeSubStack(&mystack, compstack);  // prevent underflow
  makeSubSegment(locals, F->parameter_slots + F->local_slots, locals->size, &sublocals);

  /*
    Execute code
  */

  if (fnum < BUILTIN_FUNCTIONS) {
    callBuiltin(fnum, locals, &mystack);
  } else {
    unsigned pc = 0;
    for (;;) {
      Fexecuting = F;
      Finstruction = pc;

      /*
        Check that pc is in bounds
      */
      if (pc >= F->code_length) {
        runtimeError("ran past end of function; missing ret?");
      }
      instruction I = F->code[pc];
      pc++;   
#ifdef SHOW_EXECUTION
      fprintf(stderr, "Executing ");
      showInstruction(stderr, I);
      fputc('\n', stderr);
#endif
#ifdef SHOW_STACK
      showStack(stderr, "stack (before): ", &mystack);
#endif

      /*
        Execute instruction
      */
      unsigned leftu, rightu;
      int lefti, righti;
      float leftf, rightf;
      if (RET == I.op) break;
      switch (I.op) {
        case PUSH:    /* PUSH address */
                      switch (I.atype) {
                        case CONST: 
                                    assert(I.addr < P->constants.size);
                                    push(&mystack, P->constants.data[I.addr]);
                                    break;
                        case GLOBAL:
                                    assert(I.addr < P->globals.size);
                                    push(&mystack, P->globals.data[I.addr]);
                                    break;
                        case LOCAL:
                                    assert(I.addr < F->parameter_slots + F->local_slots);
                                    push(&mystack, locals->data[I.addr]);
                                    break;
                        default:
                                    runtimeError("Internal error 4");
                      };
                      break;
        case PTRTO:   /* PUSH ptr to addr */
                      switch (I.atype) {
                        case CONST:
                                    assert(I.addr < P->constants.size);
                                    push(&mystack, addr2ptr(&P->constants, I.addr));
                                    break;    
                        case GLOBAL:
                                    assert(I.addr < P->globals.size);
                                    push(&mystack, addr2ptr(&P->globals, I.addr));
                                    break;    
                        case LOCAL:
                                    assert(I.addr < F->parameter_slots + F->local_slots);
                                    push(&mystack, addr2ptr(locals, I.addr));
                                    break;    
                        default:
                                    runtimeError("Internal error 5");
                      };
                      break;
        case PUSHv:
                      assert(VALUE==I.atype);
                      push(&mystack, I.addr);
                      break;
        case PUSHc:
                      leftu = pop(&mystack);
                      righti = u2i(pop(&mystack));
                      push(&mystack, i2u(((char*)(locals->mem_base + leftu))[righti]));
                      break;
        case PUSHi:   
                      leftu = pop(&mystack);
                      righti = u2i(pop(&mystack));
                      push(&mystack, i2u(((int*)(locals->mem_base + leftu))[righti]));
                      break;
        case PUSHf:   
                      leftu = pop(&mystack);
                      righti = u2i(pop(&mystack));
                      push(&mystack, f2u(((float*)(locals->mem_base + leftu))[righti]));
                      break;
        case COPY:    
                      push(&mystack, top(&mystack));
                      break;
        case MOVE:
                      move(&mystack, I.addr);
                      break;
        case POPX:    /* Pop and discard */
                      pop(&mystack);
                      break;
        case POP:
                      if (GLOBAL == I.atype) {
                        // store in global
                        assert(I.addr < P->globals.size);
                        P->globals.data[I.addr] = pop(&mystack);
                      } else {
                        // store in local
                        assert(I.addr < F->parameter_slots + F->local_slots);
                        locals->data[I.addr] = pop(&mystack);
                      }
                      break;
        case POPc:    
                      righti = u2i(pop(&mystack));
                      leftu = pop(&mystack);
                      lefti = u2i(pop(&mystack));
                      ((char*)(locals->mem_base + leftu))[lefti] = righti;
                      break;
        case POPi:    
                      righti = u2i(pop(&mystack));
                      leftu = pop(&mystack);
                      lefti = u2i(pop(&mystack));
                      ((int*)(locals->mem_base + leftu))[lefti] = righti;
                      break;
        case POPf:    
                      rightf = u2f(pop(&mystack));
                      leftu = pop(&mystack);
                      lefti = u2i(pop(&mystack));
                      ((float*)(locals->mem_base + leftu))[lefti] = rightf;
                      break;

        case CALL:    /* CALL fnum */
                      assert(FNUM == I.atype);
                      callFunction(P, I.addr, &sublocals, &mystack);
                      break;

        case RET:     /* Can't happen */
                      runtimeError("Internal error 6");
                      break;  // sanity
        case INCc:
        case INCi: 
                      (*((int*)topptr(&mystack)))++;
                      break;
        case INCf: 
                      (*((float*)topptr(&mystack)))++;
                      break;
        case DECc: 
        case DECi: 
                      (*((int*)topptr(&mystack)))--;
                      break;
        case DECf: 
                      (*((float*)topptr(&mystack)))--;
                      break;
        case NEGc: 
        case NEGi: 
                      (*((int*)topptr(&mystack))) *= -1;
                      break;
        case NEGf: 
                      (*((float*)topptr(&mystack))) *= -1.0;
                      break;
        case FLIP: 
                      push(&mystack, ~pop(&mystack));
                      break;
        case CONVif:
                      lefti = u2i(pop(&mystack));
                      push(&mystack, f2u(lefti));
                      break;
        case CONVfi:  
                      leftf = u2f(pop(&mystack));
                      push(&mystack, i2u((int)leftf));
                      break;
        case PLUSc:
        case PLUSi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      push(&mystack, i2u(lefti + righti));
                      break;
        case PLUSf:
                      rightf = u2f(pop(&mystack));
                      leftf = u2f(pop(&mystack));
                      push(&mystack, f2u(leftf + rightf));
                      break;
        case MINUSc:
        case MINUSi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      push(&mystack, i2u(lefti - righti));
                      break;
        case MINUSf:
                      rightf = u2f(pop(&mystack));
                      leftf = u2f(pop(&mystack));
                      push(&mystack, f2u(leftf - rightf));
                      break;
        case STARc:
        case STARi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      push(&mystack, i2u(lefti * righti));
                      break;
        case STARf:
                      rightf = u2f(pop(&mystack));
                      leftf = u2f(pop(&mystack));
                      push(&mystack, f2u(leftf * rightf));
                      break;
        case SLASHc:
        case SLASHi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      push(&mystack, i2u(lefti / righti));
                      break;
        case SLASHf:
                      rightf = u2f(pop(&mystack));
                      leftf = u2f(pop(&mystack));
                      push(&mystack, f2u(leftf / rightf));
                      break;
        case MODc:
        case MODi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      push(&mystack, i2u(lefti % righti));
                      break;
        case AND: 
                      rightu = pop(&mystack);
                      leftu = pop(&mystack);
                      push(&mystack, leftu & rightu);
                      break;
        case OR:
                      rightu = pop(&mystack);
                      leftu = pop(&mystack);
                      push(&mystack, leftu | rightu);
                      break;
        case GOTO:
                      assert(LABEL == I.atype);
                      assert(I.addr < F->code_length);
                      pc = I.addr;
                      break;
        case IFZc:
        case IFZi:  
                      lefti = u2i(pop(&mystack));
                      if (0==lefti) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFZf:
                      leftf = u2f(pop(&mystack));
                      if (0==leftf) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFNZc:
        case IFNZi:
                      lefti = u2i(pop(&mystack));
                      if (0!=lefti) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFNZf:
                      leftf = u2f(pop(&mystack));
                      if (0!=leftf) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFEQc:
        case IFEQi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      if (lefti == righti) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFEQf:
                      rightf = u2f(pop(&mystack));
                      leftf = u2f(pop(&mystack));
                      if (leftf == rightf) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFNEc:
        case IFNEi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      if (lefti != righti) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFNEf:
                      rightf = u2f(pop(&mystack));
                      leftf = u2f(pop(&mystack));
                      if (leftf != rightf) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFLTc:
        case IFLTi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      if (lefti < righti) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFLTf:
                      rightf = u2f(pop(&mystack));
                      leftf = u2f(pop(&mystack));
                      if (leftf < rightf) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFLEc:
        case IFLEi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      if (lefti <= righti) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFLEf:
                      rightf = u2f(pop(&mystack));
                      leftf = u2f(pop(&mystack));
                      if (leftf <= rightf) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFGTc:
        case IFGTi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      if (lefti > righti) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFGTf:
                      rightf = u2f(pop(&mystack));
                      leftf = u2f(pop(&mystack));
                      if (leftf > rightf) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFGEc:
        case IFGEi:
                      righti = u2i(pop(&mystack));
                      lefti = u2i(pop(&mystack));
                      if (lefti >= righti) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;
        case IFGEf:
                      rightf = u2f(pop(&mystack));
                      leftf = u2f(pop(&mystack));
                      if (leftf >= rightf) {
                        assert(I.addr < F->code_length);
                        pc = I.addr;
                      }
                      break;

        default:      /* NONE or ERROR; do nothing */
                      ;
      } /* switch */
    } /* for(;;) */ 
  } /* If not builtin */

  /*
    Save return value
  */
  if (F->return_slots) {
    assert(1==F->return_slots);
    push(compstack, pop(&mystack));
  }

#ifdef SHOW_EXECUTION
  fprintf(stderr, "Exiting %s\n", F->name);
#endif
}

/*
  Main
*/

const char* usage = 
"  Read intermediate code; if no file specified, reads standard input.\n\
  The lowest numbered function named main is called.\n\n\
  Input format is ComS 440/540 intermediate representation, which\n\
  is a free-form text file.  Any text between ; and end of line is\n\
  ignored.  Format is the following:\n\
    .CONSTANTS (number of slots)\n\
    (slot 1 as 32-bit unsigned hex)\n\
    (slot 2 as 32-bit unsigned hex)\n\
    ...\n\
    .GLOBALS (number of slots)\n\
    .FUNCTIONS (number of functions)\n\
    .FUNC number name\n\
      .params (number of slots)\n\
      .return (number of slots (0 or 1))\n\
      .locals (number of slots)\n\
      0 instruction-0\n\
      1 instruction-1\n\
      ...\n\
    .end FUNC\n\
    .FUNC\n\
      ...\n\
    .end FUNC\n\n\
  Legal instructions: see project spec document.\n\n\
  Built-in functions:\n\
    Function 0: int getchar()\n\
    Function 1: int putchar(int c)\n\
  So, don't overwrite those.\n\n";

int main(int argc, const char** argv)
{
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [file]\n\n", argv[0]);
    fputs(usage, stderr);
    return 1;
  }
  FILE* in;
  if (argv[1]) {
    in = fopen(argv[1], "r");
    if (0==in) {
      fprintf(stderr, "Error, couldn't open file '%s'\n", argv[1]);
      return 1;
    }
  } else {
    in = stdin;
  }
  lineno = 1;

/* 
  Initialize "memory"
*/

  program P;
  segment main_memory, locals;
  initSegment(&main_memory, 65536);
  segment compstack_memory;
  initSegment(&compstack_memory, 65536);
  stack compstack;
  initStack(&compstack_memory, 0, &compstack);
  Fexecuting = 0;
  Finstruction = 0;

/*
  Parse constants, globals
*/
  unsigned nc = readConstants(in, &main_memory);
  unsigned ng = readGlobals(in);
  makeSubSegment(&main_memory, 0, nc, &P.constants);    /* Constant segment */
  makeSubSegment(&main_memory, nc, nc+ng, &P.globals);  /* Globals segment */
  makeSubSegment(&main_memory, nc+ng, main_memory.size, &locals);   /* Rest - locals */
  
/*
  Parse functions
*/
  P.nf = readFunctions(in);
  P.nf += BUILTIN_FUNCTIONS;  // reserved functions
  P.F = malloc(P.nf * sizeof(function));
  unsigned f;
  for (f=0; f<P.nf; f++) {
    zeroFunc(P.F+f);
  }
  initBuiltins(P.F);

#ifdef DEBUG_PARSER
  printf("Reading %u functions\n", P.nf);
#endif
  for (f=BUILTIN_FUNCTIONS; f<P.nf; f++) {
    unsigned fnum = readFuncNum(in);
    if (fnum >= P.nf) {
      fprintf(stderr, "Error line %d: function number %u too large (max %u)\n",
        lineno, fnum, P.nf);
      exit(1);
    }
    if (P.F[fnum].name) {
      fprintf(stderr, "Error line %d: function number %u in use by %s\n",
        lineno, fnum, P.F[fnum].name);
      exit(1);
    }
    readFunc(in, P.F+fnum, nc, ng);
  }


#ifdef SHOW_PROGRAM
  printf("Done reading input.\n");
  printf("Constants:\n");
  showSegment(&P.constants);
  printf("Functions:\n");
  for (f=0; f<P.nf; f++) {
    showFunction(stdout, f, P.F+f);
  }
#endif

  unsigned entry = 0;
  for (f=P.nf-1; f; f--) {
    if (0==P.F[f].name) continue;
    if (0==strcmp("main", P.F[f].name)) entry = f;
  }

  if (!entry) {
    fprintf(stderr, "Error, no function named main to execute\n");
    return 2;
  }
#ifdef SHOW_PROGRAM
  else {
    printf("Entry point is function #%u\n", entry);
  }
#endif

  callFunction(&P, entry, &locals, &compstack);
  printf("Function main returned: %d\n", u2i(top(&compstack)));

  return 0;
}

