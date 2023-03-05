/** @file
*/

#include "first_pass.h"
#include "syntactical_analysis.h"
#include "encoding.h"
#include <string.h>

bool do_first_pass(char* path, memoryBuffer* img, SymbolTable* sym_table, debugList* dbg_list)
{
	FILE* in = NULL;
	LineIterator it;
	char* curr_line = NULL;
	long line = 1;
	bool should_encode = TRUE;

	fpass_dispatch_table table[FP_TOTAL] = {
		first_pass_process_sym_def,
		first_pass_process_sym_data,
		first_pass_process_sym_string,
		first_pass_process_sym_ext,
		first_pass_process_sym_ent,
		first_pass_process_opcode
	};
	in = open_file(path, MODE_READ);

	/* Read a new line from the input stream.*/
	while ((curr_line = get_line(in)) != NULL) {
		char* word = NULL;
		firstPassStates state;
		
		/* Feed the iterator with a new line. */
		line_iterator_put_line(&it, curr_line);
		/* Trim white spaces. */
		line_iterator_consume_blanks(&it);

		word = line_iterator_next_word(&it, " ");
		state = get_symbol_type(&it, word);

		if (state == FP_SYM_IGNORED) {
			debug_list_register_node(dbg_list, debug_list_new_node(it.start, it.current, line, ERROR_CODE_SYMBOL_IGNORED_WARN));
		}
		/* none of the above, must be an error. */
		/* if state FP_NONE register the node in the list and register it as a new node. */
		else if (state == FP_NONE) {
			debug_list_register_node(dbg_list, debug_list_new_node(it.start, it.current, line, ERROR_CODE_SYNTAX_ERROR));
			should_encode = FALSE;
		}
		else {
			should_encode &= table[state](&it, img, sym_table, dbg_list, word, line, should_encode);
		}

		free(word);
        free(curr_line);
		line++;
	}

    fclose(in);
	symbol_table_set_completed(sym_table, TRUE);
	
	return should_encode;
}

firstPassStates get_symbol_type(LineIterator* it, char* word)
{
	bool is_valid = TRUE;

	/* An .entry definition. */
	/* Returns FP_SYM_ENT or FP_SYM_EXT. entry. extern. data or. string.*/
	if (strcmp(word, DOT_ENTRY_STRING) == 0) {
		return FP_SYM_ENT;
	}
	/* An .extern definition. */
	/* Returns FP_SYM_EXT or FP_SYM_DEF depending on the word.*/
	else if (strcmp(word, DOT_EXTERN_STRING) == 0) {
		return FP_SYM_EXT;
	}
	/* Get the opcode of the word.*/
	else if (get_opcode(word) != OP_UNKNOWN) {
		/* Unget the opcode. */
		line_iterator_unget_word(it, word);
		return FP_OPCODE;
	}
	/* Symbol definition, may follow, .data or .string*/
	/* Check if the word is a valid label.*/
	else if ((is_valid = is_valid_label(word)) == TRUE) {
		char* next_word = line_iterator_next_word(it,SPACE_STRING);

		/* Free the next word. */
		if (!next_word) {
			free(next_word);
			return FP_NONE;
		}

		/* Check if .data */
		if (strcmp(next_word, DOT_DATA_STRING) == 0) {
            free(next_word);
			return FP_SYM_DATA;
		}
		/* Check if .string */
		if (strcmp(next_word, DOT_STRING__STRING) == 0) {
            free(next_word);
			return FP_SYM_STR;
		}
		/* Ungets the next word from the line iterator. */
		if (strcmp(next_word, ENTRY_STRING) == 0 || strcmp(next_word, EXTERN_STRING) == 0) {
			line_iterator_unget_word(it, next_word);
			line_iterator_unget_word(it, word);
            free(next_word);
			return FP_SYM_IGNORED;
		}

		/* Unget the word, and return FP_SYM_DEF */
		line_iterator_unget_word(it, next_word);
        free(next_word);
		return FP_SYM_DEF;
	}

	/* Returns FP_NONE if the field is not valid. */
	if (!is_valid) {
		return FP_NONE;
	}

	return FP_NONE;
}

