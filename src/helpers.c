#include <stdio.h>

#include "helpers.h"

void discard_rest_of_line(FILE *file) {
    int c;
    /* discard characters until newline or end of file */
    while ((c = fgetc(file)) != '\n' && c != EOF);
}