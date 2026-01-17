/* include guard to define only once */
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "bool.h"

/* an enum to indicate symbol type */
typedef enum { SYMBOL_CODE, SYMBOL_DATA, SYMBOL_EXTERNAL } SymbolType;

/* a struct with data about a symbol */
typedef struct {
    int address;
    SymbolType type;
    Bool is_entry;
} Symbol;

#endif