bool first_pass_process_sym_def(LineIterator* it, memoryBuffer* img, SymbolTable* sym_table, debugList* dbg_list, char* name, long line, bool should_encode)
{
	/* Get a handle to the node, if the type is entry/extern then update its counter to the img->instruction_image.counter. */
	/* If it is not an extern/entry then register an error. */
	SymbolTableNode* node = symbol_table_search_symbol(sym_table, name);


	/* Register a symbol definition node.*/
	if (node && (symbol_get_type(symbol_node_get_sym(node)) == SYM_DATA || symbol_get_type(symbol_node_get_sym(node)) == SYM_CODE)) {
		debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_SYMBOL_REDEFINITION));
		return FALSE;
	}

	symbol_table_insert_symbol(sym_table, symbol_table_new_node(name, SYM_CODE, img_memory_get_counter(memory_buffer_get_inst_img(img))));

	/* Check the syntax, we want a copy of the iterator because if the syntax is correct we will encode the instructions to memory. */
	/* Check if the syntax is valid.*/
	if (!validate_syntax(*it, FP_SYM_DEF, line, dbg_list)) {
		return FALSE;
	}
	/* Encode to the image.*/
	if (should_encode) {
		encode_opcode(it, img);
	}

	return TRUE;
}

bool first_pass_process_opcode(LineIterator* it, memoryBuffer* img, SymbolTable* sym_table, debugList* dbg_list, char* name, long line, bool should_encode)
{
	/* Check the syntax, we want a copy of the iterator because if the syntax is correct we will encode the instructions to memory. */
	if (!validate_syntax(*it, FP_OPCODE, line, dbg_list)) {
		return FALSE;
	}
	/* Encode to the image.*/
	if (should_encode) {
		encode_opcode(it,img);
	}
	return TRUE;
}

bool first_pass_process_sym_data(LineIterator* it, memoryBuffer* img, SymbolTable* sym_table, debugList* dbg_list, char* name, long line, bool should_encode)
{
	/* Get a handle to the node, if the type is entry/extern then update its counter to the img->instruction_image.counter. */
	/* If it is not an extern/entry then register an error. */
	SymbolTableNode* node = symbol_table_search_symbol(sym_table, name);

	if (node && (symbol_get_type(symbol_node_get_sym(node)) == SYM_DATA || symbol_get_type(symbol_node_get_sym(node)) == SYM_CODE)) {
		debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_SYMBOL_REDEFINITION));
		return FALSE;
	}

	symbol_table_insert_symbol(sym_table, symbol_table_new_node(name, SYM_DATA, img_memory_get_counter(memory_buffer_get_inst_img(img)) + img_memory_get_counter(memory_buffer_get_data_img(img))));

	/* Check the syntax, we want a copy of the iterator because if the syntax is correct we will encode the instructions to memory. */
	if (!validate_syntax(*it, FP_SYM_DATA, line, dbg_list)) {
		return FALSE;
	}
	/* Encode the .data.*/
	if (should_encode) {
		encode_dot_data(it, img);
	}

	return TRUE;
}

bool first_pass_process_sym_string(LineIterator* it, memoryBuffer* img, SymbolTable* sym_table, debugList* dbg_list, char* name, long line, bool should_encode)
{
	if (symbol_table_search_symbol_bool(sym_table, name)) {
		debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_SYMBOL_REDEFINITION));
		return FALSE;
	}
	symbol_table_insert_symbol(sym_table, symbol_table_new_node(name, SYM_DATA, img_memory_get_counter(memory_buffer_get_data_img(img))));

	/* Check the syntax, we want a copy of the iterator because if the syntax is correct we will encode the instructions to memory. */

	/* Check if the syntax is valid. */
	if (!validate_syntax(*it, FP_SYM_STR, line, dbg_list)) {
		return FALSE;
	}

	/* Encode the image as a dot string. */
	if (should_encode) {
		encode_dot_string(it, img);
	}
	return TRUE;
}

