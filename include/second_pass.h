/* include guard to define only once */
#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "assembler.h"
#include "bool.h"

/* resolves symbol addresses, processes .entry, returns true on success */
Bool second_pass(char *filename, AssemblerState *state);

#endif