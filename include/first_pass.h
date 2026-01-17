/* include guard to define only once */
#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "assembler.h"

/* parses .am file, builds symbol table, encodes instructions/data, returns AssemblerState on success, NULL on error */
AssemblerState* first_pass(char *filename);

#endif