bool first_pass_process_sym_ent(LineIterator* it, memoryBuffer* img, SymbolTable* sym_table, debugList* dbg_list, char* name, long line, bool should_encode)
{
    	char* word = line_iterator_next_word(it, SPACE_STRING);
	SymbolTableNode* node = NULL;

	line_iterator_unget_word(it, word);
	/* register a new word in the list*/
	if (!word) {
		debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_SYNTAX_ERROR));
		free(word);
		return FALSE;
	}
	if (!is_label_name(it)) {
		debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_INVALID_LABEL_DEF));
		free(word);
		return FALSE;
	}

	if (symbol_table_search_symbol_bool(sym_table, word) && check_symbol_existence(sym_table, word, SYM_ENTRY)) {
		debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_LABEL_ALREADY_EXISTS_AS_EXTERN));
		return FALSE;
	}

	line_iterator_unget_word(it, word);
	if (get_opcode(word) != OP_UNKNOWN || is_register_name_whole(it)) {
		debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_LABEL_CANNOT_BE_DEFINED_AS_OPCODE_OR_REGISTER));
		return FALSE;
	}
	line_iterator_next_word(it, " ");

	/* Check wheter the symbol already exist as an entry/extern directive */
	node = symbol_table_search_symbol(sym_table, word);


	/* Insert symbol in symbol table.*/
	if (node && (symbol_get_type(symbol_node_get_sym(node)) != SYM_ENTRY && symbol_get_type(symbol_node_get_sym(node)) != SYM_EXTERN)) {
		symbol_table_insert_symbol(sym_table, symbol_table_new_node(word, SYM_ENTRY, symbol_get_counter(symbol_node_get_sym(node))));
	}
	else {
		symbol_table_insert_symbol(sym_table, symbol_table_new_node(word, SYM_ENTRY, 0));
	}

	/* Check the syntax, we want a copy of the iterator because if the syntax is correct we will encode the instructions to memory. */
	/* Check if the syntax is valid.*/
	if (!validate_syntax(*it, FP_SYM_ENT, line, dbg_list)) {
		free(word);
		return FALSE;
	}

	free(word);

	return TRUE;
}

bool first_pass_process_sym_ext(LineIterator* it, memoryBuffer* img, SymbolTable* sym_table, debugList* dbg_list, char* name, long line, bool should_encode)
{
    	char* word = line_iterator_next_word(it, SPACE_STRING);

	line_iterator_unget_word(it, word);
	/* register a new word in the list*/
	if (!word) {
		debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_SYNTAX_ERROR));
		free(word);
		return FALSE;
	}
	if (!is_label_name(it)) {
		debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_INVALID_LABEL_DEF));
		free(word);
		return FALSE;
	}
	if (symbol_table_search_symbol_bool(sym_table, word) && check_symbol_existence(sym_table, word, SYM_EXTERN)) {
			debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_LABEL_ALREADY_EXISTS_AS_ENTRY));
			return FALSE;
	}
	line_iterator_unget_word(it, word);
	if (get_opcode(word) != OP_UNKNOWN || is_register_name_whole(it)) {
		debug_list_register_node(dbg_list, debug_list_new_node(it->start, it->current, line, ERROR_CODE_LABEL_CANNOT_BE_DEFINED_AS_OPCODE_OR_REGISTER));
		return FALSE;
	}
	line_iterator_next_word(it, SPACE_STRING);

	symbol_table_insert_symbol(sym_table, symbol_table_new_node(word, SYM_EXTERN, 0));

	/* Check the syntax, we want a copy of the iterator because if the syntax is correct we will encode the instructions to memory. */
	/* Check if the syntax is valid.*/
	if (!validate_syntax(*it, FP_SYM_ENT, line, dbg_list)) {
		free(word);
		return FALSE;
	}

	free(word);

	return TRUE;
}
