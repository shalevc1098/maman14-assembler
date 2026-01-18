/* include guard to define only once */
#ifndef WARNS_H
#define WARNS_H

/* warning printing macros */
#define WARN_LINE(line, msg) fprintf(stderr, "Warning on line %d: %s\n", line, msg)

/* directive warnings */
#define WARN_LABEL_BEFORE_EXTERN "label before .extern is meaningless"

#endif