#include <string.h>

#include "globals.h"
#include "parser.h"

char *skip_whitespace(char *str) {
    /* loops until the char at *str is not whitespace */
    /* loops as long as *str is not whitespace  */
    while (*str == ' ' || *str == '\t')
        str++;  /* moves str to the next char */
    return str; /* returns the str starting from the first non-whitespace
                   character */
}

char *get_token(char *str, char *dest) {
    str = skip_whitespace(str); /* skip leading whitespaces */
    /* loops while *str is not a whitespace or a NULL terminator  */
    while (*str != ' ' && *str != '\t' && *str != '\0') {
        *dest = *str; /* assign *str char to *dest char */
        dest++;       /* move dest to next char */
        str++;        /* move str to next char */
    }
    *dest = '\0'; /* add NULL terminator at the end of dest */
    return str;   /* return pointer to where we stopped */
}

Bool is_instruction(char *name) {
    /* index tracker */
    int i;
    for (i = 0; INSTRUCTIONS[i] != NULL; i++) {
        /* compare name with INSTRUCTIONS[i] */
        if (strcmp(name, INSTRUCTIONS[i]) == 0)
            return true; /* if name is an instruction */
    }
    return false; /* if name is not an instruction */
}

Bool is_register(char *name) {
    /* index tracker */
    int i;
    for (i = 0; REGISTERS[i] != NULL; i++) {
        /* compare name with REGISTERS[i] */
        if (strcmp(name, REGISTERS[i]) == 0)
            return true; /* if name is a register */
    }
    return false; /* if name is not a register */
}

Bool is_directive(char *name) {
    /* index tracker */
    int i;
    for (i = 0; DIRECTIVES[i] != NULL; i++) {
        /* compare name with DIRECTIVES[i] */
        if (strcmp(name, DIRECTIVES[i]) == 0)
            return true; /* if name is a directive */
    }
    return false; /* if name is not a directive */
}

Bool is_reserved_word(char *name) {
    /* true if either of those, false otherwise */
    return is_instruction(name) || is_register(name) || is_directive(name);
}

Bool is_empty_line(char *str) {
    /* makes str to start from the first non-whitespace char */
    str = skip_whitespace(str);
    return *str == '\0' || *str == '\n';
}

Bool is_comment(char *str) {
    return *str == ';';
}