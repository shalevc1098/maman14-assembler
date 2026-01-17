#include <stdlib.h>

#include "assembler.h"
#include "bool.h"
#include "hash_table.h"

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
        /* free state */
        free(state);
    }

    /* return NULL because the program reassigns state with that return value */
    return NULL;
}