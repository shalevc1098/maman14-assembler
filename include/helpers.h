/* include guard to define only once */
#ifndef HELPERS_H
#define HELPERS_H

/* needed for FILE, might warn
 * the below comment tells clangd to keep this include even if it looks unused
 */
#include <stdio.h> /* IWYU pragma: keep */

/* discards remaining characters in line until newline or EOF */
void discard_rest_of_line(FILE *file);

#endif