#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "bool.h"
#include "errors.h"
#include "first_pass.h"
#include "hash_table.h"
#include "helpers.h"
#include "instructions.h"
#include "parser.h"
#include "symbol_table.h"
#include "warns.h"

static void report_memory_overflow(int line_num, Bool *already_reported) {
    /* if already reported, return so we not spam */
    if (*already_reported)
        return;

    /* mark already_reported as true */
    *already_reported = true;
    /* report memory overflow error */
    ERROR_LINE(line_num, ERR_MEMORY_OVERFLOW);
}

typedef enum { SYMBOL_OK, SYMBOL_ERROR_CONTINUE, SYMBOL_ERROR_FATAL } SymbolResult;
static SymbolResult add_symbol(AssemblerState *state, char *label, int address, SymbolType type, Bool is_entry,
                               int line_num, Bool *has_errors) {
    /* the symbol to add to symbols table */
    Symbol *symbol = NULL;
    /* existing symbol with same name, NULL if not found */
    Symbol *existing = hash_table_lookup(state->symbols, label);

    /* if symbol with label already exists and either of them is not external, report error */
    if (existing && (type != SYMBOL_EXTERNAL || existing->type != SYMBOL_EXTERNAL)) {
        /* if either of them is external, report ERR_EXTERN_AND_LOCAL error */
        if (type == SYMBOL_EXTERNAL || existing->type == SYMBOL_EXTERNAL)
            ERROR_LINE(line_num, ERR_EXTERN_AND_LOCAL);
        /* in any other case, report ERR_LABEL_ALREADY_DEFINED error */
        else
            ERROR_LINE(line_num, ERR_LABEL_ALREADY_DEFINED);
        /* set has_errors to true */
        *has_errors = true;
        /* tells loop to skip to next line */
        return SYMBOL_ERROR_CONTINUE;
    }

    /* if symbols table doesn't have a symbol with key label, add a new one */
    if (!existing) {
        /* allocate new symbol */
        symbol = malloc(sizeof(Symbol));
        /* if allocation failed, throw error and cleanup */
        if (!symbol) {
            ERROR(ERR_MEMORY_ALLOC);
            /* tells loop to cleanup */
            return SYMBOL_ERROR_FATAL;
        }
        /* set symbol address to address */
        symbol->address = address;
        /* set symbol type to type */
        symbol->type = type;
        /* set symbol is_entry to is_entry */
        symbol->is_entry = is_entry;
        /* if insertion failed, throw error and cleanup */
        if (!hash_table_insert(state->symbols, label, symbol)) {
            ERROR(ERR_MEMORY_ALLOC);
            free(symbol);
            /* tells loop to cleanup */
            return SYMBOL_ERROR_FATAL;
        }
    }

    /* tells loop to continue as usual */
    return SYMBOL_OK;
}

static Bool is_label_too_long(char *label) {
    return strlen(label) >= MAX_LABEL;
}

static Bool is_label_starts_with_letter(char *label) {
    return isalpha(label[0]);
}

static Bool is_label_alphanumeric(char *label) {
    /* index tracker */
    int i;

    /* loop all label name characters to find if one is an invalid character */
    for (i = 0; label[i] != '\0'; i++) {
        /* if label name contains an invalid character */
        if (!isalnum(label[i]))
            return false;
    }

    return true;
}

static Bool is_valid_label(char *label, int line_num, Bool *has_errors) {
    /* if label is longer or equal to MAX_LABEL, report error and skip to next line */
    if (is_label_too_long(label)) {
        ERROR_LINE(line_num, ERR_LABEL_TOO_LONG);
        *has_errors = true;
        return false;
    }

    /* if label doesn't start with a letter, report error and skip to next line */
    if (!is_label_starts_with_letter(label)) {
        ERROR_LINE(line_num, ERR_LABEL_START_LETTER);
        *has_errors = true;
        return false;
    }

    /* if label name contains an invalid character, report error and skip to next line */
    if (!is_label_alphanumeric(label)) {
        ERROR_LINE(line_num, ERR_LABEL_INVALID_CHAR);
        *has_errors = true;
        return false;
    }

    /* if label is a reserved word, report error and skip to next line */
    if (is_reserved_word(label)) {
        ERROR_LINE(line_num, ERR_LABEL_RESERVED);
        *has_errors = true;
        return false;
    }

    return true;
}

