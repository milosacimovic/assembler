#ifndef _UTILS_H_
#define _UTILS_H_

#include <cstdio>
#include <cstdint>
#include <vector>
#include <cstdarg>

int is_log_file_set();

void set_log_file(const char* filename);

void write_to_log(char* fmt, ...);

void log_inst(const char* name, char** args, uint8_t num_args);

char* to_lower(const char* str);

bool str_contains(const char* str, char c);

bool char_vector_contains(std::vector<char>& str, char c);

void alloc_fail();

#endif