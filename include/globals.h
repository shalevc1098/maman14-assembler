/* include guard to define only once */
#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdlib.h>

/* max memory */
#define MAX_MEMORY 4096
/* 80 chars + null */
#define MAX_LINE 81
/* 31 chars + null */
#define MAX_LABEL 32
/* where to start ic count from */
#define IC_START 100

/* reserved words - instructions */
static const char *INSTRUCTIONS[] = {"mov", "cmp", "add", "sub", "lea", "clr", "not",  "inc", "dec",
                                     "jmp", "bne", "jsr", "red", "prn", "rts", "stop", NULL};

/* reserved words - registers */
static const char *REGISTERS[] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", NULL};

/* reserved words - directives */
static const char *DIRECTIVES[] = {".data", ".string", ".entry", ".extern", "mcro", "mcroend", NULL};

/* ARE enum */
typedef enum { ARE_A, ARE_R, ARE_E } ARE;

/* memory word (stores 12-bit value + ARE marking) */
typedef struct {
    int value;
    ARE are;
} Word;

/* i defined these because ansi c has no bool */
typedef enum { false, true } Bool;

#endif