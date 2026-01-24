/* include guard to define only once */
#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "bool.h"
#include "hash_table.h"

/* max memory */
#define MAX_MEMORY 4096
/* 80 chars + newline + null */
#define MAX_LINE 82
/* 31 chars + null */
#define MAX_LABEL 32
/* where to start ic count from */
#define IC_START 100
/* comment character */
#define COMMENT_CHAR ';'
/* min number */
#define MIN_NUMBER -2048
/* max number */
#define MAX_NUMBER 2047

/* reserved words - registers */
extern const char *REGISTERS[];

/* reserved words - directives */
extern const char *DIRECTIVES[];

/* ARE enum */
typedef enum { ARE_A, ARE_R, ARE_E } ARE;

/* memory word (stores 12-bit value + ARE marking) */
typedef struct {
    int value;
    ARE are;
} Word;

/* tracks where an external symbol is used (for .ext output) */
typedef struct {
    char name[MAX_LABEL];
    int address;
} External;

/* shared state between assembler passes */
typedef struct {
    HashTable *symbols;
    Word *code;
    Word *data;
    External *externals;
    int ic;
    int dc;
    int ec; /* external count */
} AssemblerState;

/* checks if program would have enough memory after adding "additional" to */
Bool has_memory(int ic, int dc, int additional);
/* frees assembler state, returns NULL */
AssemblerState *free_assembler_state(AssemblerState *state);

#endif