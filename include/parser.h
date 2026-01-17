/* include guard to define only once */
#ifndef PARSER_H
#define PARSER_H

#include "bool.h"

/* skips spaces/tabs, returns pointer to first non-whitespace char */
char *skip_whitespace(char *str);
/* gets next word, returns pointer to rest of string */
char *get_token(char *str, char *dest);
/* checks if name is an instruction */
Bool is_instruction(char *name);
/* checks if name is a register */
Bool is_register(char *name);
/* checks if name is a directive */
Bool is_directive(char *name);
/* checks if name is a reserved word */
Bool is_reserved_word(char *name);
/* checks if str is empty or whitespace only */
Bool is_empty(char *str);
/* checks if str is a valid number */
Bool is_number(char *str);

#endif