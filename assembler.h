#ifndef _ASSEMBLER_H_
#define _ASSEMBLER_H_

#include <vector>

int assemble(const char* input, const char* tmp, const char* out);

bool pass_one(FILE* input, FILE* output, std::vector<std::vector<Symbol*>>& symtbl);

int pass_two(FILE* input, FILE* output, std::vector<std::vector<Symbol*>>& symtbl, std::vector<RelTable*>& reltbl);

#endif