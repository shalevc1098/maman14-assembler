/* include guard to define only once */
#ifndef PRE_ASSEMBLER_H
#define PRE_ASSEMBLER_H

#include "globals.h"

/* struct for macros */
typedef struct {
    char **lines; /* array of lines inside macro */
    int line_count; /* count of lines inside macro */
} Macro;

/* expands macros from .as file, outputs .am file, returns true on success, false on error */
Bool pre_assemble(char *filename);

#endif