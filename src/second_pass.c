#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "bool.h"
#include "errors.h"
#include "hash_table.h"
#include "instructions.h"
#include "parser.h"
#include "second_pass.h"
#include "symbol_table.h"

Bool second_pass(char *filename, AssemblerState *state) {
    /* used to tell cleanup whether to free state variable or not */
    Bool success = false;
    /* a flag to tell whether the file has any errors or not */
    Bool has_errors = false;
    /* current line from fgets */
    char line[MAX_LINE];
    /* used to track current line num */
    int line_num = 0;
    /* a word from line */
    char token[MAX_LINE];
    /* pointer to text after token */
    char *token_ptr;
    /* stores a pointer to comment start (if any) */
    char *comment_start = NULL;
    /* symbol from assembler state */
    Symbol *symbol = NULL;
    /* read-only instruction info */
    const InstructionInfo *instruction_info = NULL;
    /* operands for instruction */
    char operand1[MAX_LINE], operand2[MAX_LINE];
    /* operand1 length to trim trailing whitepsaces of first operand for 2 operations instruction */
    int operand1_length;
    /* operands addressing modes */
    int operand1_addressing_mode, operand2_addressing_mode;
    /* operand comma pointer */
    char *comma_ptr = NULL;
    /* would store state->ic - IC_START */
    int code_index = 0;
    /* operand symbol name */
    char *symbol_name = NULL;
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

    /* allocate externals array */
    state->externals = malloc(MAX_MEMORY * sizeof(External));
    /* if failed, throw error and cleanup */
    if (!state->externals) {
        ERROR(ERR_MEMORY_ALLOC);
        goto cleanup;
    }

    /* set initial ec to 0 */
    state->ec = 0;

    /* while there are lines to read */
    while (fgets(line, MAX_LINE, input_file)) {
        /* increase line counter */
        line_num++;

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

        /* if token is a label, skip it (labels were already processed in first pass) */
        if (token[0] != '\0' && token[strlen(token) - 1] == ':')
            token_ptr = get_token(token_ptr, token);

        /* if token is .entry directive */
        if (strcmp(token, ".entry") == 0) {
            /* store symbol name in token */
            token_ptr = get_token(token_ptr, token);
            /* if no symbol provided, report error and skip to next line */
            if (is_empty(token)) {
                ERROR_LINE(line_num, ERR_ENTRY_INVALID_SYMBOL);
                has_errors = true;
                continue;
            }

            /* get symbol from symbols table */
            symbol = hash_table_lookup(state->symbols, token);
            /* if symbol not found, report error and skip to next line */
            if (!symbol) {
                ERROR_LINE(line_num, ERR_ENTRY_NOT_FOUND);
                has_errors = true;
                continue;
            }

            /* if symbol is external, report error and skip to next line */
            if (symbol->type == SYMBOL_EXTERNAL) {
                ERROR_LINE(line_num, ERR_ENTRY_IS_EXTERN);
                has_errors = true;
                continue;
            }

            /* mark symbol as entry */
            symbol->is_entry = true;
            /* if token is not a directive (not .data, .string, and .extern) */
        } else if (!is_directive(token)) {
            /* skip instruction word */
            code_index++;

            /* get instruction info */
            instruction_info = get_instruction_info(token);

            /* handle 1-operand instruction */
            if (instruction_info->num_operands == 1) {
                /* get first operand */
                token_ptr = get_token(token_ptr, operand1);

                /* set operand1_length to operand1 length */
                operand1_length = strlen(operand1);
                /* skip leading whitespace */
                token_ptr = skip_whitespace(token_ptr);

                /* get addressing mode for operand1 */
                operand1_addressing_mode = get_addressing_mode(operand1);
            }

            /* handle 2-operand instruction */
            if (instruction_info->num_operands == 2) {
                /* skip leading whitespace */
                token_ptr = skip_whitespace(token_ptr);

                /* get pointer to first comma */
                comma_ptr = strchr(token_ptr, ',');

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

                /* extract operand2 from token_ptr */
                token_ptr = get_token(token_ptr, operand2);

                /* skip leading whitespace */
                token_ptr = skip_whitespace(token_ptr);

                /* get addressing mode for operand1 */
                operand1_addressing_mode = get_addressing_mode(operand1);
                /* get addressing mode for operand2 */
                operand2_addressing_mode = get_addressing_mode(operand2);
            }

            /* update operand1 code word */
            if (instruction_info->num_operands >= 1) {
                if (operand1_addressing_mode == ADDR_DIRECT || operand1_addressing_mode == ADDR_RELATIVE) {
                    /* store operand1 symbol name */
                    symbol_name = operand1 + (operand1_addressing_mode == ADDR_RELATIVE ? 1 : 0);
                    /* get symbol from symbols table */
                    symbol = hash_table_lookup(state->symbols, symbol_name);
                    /* if symbol not found, report error and skip to next line */
                    if (!symbol) {
                        ERROR_LINE(line_num, ERR_SYMBOL_NOT_FOUND);
                        has_errors = true;
                        continue;
                    }
                }

                /* if operand1 addressing mode is relative and symbol is external, report error and skip to next line */
                if (operand1_addressing_mode == ADDR_RELATIVE && symbol->type == SYMBOL_EXTERNAL) {
                    ERROR_LINE(line_num, ERR_RELATIVE_EXTERNAL);
                    has_errors = true;
                    continue;
                }

                /* if operand1 addressing mode is direct */
                if (operand1_addressing_mode == ADDR_DIRECT) {
                    /* if symbol is external */
                    if (symbol->type == SYMBOL_EXTERNAL) {
                        /* update code word data */
                        state->code[code_index].value = 0;
                        state->code[code_index].are = ARE_E;

                        /* add external to externals array */
                        strcpy(state->externals[state->ec].name, symbol_name);
                        state->externals[state->ec].address = IC_START + code_index;
                        state->ec++;

                        /* in any other case */
                    } else {
                        state->code[code_index].value = symbol->address;
                        state->code[code_index].are = ARE_R;
                    }
                    /* if operand1 addressing mode is relative */
                } else if (operand1_addressing_mode == ADDR_RELATIVE) {
                    state->code[code_index].value = symbol->address - (IC_START + code_index);
                    state->code[code_index].are = ARE_A;
                }

                /* advance code_index to next word */
                code_index++;
            }

            /* update operand2 code word */
            if (instruction_info->num_operands == 2) {
                if (operand2_addressing_mode == ADDR_DIRECT || operand2_addressing_mode == ADDR_RELATIVE) {
                    /* store operand2 symbol name */
                    symbol_name = operand2 + (operand2_addressing_mode == ADDR_RELATIVE ? 1 : 0);
                    /* get symbol from symbols table */
                    symbol = hash_table_lookup(state->symbols, symbol_name);
                    /* if symbol not found, report error and skip to next line */
                    if (!symbol) {
                        ERROR_LINE(line_num, ERR_SYMBOL_NOT_FOUND);
                        has_errors = true;
                        continue;
                    }
                }

                /* if operand2 addressing mode is relative and symbol is external, report error and skip to next line */
                if (operand2_addressing_mode == ADDR_RELATIVE && symbol->type == SYMBOL_EXTERNAL) {
                    ERROR_LINE(line_num, ERR_RELATIVE_EXTERNAL);
                    has_errors = true;
                    continue;
                }

                /* if operand2 addressing mode is direct */
                if (operand2_addressing_mode == ADDR_DIRECT) {
                    /* if symbol is external */
                    if (symbol->type == SYMBOL_EXTERNAL) {
                        /* update code word data */
                        state->code[code_index].value = 0;
                        state->code[code_index].are = ARE_E;

                        /* add external to externals array */
                        strcpy(state->externals[state->ec].name, symbol_name);
                        state->externals[state->ec].address = IC_START + code_index;
                        state->ec++;

                        /* in any other case */
                    } else {
                        state->code[code_index].value = symbol->address;
                        state->code[code_index].are = ARE_R;
                    }
                    /* if operand2 addressing mode is relative */
                } else if (operand2_addressing_mode == ADDR_RELATIVE) {
                    state->code[code_index].value = symbol->address - (IC_START + code_index);
                    state->code[code_index].are = ARE_A;
                }

                /* advance code_index to next word */
                code_index++;
            }
        }
    }

    success = !has_errors;

cleanup:
    /* if input_file is open, close it */
    if (input_file)
        fclose(input_file);

    /* if success is false, free assembler state */
    if (!success)
        free_assembler_state(state);

    return success;
}