static Bool is_valid_label_syntax(char *label) {
    return !is_label_too_long(label) && is_label_starts_with_letter(label) && is_label_alphanumeric(label) &&
           !is_reserved_word(label);
}

static Bool is_number_in_range(long num) {
    return num >= MIN_NUMBER && num <= MAX_NUMBER;
}

static int get_addressing_mode(char *operand) {
    if (*operand == '#')
        return ADDR_IMMEDIATE;
    else if (*operand == '%')
        return ADDR_RELATIVE;
    else if (*operand == 'r' && *(operand + 1) >= '0' && *(operand + 1) <= '7' && *(operand + 2) == '\0')
        return ADDR_REGISTER;
    else
        return ADDR_DIRECT;
}

Bool is_valid_addressing_mode(char *operand, int mode) {
    Bool has_optional_plus_minus = false;
    switch (mode) {
        case ADDR_IMMEDIATE:
            if (*(operand + 1) == '+' || *(operand + 1) == '-')
                has_optional_plus_minus = true;
            else if (!isdigit(*(operand + 1)))
                return false;
            return *operand == '#' && is_number(operand + (has_optional_plus_minus ? 2 : 1));
        case ADDR_DIRECT:
            return is_valid_label_syntax(operand);
        case ADDR_RELATIVE:
            return is_valid_label_syntax(operand + 1);
        case ADDR_REGISTER:
            return *operand == 'r' && *(operand + 1) >= '0' && *(operand + 1) <= '7' && *(operand + 2) == '\0';
        default:
            return false;
    }
}

static void update_symbol_data_address(char *key, void *data, void *context) {
    /* cast data to Symbol pointer */
    Symbol *symbol = (Symbol *)data;
    /* cast context to int */
    int final_ic = *(int *)context;
    /* silence unused parameter warning */
    (void)key;
    if (symbol->type == SYMBOL_DATA)
        symbol->address += final_ic;
}

