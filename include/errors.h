/* include guard to define only once */
#ifndef ERRORS_H
#define ERRORS_H

/* error printing macros */
#define ERROR(msg) fprintf(stderr, "Error: %s\n", msg)
#define ERROR_LINE(line, msg) fprintf(stderr, "Error on line %d: %s\n", line, msg)

/* file errors */
#define ERR_CANNOT_OPEN_FILE "cannot open file"
#define ERR_CANNOT_CREATE_FILE "cannot create file"
#define ERR_CANNOT_WRITE_FILE "cannot write to file"

/* macro errors */
#define ERR_MACRO_NO_NAME "no macro name"
#define ERR_MACRO_RESERVED "macro name cannot be a reserved word"
#define ERR_MACRO_EXTRA_TEXT "extra text after macro definition"
#define ERR_MACRO_ALREADY_DEFINED "macro already defined"
#define ERR_MACRO_START_WITHOUT_END "mcro without mcroend"
#define ERR_MACRO_END_WITHOUT_START "mcroend without mcro"

/* memory errors */
#define ERR_MEMORY_ALLOC "memory allocation failed"

/* syntax errors */
#define ERR_LINE_TOO_LONG "line exceeds 80 characters"

#endif