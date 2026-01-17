#include <string.h>

#include "bool.h"
#include "instructions.h"

const InstructionInfo *get_instruction_info(char *name) {
    /* index tracker */
    int i;
    for (i = 0; INSTRUCTION_TABLE[i].name != NULL; i++) {
        /* compare name with INSTRUCTION_TABLE[i].name */
        if (strcmp(name, INSTRUCTION_TABLE[i].name) == 0)
            return &INSTRUCTION_TABLE[i]; /* the instruction info of instruction "name" */
    }
    return NULL;
}

Bool is_valid_src_mode(const InstructionInfo *info, int mode) {
    /* check if bit at position "mode" is set in src_modes */
    return (info->src_modes & (1 << mode)) != 0;
}

Bool is_valid_dest_mode(const InstructionInfo *info, int mode) {
    /* check if bit at position "mode" is set in dest_modes */
    return (info->dest_modes & (1 << mode)) != 0;
}