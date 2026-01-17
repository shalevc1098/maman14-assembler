/* include guard to define only once */
#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdlib.h>

#include "bool.h"

/* addressing modes */
#define ADDR_IMMEDIATE 0
#define ADDR_DIRECT 1
#define ADDR_RELATIVE 2
#define ADDR_REGISTER 3

/* instruction info struct */
typedef struct {
    char *name;
    int opcode;
    int funct;
    int num_operands;
    /* bitmask of allowed addressing modes (bit 0=immediate, 1=direct, 2=relative, 3=register) */
    int src_modes;
    int dest_modes;
} InstructionInfo;

/* instruction table */
static const InstructionInfo INSTRUCTION_TABLE[] = {
    /* two operand instructions */
    {"mov", 0, 0, 2, 0xB, 0xA},  /* src: 0,1,3  dest: 1,3 */
    {"cmp", 1, 0, 2, 0xB, 0xB},  /* src: 0,1,3  dest: 0,1,3 */
    {"add", 2, 10, 2, 0xB, 0xA}, /* src: 0,1,3  dest: 1,3 */
    {"sub", 2, 11, 2, 0xB, 0xA}, /* src: 0,1,3  dest: 1,3 */
    {"lea", 4, 0, 2, 0x2, 0xA},  /* src: 1      dest: 1,3 */
    /* one operand instructions */
    {"clr", 5, 10, 1, 0, 0xA}, /* dest: 1,3 */
    {"not", 5, 11, 1, 0, 0xA}, /* dest: 1,3 */
    {"inc", 5, 12, 1, 0, 0xA}, /* dest: 1,3 */
    {"dec", 5, 13, 1, 0, 0xA}, /* dest: 1,3 */
    {"jmp", 9, 10, 1, 0, 0x6}, /* dest: 1,2 */
    {"bne", 9, 11, 1, 0, 0x6}, /* dest: 1,2 */
    {"jsr", 9, 12, 1, 0, 0x6}, /* dest: 1,2 */
    {"red", 12, 0, 1, 0, 0xA}, /* dest: 1,3 */
    {"prn", 13, 0, 1, 0, 0xB}, /* dest: 0,1,3 */
    /* zero operand instructions */
    {"rts", 14, 0, 0, 0, 0},
    {"stop", 15, 0, 0, 0, 0},
    /* terminator */
    {NULL, 0, 0, 0, 0, 0}};

/* lookups instruction by name, returns NULL if not found */
const InstructionInfo *get_instruction_info(char *name);
/* checks if addressing mode is valid for source operand */
Bool is_valid_src_mode(const InstructionInfo *info, int mode);
/* checks if addressing mode is valid for destination operand */
Bool is_valid_dest_mode(const InstructionInfo *info, int mode);

#endif