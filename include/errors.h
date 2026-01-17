/* include guard to define only once */
#ifndef ERRORS_H
#define ERRORS_H

/* error printing macros */
#define ERROR(msg) fprintf(stderr, "Error: %s\n", msg)
#define ERROR_FILE(msg, path) fprintf(stderr, "Error: %s '%s'\n", msg, path)
#define ERROR_LINE(line, msg) fprintf(stderr, "Error on line %d: %s\n", line, msg)

/* directive errors */
#define ERR_DATA_INVALID_NUMBER "invalid number in .data directive"
#define ERR_DATA_EXPECTED_COMMA "expected comma between numbers"
#define ERR_DATA_ILLEGAL_COMMA "illegal comma in .data directive"
#define ERR_DATA_EXTRA_COMMA "extra comma in .data directive"
#define ERR_DATA_MISSING_NUMBERS "missing numbers in .data directive"
#define ERR_STRING_MISSING "missing string in .string directive"
#define ERR_STRING_INVALID "invalid string format"
#define ERR_EXTERN_INVALID_SYMBOL "invalid symbol name in .extern"
#define ERR_ENTRY_INVALID_SYMBOL "invalid symbol name in .entry"
#define ERR_EXTERN_AND_LOCAL "symbol declared extern and defined locally"
#define ERR_UNKNOWN_DIRECTIVE "unknown directive"

/* file errors */
#define ERR_CANNOT_OPEN_FILE "cannot open file"
#define ERR_CANNOT_CREATE_FILE "cannot create file"
#define ERR_CANNOT_WRITE_FILE "cannot write to file"

/* instruction errors */
#define ERR_INVALID_SOURCE_MODE "invalid addressing mode for source operand"
#define ERR_INVALID_DEST_MODE "invalid addressing mode for destination operand"
#define ERR_INVALID_REGISTER "invalid register name"
#define ERR_UNKNOWN_INSTRUCTION "unknown instruction"

/* label errors */
#define ERR_LABEL_START_LETTER "label must start with a letter"
#define ERR_LABEL_TOO_LONG "label exceeds 31 characters"
#define ERR_LABEL_INVALID_CHAR "label contains invalid character"
#define ERR_LABEL_ALREADY_DEFINED "label already defined"
#define ERR_LABEL_RESERVED "label is a reserved word"

/* line errors */
#define ERR_LINE_TOO_LONG "line exceeds 80 characters"

/* macro errors */
#define ERR_MACRO_NO_NAME "no macro name"
#define ERR_MACRO_RESERVED "macro name cannot be a reserved word"
#define ERR_MACRO_EXTRA_TEXT "extra text after macro definition"
#define ERR_MACRO_ALREADY_DEFINED "macro already defined"
#define ERR_MACRO_START_WITHOUT_END "mcro without mcroend"
#define ERR_MACRO_END_WITHOUT_START "mcroend without mcro"
#define ERR_LABEL_IS_MACRO_NAME "label name conflicts with macro name"
#define ERR_MACRO_NAME_IS_LABEL "macro name conflicts with label name"
#define ERR_LABEL_BEFORE_MACRO "label cannot appear before macro definition"

/* memory errors */
#define ERR_MEMORY_ALLOC "memory allocation failed"
#define ERR_MEMORY_OVERFLOW "memory overflow - program too large"

/* number errors */
#define ERR_NUMBER_OUT_OF_RANGE "number out of range"

/* operand errors */
#define ERR_MISSING_OPERAND "missing operand"
#define ERR_TOO_MANY_OPERANDS "too many operands"
#define ERR_OPERAND_ILLEGAL_COMMA "illegal comma in operand"
#define ERR_OPERAND_EXPECTED_COMMA "expected comma between operands"
#define ERR_OPERAND_EXTRA_COMMA "extra comma in operands"
#define ERR_EXTRA_TEXT "extra text after statement"
#define ERR_INVALID_OPERAND "invalid operand syntax"

/* symbol errors */
#define ERR_SYMBOL_NOT_FOUND "undefined symbol"
#define ERR_ENTRY_NOT_FOUND "symbol in .entry not defined"

#endif