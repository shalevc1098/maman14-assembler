#include <stdlib.h>

#include "assembler.h"
#include "bool.h"
#include "hash_table.h"
#include "instructions.h"

Bool has_memory(int ic, int dc, int additional) {
    return ic + dc + additional <= MAX_MEMORY;
}

AssemblerState *free_assembler_state(AssemblerState *state) {
    /* if state is not NULL, free its child along with it */
    if (state) {
        /* free symbols table */
        hash_table_free(state->symbols, free);
        /* free code array */
        free(state->code);
        /* free data array */
        free(state->data);
        /* free externals array */
        free(state->externals);
        /* free state */
        free(state);
    }

    /* return NULL because the program reassigns state with that return value */
    return NULL;
}

int get_addressing_mode(char *operand) {
    if (*operand == '#')
        return ADDR_IMMEDIATE;
    else if (*operand == '%')
        return ADDR_RELATIVE;
    else if (*operand == 'r' && *(operand + 1) >= '0' && *(operand + 1) <= '7' && *(operand + 2) == '\0')
        return ADDR_REGISTER;
    else
        return ADDR_DIRECT;
}