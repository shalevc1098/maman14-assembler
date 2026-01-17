#include <ctype.h>
#include <string.h>

#include "assembler.h"
#include "bool.h"
#include "instructions.h"
#include "parser.h"

/* reserved words - registers */
const char *REGISTERS[] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", NULL};

/* reserved words - directives */
const char *DIRECTIVES[] = {".data", ".string", ".entry", ".extern", "mcro", "mcroend", NULL};

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
    return str;   /* return pointer to remaining string */
}

Bool is_instruction(char *name) {
    /* index tracker */
    int i;
    for (i = 0; INSTRUCTION_TABLE[i].name != NULL; i++) {
        /* compare name with INSTRUCTION_TABLE[i].name */
        if (strcmp(name, INSTRUCTION_TABLE[i].name) == 0)
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

Bool is_empty(char *str) {
    /* makes str to start from the first non-whitespace char */
    str = skip_whitespace(str);
    return *str == '\0' || *str == '\n';
}

Bool is_number(char *str) {
    /* index tracker */
    int i;
    /* if str is empty, return false */
    if (*str == '\0')
        return false;
    /* loop all characters */
    for (i = 0; str[i] != '\0'; i++) {
        /* if character is not a valid number, return false */
        if (!isdigit(str[i])) {
            return false;
        }
    }
    /* all characters are valid numbers, return true */
    return true;
}