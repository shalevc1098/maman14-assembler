#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "globals.h"
#include "hash_table.h"
#include "parser.h"
#include "pre_assembler.h"

/* frees macro lines array */
static void free_macro_lines(Macro *m) {
    /* index tracker */
    int i;
    /* for each macro line */
    for (i = 0; i < m->line_count; i++)
        /* free line */
        free(m->lines[i]);
    /* at last free lines array itself */
    free(m->lines);
}

static void free_macro(void *data) {
    /* cast data to Macro pointer */
    Macro *m = (Macro *)data;
    /* free macro lines array */
    free_macro_lines(m);
    /* free macro itself */
    free(m);
}

Bool pre_assemble(char *filename) {
    /* used to tell cleanup whether to remove expanded_file or not */
    Bool success = false;
    /* current line from fgets */
    char line[MAX_LINE];
    /* a word from line */
    char token[MAX_LINE];
    /* pointer to text after token */
    char *token_ptr;
    /* expanded macro from macros table */
    Macro *macro_to_expand = NULL;
    /* a flag to determine if in macro or outside */
    Bool in_macro = false;
    /* would be initialized for every macro and inserted to macros table */
    Macro *macro = NULL;
    /* temp variable to store macro lines before realloc (used for cleanup) */
    char **prev_macro_lines;
    /* macros table */
    HashTable *macros = NULL;
    /* labels array */
    char **labels = NULL;
    /* labels array count */
    int label_count = 0;
    /* temp variable to store labels array before realloc (used for cleanup) */
    char **prev_labels = NULL;
    /* parsed macro name */
    char macro_name[MAX_LINE];
    /* input file path */
    char input_file_path[MAX_LINE];
    /* file after macro expansion path */
    char expanded_file_path[MAX_LINE];
    /* index tracker */
    int i;
    /* used to track current line num */
    int line_num = 0;
    /* used to track macro line num */
    int macro_line_num = 0;
    /* original file */
    FILE *input_file = NULL;
    /* expanded file */
    FILE *expanded_file = NULL;
    /* write input path to input_file_path */
    sprintf(input_file_path, "%s.as", filename);
    /* write output path to expanded_file_path */
    sprintf(expanded_file_path, "%s.am", filename);
    /* open input_file as read-only */
    input_file = fopen(input_file_path, "r");
    /* if failed, throw error and cleanup */
    if (!input_file) {
        ERROR(ERR_CANNOT_OPEN_FILE);
        goto cleanup;
    }
    /* open input_file as write-only */
    expanded_file = fopen(expanded_file_path, "w");
    /* if failed, throw error and cleanup */
    if (!expanded_file) {
        ERROR(ERR_CANNOT_CREATE_FILE);
        goto cleanup;
    }
    /* create macros table */
    macros = hash_table_create();
    /* if failed, throw error and cleanup */
    if (!macros) {
        ERROR(ERR_MEMORY_ALLOC);
        goto cleanup;
    }
    /* while there are lines to read */
    while (fgets(line, MAX_LINE, input_file)) {
        /* increase line counter */
        line_num++;
        /* gets first word from line */
        token_ptr = get_token(line, token);

        /* checks if label conflicts with macro name */
        if (token[0] != '\0' && token[strlen(token) - 1] == ':') {
            /* truncate ':' */
            token[strlen(token) - 1] = '\0';
            /* if a macro with name of label was already parsed, throw error and cleanup */
            if (hash_table_lookup(macros, token)) {
                ERROR_LINE(line_num, ERR_LABEL_IS_MACRO_NAME);
                goto cleanup;
            }
            /* backup labels array before realloc */
            prev_labels = labels;
            /* realloc labels array by +1 and assign it to labels */
            labels = realloc(labels, (label_count + 1) * sizeof(char *));
            /* if realloc failed, throw error, assign prev_labels to labels and cleanup */
            if (!labels) {
                ERROR(ERR_MEMORY_ALLOC);
                labels = prev_labels;
                goto cleanup;
            }
            /* allocate slot for label */
            labels[label_count] = malloc(strlen(token) + 1);
            /* if allocation failed, throw error and cleanup */
            if (!labels[label_count]) {
                ERROR(ERR_MEMORY_ALLOC);
                goto cleanup;
            }
            /* copy label to labels[label_count] */
            strcpy(labels[label_count], token);
            /* increase label count by +1 */
            label_count++;
            /* restore ':' */
            token[strlen(token)] = ':';
        }

        /* if line is a macro */
        if (strcmp(token, "mcro") == 0) {
            /* gets macro name */
            token_ptr = get_token(token_ptr, macro_name);
            /* if macro name is empty, throw error and cleanup */
            if (macro_name[0] == '\0') {
                ERROR_LINE(line_num, ERR_MACRO_NO_NAME);
                goto cleanup;
            }
            /* if macro name is a reserved word, throw error and cleanup */
            if (is_reserved_word(macro_name)) {
                ERROR_LINE(line_num, ERR_MACRO_RESERVED);
                goto cleanup;
            }
            /* make sure macro name doesn't have any word after it */
            token_ptr = get_token(token_ptr, token);
            /* if macro name has at least one word after it, throw error and cleanup */
            if (token[0] != '\0') {
                ERROR_LINE(line_num, ERR_MACRO_EXTRA_TEXT);
                goto cleanup;
            }
            /* if macro already exists in macros table (duplicate), throw error and cleanup */
            if (hash_table_contains_key(macros, macro_name)) {
                ERROR_LINE(line_num, ERR_MACRO_ALREADY_DEFINED);
                goto cleanup;
            }
            /* if there is at least one label, check if a label with macro_name was already defined */
            if (labels) {
                /* loop all labels */
                for (i = 0; i < label_count; i++) {
                    /* if macro_name found in labels array, throw error and cleanup */
                    if (strcmp(labels[i], macro_name) == 0) {
                        ERROR_LINE(line_num, ERR_MACRO_NAME_IS_LABEL);
                        goto cleanup;
                    }
                }
            }
            /* allocate new Macro */
            macro = malloc(sizeof(Macro));
            /* if allocation failed, throw error and cleanup */
            if (!macro) {
                ERROR(ERR_MEMORY_ALLOC);
                goto cleanup;
            }
            /* set macro lines to NULL */
            macro->lines = NULL;
            /* set macro line count to 0 */
            macro->line_count = 0;
            /* if insert macro to macros table failed, throw error and cleanup */
            if (!hash_table_insert(macros, macro_name, macro)) {
                ERROR(ERR_MEMORY_ALLOC);
                free_macro(macro);
                goto cleanup;
            }
            /* enable in_macro flag */
            in_macro = true;
            /* update macro_line_num with current line num */
            macro_line_num = line_num;
            /* if line is macro end */
        } else if (strcmp(token, "mcroend") == 0) {
            /* if reached here with in_macro set to false, no mcro, thus throw error and cleanup */
            if (!in_macro) {
                ERROR_LINE(line_num, ERR_MACRO_END_WITHOUT_START);
                goto cleanup;
            }
            /* make sure macro end doesn't have any word after it */
            token_ptr = get_token(token_ptr, token);
            /* if macro end has at least one word after it, throw error and cleanup */
            if (token[0] != '\0') {
                ERROR_LINE(line_num, ERR_MACRO_EXTRA_TEXT);
                goto cleanup;
            }
            /* disable in_macro flag */
            in_macro = false;
            /* if in_macro flag enabled */
        } else if (in_macro) {
            /* backup macro lines before realloc */
            prev_macro_lines = macro->lines;
            /* realloc macro lines array by +1 and assign it to macro->lines */
            macro->lines = realloc(macro->lines, (macro->line_count + 1) * sizeof(char *));
            /* if realloc failed, throw error, assign prev_macro_lines to macro->lines and cleanup */
            if (!macro->lines) {
                ERROR(ERR_MEMORY_ALLOC);
                macro->lines = prev_macro_lines;
                goto cleanup;
            }
            /* allocate slot for macro line */
            macro->lines[macro->line_count] = malloc(strlen(line) + 1);
            /* if allocation failed, throw error and cleanup */
            if (!macro->lines[macro->line_count]) {
                ERROR(ERR_MEMORY_ALLOC);
                goto cleanup;
            }
            /* copy line to macro->lines[macro->line_count] */
            strcpy(macro->lines[macro->line_count], line);
            /* increase macro line count by +1 */
            macro->line_count++;
            /* if in_macro flag disabled */
        } else {
            /* check if token is a macro name */
            macro_to_expand = hash_table_lookup(macros, token);
            /* if macro not found */
            if (!macro_to_expand) {
                /* write line to expanded_file as is, if failed, throw error and cleanup */
                if (fputs(line, expanded_file) == EOF) {
                    ERROR(ERR_CANNOT_WRITE_FILE);
                    goto cleanup;
                }
                /* if macro found */
            } else {
                /* go over each macro line */
                for (i = 0; i < macro_to_expand->line_count; i++) {
                    /* write macro line to expanded_file, if failed, throw error and cleanup */
                    if (fputs(macro_to_expand->lines[i], expanded_file) == EOF) {
                        ERROR(ERR_CANNOT_WRITE_FILE);
                        goto cleanup;
                    }
                }
            }
        }
    }

    /* if still in macro, no mcroend, thus throw error and cleanup */
    if (in_macro) {
        ERROR_LINE(macro_line_num, ERR_MACRO_START_WITHOUT_END);
        goto cleanup;
    }

    /* mark operation as success so cleanup wouldn't remove expanded_file */
    success = true;

cleanup:
    /* if input_file is open, close it */
    if (input_file)
        fclose(input_file);
    /* if expanded_file is open, close it */
    if (expanded_file) {
        fclose(expanded_file);
        /* if operation failed, remove expanded_file */
        if (!success)
            remove(expanded_file_path);
    }
    /* if macros table created, free it */
    if (macros)
        hash_table_free(macros, free_macro);

    /* if labels array exists, free it and its members */
    if (labels) {
        /* loop all labels */
        for (i = 0; i < label_count; i++)
            /* free label */
            free(labels[i]);
        /* free labels array */
        free(labels);
    }

    /* return whether the operation succeeded or failed */
    return success;
}