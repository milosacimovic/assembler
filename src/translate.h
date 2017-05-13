#ifndef _TRANSLATE_H
#define _TRANSLATE_H

#include <vector>
#include <cstdio>
#include <cstdint>

#include "tables.h"

extern const uint8_t MAX_ARGS;
extern const uint16_t BUF_SIZE;
extern const char* IGNORE_CHARS;
extern const int NUM_BUCKETS;

void write_pass_one(FILE* output, const char* name, char** args, uint8_t num_args);

void def_directive(char* buf, std::vector<std::vector<Symbol*>>& symtbl);

void translate_directive_pass_one(const char* name, char* buf, FILE* output, std::vector<std::vector<Symbol*>>& symtbl,uint32_t& location_counter, const char* prev_name);

void translate_directive_pass_two(const char* name, char* buf, std::vector<std::vector<Symbol*>>& symtbl, std::vector<RelTable*>& reltbls, uint32_t& location_counter, int& rel_ind);

void location_counter_update(char** args, int num_args, uint32_t& location_counter);

int translate_inst(const char* name, char** args, int num_args, std::vector<std::vector<Symbol*>>& symtbl, std::vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter);

int trans_ar_log(uint8_t opcode, char** args, int num_args, std::vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter);

int trans_not(uint8_t opcode, char** args, int num_args, std::vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter);

int trans_stack(uint8_t opcode, char** args, int num_args, std::vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter);

int trans_int(uint8_t opcode, char** args, int num_args, std::vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter);

int trans_ret(uint8_t opcode, char** args, int num_args, std::vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter);

int32_t check_sub(char* expr, std::vector<Symbol*>& symbols, bool& ret, uint32_t& sym_num);

int32_t resolve_addr_mode(char* arg, uint32_t& instruction1, uint32_t& instruction2, std::vector<int32_t>& literals, int32_t tmp,bool& ret, uint32_t& sym_num);

int32_t resolve_addr_mode(char* arg, uint32_t& instruction1, uint32_t& instruction2, std::vector<int32_t>& literals, int32_t tmp,bool& ret, uint32_t& sym_num, int& r);

int trans_uncond_branch(uint8_t opcode, char** args, int num_args, std::vector<std::vector<Symbol*>>& symtbl, std::vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter);

int trans_cond_branch(uint8_t opcode, char** args, int num_args, std::vector<std::vector<Symbol*>>& symtbl, std::vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter);

int trans_load(uint8_t opcode,const char* name, char** args, int num_args, std::vector<std::vector<Symbol*>>& symtbl, std::vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter);

int trans_store(uint8_t opcode,const char* name, char** args, int num_args, std::vector<std::vector<Symbol*>>& symtbl, std::vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter);
#endif

