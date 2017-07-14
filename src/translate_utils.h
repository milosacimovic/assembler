#ifndef _TRANSLATE_UTILS_H_
#define _TRANSLATE_UTILS_H_

#include <cstdint>
#include <cstdio>
#include <vector>

#include "tables.h"

extern const uint8_t MAX_ARGS;
extern const uint16_t BUF_SIZE;
extern const char* IGNORE_CHARS;
extern const int NUM_BUCKETS;

void write_hex_inst(uint32_t inst, RelTable* rel);


/* Writes the instruction as a string to OUTPUT. NAME is the name of the 
   instruction, and its arguments are in ARGS. NUM_ARGS is the length of
   the array.
 */
void write_inst_string(FILE* output, const char* name, char** args, int num_args);


/* Returns true if the label is valid and false if it is invalid. A valid label is one
   where the first character is a character or underscore and the remaining 
   characters are either characters, digits, or underscores.
 */
void remove_spaces(char* source);

int c_to_ind(char c);

char ind_to_c(int c);

bool is_valid_label(const char* str);

bool is_pass_two_directive(const char* dir);

bool is_directive(const char* dir);

int32_t convert_to_num(const char* str);

int32_t bin_op(int32_t oper1, int32_t oper2, char op);

bool is_operand(char c);

bool is_operator(char c);

int ipr(char c);

int spr(char c);

int R(char c);

char* to_postfix(char* expr);

int32_t eval_postfix(char* expr, std::vector<int32_t> literals);

void assign_ranks(std::vector<std::vector<Symbol*>>& symtbl);

int translate_reg(const char* str);

void name_already_exists(const char* name);

void invalid_label(uint32_t line, const char* label);

void inst_error(uint32_t line, const char* name, char** args, uint8_t num_args);

void skip_comment(char* str);

void extra_arg_error(uint32_t line, const char* extra);

int add_if_label(uint32_t line, char* str, uint32_t location_counter, std::vector<std::vector<Symbol*>>& symtbl);

#endif