AssemblerState *first_pass(char *filename) {
    /* assembler state */
    AssemblerState *state = NULL;
    /* used to tell cleanup whether to free state variable or not */
    Bool success = false;
    /* a flag to tell whether the file has any errors or not */
    Bool has_errors = false;
    /* indicates whether this line has a label or not */
    Bool has_label = false;
    /* a flag to tell whether memory overflow error was already reported or not */
    Bool memory_overflow_reported = false;
    /* a flag to tell whether .data directive number parse failed or not */
    /* current line from fgets */
    char line[MAX_LINE];
    /* used to track current line num */
    int line_num = 0;
    /* a word from line */
    char token[MAX_LINE];
    /* pointer to text after token */
    char *token_ptr;
    /* stores label name so i can add it to symbols table later */
    char label[MAX_LINE];
    /* stores a pointer to comment start (if any) */
    char *comment_start = NULL;
    /* would store the token_ptr pos before strtol */
    char *data_pos = NULL;
    /* would store the .data directive number after strtol */
    long data_num;
    /* add_symbol result */
    SymbolResult symbol_result;
    /* read-only instruction info */
    const InstructionInfo *instruction_info = NULL;
    /* operands for instruction */
    char operand1[MAX_LINE], operand2[MAX_LINE];
    /* operands length */
    int operand1_length, operand2_length;
    /* operands addressing modes */
    int operand1_addressing_mode, operand2_addressing_mode;
    /* operand comma pointer */
    char *comma_ptr = NULL;
    /* instruction length */
    int instruction_length;
    /* instruction src and dest modes */
    int src_mode, dest_mode;
    /* would store state->ic - IC_START */
    int code_index;
    /* input file path */
    char input_file_path[MAX_LINE];
    /* original file */
    FILE *input_file = NULL;

    /* write input path to input_file_path */
    sprintf(input_file_path, "%s.am", filename);

    /* open input_file as read-only */
    input_file = fopen(input_file_path, "r");
    /* if failed, throw error and cleanup */
    if (!input_file) {
        ERROR_FILE(ERR_CANNOT_OPEN_FILE, input_file_path);
        goto cleanup;
    }

    /* allocate new AssemblerState */
    state = malloc(sizeof(AssemblerState));
    /* if allocation failed, throw error and cleanup */
    if (!state) {
        ERROR(ERR_MEMORY_ALLOC);
        goto cleanup;
    }

    /* create symbols table */
    state->symbols = hash_table_create();
    /* if failed, throw error and cleanup */
    if (!state->symbols) {
        ERROR(ERR_MEMORY_ALLOC);
        goto cleanup;
    }

    /* allocate code words array */
    state->code = malloc(MAX_MEMORY * sizeof(Word));
    /* if failed, throw error and cleanup */
    if (!state->code) {
        ERROR(ERR_MEMORY_ALLOC);
        goto cleanup;
    }

    /* allocate data words array */
    state->data = malloc(MAX_MEMORY * sizeof(Word));
    /* if failed, throw error and cleanup */
    if (!state->data) {
        ERROR(ERR_MEMORY_ALLOC);
        goto cleanup;
    }

    /* set initial ic to IC_START */
    state->ic = IC_START;
    /* set initial dc to 0 */
    state->dc = 0;

    /* while there are lines to read */
    while (fgets(line, MAX_LINE, input_file)) {
        /* increase line counter */
        line_num++;
        /* reset has_label flag */
        has_label = false;

        /* if line is longer than MAX_LINE, report error and skip to next line */
        if (strchr(line, '\n') == NULL && !feof(input_file)) {
            ERROR_LINE(line_num, ERR_LINE_TOO_LONG);
            has_errors = true;
            discard_rest_of_line(input_file);
            continue;
        }

        /* finds first comment char in line */
        comment_start = strchr(line, COMMENT_CHAR);
        /* if found, strips it and what's after */
        if (comment_start)
            *comment_start = '\0';

        /* if line is empty or comment, skip to next line */
        if (is_empty(line))
            continue;

        /* gets first word from line */
        token_ptr = get_token(line, token);

        /* if token is a label */
        if (token[0] != '\0' && token[strlen(token) - 1] == ':') {
            /* truncate ':' */
            token[strlen(token) - 1] = '\0';

            /* if label is invalid, continue (errors reported inside and has_errors becomes true inside too) */
            if (!is_valid_label(token, line_num, &has_errors))
                continue;

            /* if label is already defined, report error and skip to next line */
            if (hash_table_contains_key(state->symbols, token)) {
                ERROR_LINE(line_num, ERR_LABEL_ALREADY_DEFINED);
                has_errors = true;
                continue;
            }

            /* save label name for later */
            strcpy(label, token);
            /* mark has_label as true */
            has_label = true;

            /* restore ':' */
            token[strlen(token)] = ':';

            /* get next word after label */
            token_ptr = get_token(token_ptr, token);
        }

        /* if token starts with '.', it is probably a directive */
        if (token[0] == '.') {
            /* if token is an unknown directive, report error and skip to next line */
            if (!is_directive(token)) {
                ERROR_LINE(line_num, ERR_UNKNOWN_DIRECTIVE);
                has_errors = true;
                continue;
            }

            if (strcmp(token, ".data") == 0) {
                /* if has_label, add it to symbols table */
                if (has_label) {
                    /* store add_symbol result */
                    symbol_result = add_symbol(state, label, state->dc, SYMBOL_DATA, false, line_num, &has_errors);
                    /* if insertion failed, either go to cleanup or skip to next line (error already reported) */
                    if (symbol_result == SYMBOL_ERROR_FATAL)
                        goto cleanup;
                    else if (symbol_result == SYMBOL_ERROR_CONTINUE)
                        continue;
                }

                /* if token_ptr is empty, report error and skip to next line */
                if (is_empty(token_ptr)) {
                    ERROR_LINE(line_num, ERR_DATA_MISSING_NUMBERS);
                    has_errors = true;
                    continue;
                }

                /* skip leading whitespace */
                token_ptr = skip_whitespace(token_ptr);
                /* if token_ptr starts with a comma, report error and skip to next line  */
                if (token_ptr[0] == ',') {
                    ERROR_LINE(line_num, ERR_DATA_ILLEGAL_COMMA);
                    has_errors = true;
                    continue;
                }

                /* loop token_ptr until it is empty */
                while (!is_empty(token_ptr)) {
                    /* backup token_ptr to data_pos */
                    data_pos = token_ptr;
                    /* parse with strtol base 10 */
                    data_num = strtol(token_ptr, &token_ptr, 10);

                    /* if token_ptr hasn't moved, parse failed, report error and skip to next line */
                    if (token_ptr == data_pos) {
                        ERROR_LINE(line_num, ERR_DATA_INVALID_NUMBER);
                        has_errors = true;
                        goto next_line;
                    }

                    /* if number is out of range, report error and skip to next line */
                    if (!is_number_in_range(data_num)) {
                        ERROR_LINE(line_num, ERR_NUMBER_OUT_OF_RANGE);
                        has_errors = true;
                        goto next_line;
                    }

                    /* if memory overflow, report error and skip to next line */
                    if (!has_memory(state->ic, state->dc, 1)) {
                        report_memory_overflow(line_num, &memory_overflow_reported);
                        has_errors = true;
                        goto next_line;
                    }

                    /* store num in data state array */
                    state->data[state->dc].value = data_num;
                    /* set type to ARE_A (absolute) */
                    state->data[state->dc].are = ARE_A;
                    /* advance dc */
                    state->dc++;

                    /* skip leading whitespace */
                    token_ptr = skip_whitespace(token_ptr);
                    /* if comma found, skip it and check for trailing comma */
                    if (*token_ptr == ',') {
                        /* advance token_ptr to next character */
                        token_ptr++;
                        /* skip leading whitespace */
                        token_ptr = skip_whitespace(token_ptr);

                        /* if another comma was found, report error and skip to next line */
                        if (*token_ptr == ',') {
                            ERROR_LINE(line_num, ERR_DATA_EXTRA_COMMA);
                            has_errors = true;
                            goto next_line;
                        }

                        /* if token_ptr is empty, report error and skip to next line */
                        if (is_empty(token_ptr)) {
                            ERROR_LINE(line_num, ERR_DATA_ILLEGAL_COMMA);
                            has_errors = true;
                            goto next_line;
                        }
                        /* if token_ptr is not empty, report error and skip to next line */
                    } else if (!is_empty(token_ptr)) {
                        ERROR_LINE(line_num, ERR_DATA_EXPECTED_COMMA);
                        has_errors = true;
                        goto next_line;
                    }
                }
            } else if (strcmp(token, ".string") == 0) {
                /* if has_label, add it to symbols table */
                if (has_label) {
                    /* store add_symbol result */
                    symbol_result = add_symbol(state, label, state->dc, SYMBOL_DATA, false, line_num, &has_errors);
                    /* if insertion failed, either go to cleanup or skip to next line (error already reported) */
                    if (symbol_result == SYMBOL_ERROR_FATAL)
                        goto cleanup;
                    else if (symbol_result == SYMBOL_ERROR_CONTINUE)
                        continue;
                }

                /* skip leading whitespace */
                token_ptr = skip_whitespace(token_ptr);

                /* if token_ptr is empty, report error and skip to next line */
                if (is_empty(token_ptr)) {
                    ERROR_LINE(line_num, ERR_STRING_MISSING);
                    has_errors = true;
                    continue;
                }

                /* if first char of token_ptr is not ", report error and skip to next line */
                if (*token_ptr != '"') {
                    ERROR_LINE(line_num, ERR_STRING_INVALID);
                    has_errors = true;
                    continue;
                }

                /* advance token_ptr to next character (skips leading ") */
                token_ptr++;

                while (*token_ptr != '"' && !is_empty(token_ptr)) {
                    /* if memory overflow, report error and skip to next line */
                    if (!has_memory(state->ic, state->dc, 1)) {
                        report_memory_overflow(line_num, &memory_overflow_reported);
                        has_errors = true;
                        goto next_line;
                    }

                    /* store num in data state array */
                    state->data[state->dc].value = *token_ptr;
                    /* set type to ARE_A (absolute) */
                    state->data[state->dc].are = ARE_A;
                    /* advance dc */
                    state->dc++;
                    /* advance token_ptr to next character */
                    token_ptr++;
                }

                /* if last char of token_ptr is not ", report error and skip to next line */
                if (*token_ptr != '"') {
                    ERROR_LINE(line_num, ERR_STRING_INVALID);
                    has_errors = true;
                    continue;
                }

                /* if memory overflow, report error and skip to next line */
                if (!has_memory(state->ic, state->dc, 1)) {
                    report_memory_overflow(line_num, &memory_overflow_reported);
                    has_errors = true;
                    goto next_line;
                }

                /* add NULL terminator */
                state->data[state->dc].value = '\0';
                /* type ARE_A (absolute) */
                state->data[state->dc].are = ARE_A;
                /* advance dc */
                state->dc++;
            } else if (strcmp(token, ".entry") == 0) {
                /* handled in second pass */
                continue;
            } else if (strcmp(token, ".extern") == 0) {
                /* warn if line has label, then continue as usual */
                if (has_label)
                    WARN_LINE(line_num, WARN_LABEL_BEFORE_EXTERN);

                /* get next word */
                token_ptr = get_token(token_ptr, token);

                /* if token is empty, report error and skip to next line */
                if (is_empty(token)) {
                    ERROR_LINE(line_num, ERR_EXTERN_INVALID_SYMBOL);
                    has_errors = true;
                    continue;
                }

                /* if label is invalid, continue (errors reported inside and has_errors becomes true inside too) */
                if (!is_valid_label(token, line_num, &has_errors))
                    continue;

                /* skip leading whitespace */
                token_ptr = skip_whitespace(token_ptr);
                /* if token_ptr is not empty, report error and skip to next line */
                if (!is_empty(token_ptr)) {
                    ERROR_LINE(line_num, ERR_EXTRA_TEXT);
                    has_errors = true;
                    continue;
                }

                /* store add_symbol result */
                symbol_result = add_symbol(state, token, 0, SYMBOL_EXTERNAL, false, line_num, &has_errors);
                /* if insertion failed, go to cleanup (error already reported) */
                if (symbol_result == SYMBOL_ERROR_FATAL)
                    goto cleanup;
            }
            /* if token is not empty, it is probably an instruction */
        } else if (!is_empty(token)) {
            /* if token is an unknown instruction, report error and skip to next line */
            if (!is_instruction(token)) {
                ERROR_LINE(line_num, ERR_UNKNOWN_INSTRUCTION);
                has_errors = true;
                continue;
            }

            /* if has_label, add it to symbols table */
            if (has_label) {
                /* store add_symbol result */
                symbol_result = add_symbol(state, label, state->ic, SYMBOL_CODE, false, line_num, &has_errors);
                /* if insertion failed, either go to cleanup or skip to next line (error already reported) */
                if (symbol_result == SYMBOL_ERROR_FATAL)
                    goto cleanup;
                else if (symbol_result == SYMBOL_ERROR_CONTINUE)
                    continue;
            }

            /* get instruction info of current instruction */
            instruction_info = get_instruction_info(token);

            /* handle 1-operand instruction */
            if (instruction_info->num_operands == 1) {
                /* get first operand */
                token_ptr = get_token(token_ptr, operand1);

                /* if operand1 is empty, report error and skip to next line */
                if (is_empty(operand1)) {
                    ERROR_LINE(line_num, ERR_MISSING_OPERAND);
                    has_errors = true;
                    continue;
                }

                /* set operand1_length to operand1 length */
                operand1_length = strlen(operand1);
                /* skip leading whitespace */
                token_ptr = skip_whitespace(token_ptr);
                /* if operand1 starts or ends with a comma found, report error and skip to next line */
                if (*operand1 == ',' || operand1[operand1_length - 1] == ',' || *token_ptr == ',') {
                    ERROR_LINE(line_num, ERR_OPERAND_ILLEGAL_COMMA);
                    has_errors = true;
                    continue;
                }
            }

            /* handle 2-operand instruction */
            if (instruction_info->num_operands == 2) {
                /* skip leading whitespace */
                token_ptr = skip_whitespace(token_ptr);

                /* get pointer to first comma */
                comma_ptr = strchr(token_ptr, ',');

                /* if no comma found, report error and skip to next line */
                if (!comma_ptr) {
                    ERROR_LINE(line_num, ERR_OPERAND_EXPECTED_COMMA);
                    has_errors = true;
                    continue;
                }

                /* if comma_ptr is at the start of token_ptr, report error and skip to next line */
                if (comma_ptr == token_ptr) {
                    ERROR_LINE(line_num, ERR_OPERAND_ILLEGAL_COMMA);
                    has_errors = true;
                    continue;
                }

                /* copy all characters before comma to operand1 */
                strncpy(operand1, token_ptr, comma_ptr - token_ptr);
                /* add NULL terminator at the end of operand1 */
                operand1[comma_ptr - token_ptr] = '\0';

                /* skip trailing whitespace from operand1 */
                operand1_length = strlen(operand1);
                while (operand1_length > 0 &&
                       (operand1[operand1_length - 1] == ' ' || operand1[operand1_length - 1] == '\t')) {
                    operand1[operand1_length - 1] = '\0';
                    operand1_length--;
                }

                /* skip comma */
                token_ptr = comma_ptr + 1;

                /* skip leading whitespace */
                token_ptr = skip_whitespace(token_ptr);
                /* if token_ptr starts with a comma, report error and skip to next line */
                if (*token_ptr == ',') {
                    ERROR_LINE(line_num, ERR_OPERAND_EXTRA_COMMA);
                    has_errors = true;
                    continue;
                }

                /* extract operand2 from token_ptr */
                token_ptr = get_token(token_ptr, operand2);
                /* if either operand1 or operand2 are empty, report error and skip to next line */
                if (is_empty(operand1) || is_empty(operand2)) {
                    ERROR_LINE(line_num, ERR_MISSING_OPERAND);
                    has_errors = true;
                    continue;
                }

                /* set operand2_length to operand2 length */
                operand2_length = strlen(operand2);
                /* skip leading whitespace */
                token_ptr = skip_whitespace(token_ptr);
                /* if operand2 ends with a comma found, report error and skip to next line */
                if (operand2[operand2_length - 1] == ',' || *token_ptr == ',') {
                    ERROR_LINE(line_num, ERR_OPERAND_ILLEGAL_COMMA);
                    has_errors = true;
                    continue;
                }
            }

            /* if token_ptr is not empty, report error and skip to next line */
            if (!is_empty(token_ptr)) {
                ERROR_LINE(line_num, ERR_TOO_MANY_OPERANDS);
                has_errors = true;
                continue;
            }

            if (instruction_info->num_operands == 0) {
                /* set both src and dest modes to 0 */
                src_mode = 0;
                dest_mode = 0;
            } else if (instruction_info->num_operands == 1) {
                /* get addressing mode for operand1 */
                operand1_addressing_mode = get_addressing_mode(operand1);

                /* if operand1 is invalid, report error and skip to next line */
                if (!is_valid_addressing_mode(operand1, operand1_addressing_mode)) {
                    ERROR_LINE(line_num, ERR_INVALID_OPERAND);
                    has_errors = true;
                    continue;
                }

                /* if number is out of range, report error and skip to next line */
                if (operand1_addressing_mode == ADDR_IMMEDIATE && !is_number_in_range(strtol(operand1 + 1, NULL, 10))) {
                    ERROR_LINE(line_num, ERR_NUMBER_OUT_OF_RANGE);
                    has_errors = true;
                    goto next_line;
                }

                /* if invalid dest mode, report error and skip to next line */
                if (!is_valid_dest_mode(instruction_info, operand1_addressing_mode)) {
                    ERROR_LINE(line_num, ERR_INVALID_DEST_MODE);
                    has_errors = true;
                    continue;
                }

                /* assign src and dest modes */
                src_mode = 0;
                dest_mode = operand1_addressing_mode;
            } else if (instruction_info->num_operands == 2) {
                /* get addressing mode for operand1 */
                operand1_addressing_mode = get_addressing_mode(operand1);

                /* if operand1 is invalid, report error and skip to next line */
                if (!is_valid_addressing_mode(operand1, operand1_addressing_mode)) {
                    ERROR_LINE(line_num, ERR_INVALID_OPERAND);
                    has_errors = true;
                    continue;
                }

                /* if number is out of range, report error and skip to next line */
                if (operand1_addressing_mode == ADDR_IMMEDIATE && !is_number_in_range(strtol(operand1 + 1, NULL, 10))) {
                    ERROR_LINE(line_num, ERR_NUMBER_OUT_OF_RANGE);
                    has_errors = true;
                    continue;
                }

                /* if invalid src mode, report error and skip to next line */
                if (!is_valid_src_mode(instruction_info, operand1_addressing_mode)) {
                    ERROR_LINE(line_num, ERR_INVALID_SOURCE_MODE);
                    has_errors = true;
                    continue;
                }

                /* get addressing mode for operand2 */
                operand2_addressing_mode = get_addressing_mode(operand2);

                /* if operand2 is invalid, report error and skip to next line */
                if (!is_valid_addressing_mode(operand2, operand2_addressing_mode)) {
                    ERROR_LINE(line_num, ERR_INVALID_OPERAND);
                    has_errors = true;
                    continue;
                }

                /* if number is out of range, report error and skip to next line */
                if (operand2_addressing_mode == ADDR_IMMEDIATE && !is_number_in_range(strtol(operand2 + 1, NULL, 10))) {
                    ERROR_LINE(line_num, ERR_NUMBER_OUT_OF_RANGE);
                    has_errors = true;
                    continue;
                }

                /* if invalid dest mode, report error and skip to next line */
                if (!is_valid_dest_mode(instruction_info, operand2_addressing_mode)) {
                    ERROR_LINE(line_num, ERR_INVALID_DEST_MODE);
                    has_errors = true;
                    continue;
                }

                /* assign src and dest modes */
                src_mode = operand1_addressing_mode;
                dest_mode = operand2_addressing_mode;
            }

            /* calculate instruction length */
            instruction_length = 1 + instruction_info->num_operands;

            /* if memory overflow, report error and skip to next line */
            if (!has_memory(state->ic, state->dc, instruction_length)) {
                report_memory_overflow(line_num, &memory_overflow_reported);
                has_errors = true;
                goto next_line;
            }

            /* calculate index */
            code_index = state->ic - IC_START;

            /* encode first word */
            state->code[code_index].value =
                (instruction_info->opcode << 8) | (instruction_info->funct << 4) | (src_mode << 2) | dest_mode;
            state->code[code_index].are = ARE_A;

            /* encode operand1 */
            if (instruction_info->num_operands >= 1) {
                /* advance code_index to next word */
                code_index++;
                /* if addressing mode is immediate, store the number value (skip '#') */
                if (operand1_addressing_mode == ADDR_IMMEDIATE) {
                    state->code[code_index].value = strtol(operand1 + 1, NULL, 10);
                    state->code[code_index].are = ARE_A;
                    /* if addressing mode is register, store bitmask (bit N set for rN) */
                } else if (operand1_addressing_mode == ADDR_REGISTER) {
                    state->code[code_index].value = 1 << (operand1[1] - '0');
                    state->code[code_index].are = ARE_A;
                    /* if addressing mode is direct or relative, placeholder for second pass */
                } else {
                    state->code[code_index].value = 0;
                    state->code[code_index].are = ARE_A;
                }
            }

            /* encode operand2 */
            if (instruction_info->num_operands == 2) {
                /* advance code_index to next word */
                code_index++;
                /* if addressing mode is immediate, store the number value (skip '#') */
                if (operand2_addressing_mode == ADDR_IMMEDIATE) {
                    state->code[code_index].value = strtol(operand2 + 1, NULL, 10);
                    state->code[code_index].are = ARE_A;
                    /* if addressing mode is register, store bitmask (bit N set for rN) */
                } else if (operand2_addressing_mode == ADDR_REGISTER) {
                    state->code[code_index].value = 1 << (operand2[1] - '0');
                    state->code[code_index].are = ARE_A;
                    /* if addressing mode is direct or relative, placeholder for second pass */
                } else {
                    state->code[code_index].value = 0;
                    state->code[code_index].are = ARE_A;
                }
            }

            state->ic += instruction_length;
        }

next_line:;
    }

    hash_table_foreach(state->symbols, update_symbol_data_address, &state->ic);

    success = !has_errors;

cleanup:
    /* if input_file is open, close it */
    if (input_file)
        fclose(input_file);

    /* if success is false, free assembler state and set state variable to NULL */
    if (!success)
        state = free_assembler_state(state);

    return state;
}