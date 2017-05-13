#ifndef _TABLES_H_
#define _TABLES_H_

#include <vector>
#include <cstdint>

extern const uint8_t MAX_ARGS;
extern const uint16_t BUF_SIZE;
extern const char* IGNORE_CHARS;
extern const int NUM_BUCKETS;

/* Signature of the SymbolTable data structure. */

typedef struct Symbol{
    uint32_t num;
    char *name;
    int32_t sec_num;/*0 - external, 
                    -1 - absolute*/
    uint32_t addr; /*starts with 0x*/
    char flag; /*'G' for global and 'L' for local*/
    uint32_t sec_size;
    std::vector<char> flags; /*sec flags W- writeable A - allocatable X - executable P - present */
    Symbol(char* n, uint32_t address): num(0), name(n), sec_num(0), addr(address), flag('L'), sec_size(0), flags(3){}
} Symbol;

/* Signature of the RelTable data structure. */

typedef struct RelSymbol{
    uint32_t addr; /*starts with 0x*/
    char type; /*'A' for absolute and 'R' for relative*/
    uint32_t sym_num;/*or sec num if the symbol is local*/
    RelSymbol(uint32_t address, char t, uint32_t num):addr(address),type(t), sym_num(num) {}
}RelSymbol;

typedef struct RelTable{
    std::vector<RelSymbol*> tbl;
    int ind;
    uint8_t* section_content;/*This array will be created upon reaching
                            a particular section in the intermediate file, during pass_two. 
                            It will have sec_size elements.
                            After pass_two, the array of relocation tables
                            will be traversed and for each relocation table
                            which, of course, referes to one section
                            the table will be written to the object file
                            followed by its section content*/
    RelTable():ind(0), section_content(NULL), tbl(5){}
}RelTable;

int char_to_ind(char c);

int add_to_table(std::vector<std::vector<Symbol*>>& symtbl, const char* name, uint32_t addr, Symbol** sym);

uint32_t get_symbol_addr(std::vector<std::vector<Symbol*>>& symtbl, const char* name);

Symbol* get_symbol(std::vector<std::vector<Symbol*>> symtbl, const char* name);

void free_table(std::vector<std::vector<Symbol*>>& symtbl);

void write_table(std::vector<std::vector<Symbol*>>& symtbl, FILE* file);

void create_rel_tables(std::vector<RelTable*>& rels, std::vector<std::vector<Symbol*>>& symtbl);

void add_rel_symbol(RelTable& table, uint32_t addr, char type, uint32_t sym_num);

void free_rel_tables(std::vector<RelTable*>& rels);

void write_out(FILE* out, std::vector<std::vector<Symbol*>>& symtbl, std::vector<RelTable*>& rels);

#endif