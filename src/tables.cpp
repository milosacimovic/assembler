#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "tables.h"
#include "utils.h"

using namespace std;


int char_to_ind(char c){
    if(c >= 'a' && c <= 'z'){
        return c - 'a';
    }else if(c >= 'A' && c <= 'Z'){
        return c- 'A' + 26;
    }else if(c == '_'){ return 52;}
    else if(c == '.'){ return 53;}
    else{ return -1;}

}

/* Creates a new SymbolTable containg 0 elements 
   and returns a pointer to that
   table. Multiple SymbolTables may exist at the same time. 
   If memory allocation fails, it calls allocation_failed(). 
   Mode will be either SYMTBL_NON_UNIQUE or SYMTBL_UNIQUE_NAME. 
   You will need to store this value for use during add_to_table().
 */
 /* The symbol table is organized as a 
    an array of buckets of linked list.
    There are 27 buckets, corresponding to
    27 possible chars that a symbol name
    can start with(i.e. a, b, c, d, e, f, 
    g, h, i, j, k, l, m, n, o, p, q, r, s, 
    t, u, v, w, x, y, z, _)
    Symbol is mapped into a bucket by
    looking at its first letter,
 */

/* Frees the given SymbolTable and all associated memory. */
void free_table(vector<vector<Symbol*>>& symtbl){

    for(int i = 0; i < NUM_BUCKETS; i++){
        for(int j = 0; j < symtbl[i].size(); j++){
            delete symtbl[i][j];
        }
        symtbl[i].clear();
    }
    symtbl.clear();
}
/* Adds a new symbol and its address to the SymbolTable pointed to by tbl. 
   @addr is given as the byte offset from the first instruction(i.e. location counter(LC)).
   The @tbl must be able to resize itself as more elements are added. 
   @name may point to a temporary array, so it is not safe to simply
   store the @name pointer. It stores a copy of the given string. If memory
   allocation fails, it calls alloc_fail(). 
   If the allocation succeededs but the symbol is already defined the function
   will return -1.
   Otherwise, it stores the symbol name and address and returns 0.

 */
int add_to_table(vector<vector<Symbol*>>& symtbl, const char* nam, uint32_t addr, Symbol** sym){
    
    char* name = strdup(nam);
    if(name == NULL){
        alloc_fail();
    }
    Symbol* in = new (nothrow)Symbol(name, addr);
    if(in == NULL){
        alloc_fail();
    }

    int ind = char_to_ind(name[0]);
    symtbl[ind].push_back(in);
    *sym = in;
    return 0;
}

/* Returns the address (byte offset) of the given symbol. If a symbol with name
   @name is not present in @tbl, return -1.
 */
uint32_t get_symbol_addr(vector<vector<Symbol*>>& symtbl, const char* name){
    int ind = char_to_ind(name[0]);
    for(int i = 0; i < symtbl[ind].size(); i++){
        if(strcmp(symtbl[ind][i]->name, name) == 0){
               return symtbl[ind][i]->addr;
        }
    }
    return -1;
}

Symbol* get_symbol(vector<vector<Symbol*>>& symtbl, const char* name){
    int ind = char_to_ind(name[0]);
    for(int i = 0; i < symtbl[ind].size(); i++){
        if(strcmp(symtbl[ind][i]->name, name) == 0){
               return symtbl[ind][i];
        }
    }
    return NULL;
}

Symbol* get_symbol(std::vector<std::vector<Symbol*>>& symtbl, uint32_t num){
    for(int i = 0; i < NUM_BUCKETS; i++){
        for(int j = 0; j < symtbl[i].size(); j++){
            if(symtbl[i][j]->num == num){
                return symtbl[i][j];
            }
        }
    }
}

/* Writes the SymbolTable @tbl to OUTPUT. Ranks of symbols have been
    assigned before pass_two!!!
 */
void write_table(vector<vector<Symbol*>>& symtbl, FILE* file){

    fprintf(file, "#TabelaSimbola\n");
    int num_sections = symtbl[NUM_BUCKETS-1].size();
    for(int i =0; i < num_sections; i++){
        Symbol * current = symtbl[NUM_BUCKETS - 1][i];
        fprintf(file, "SEG %u %s %d 0x%08x 0x%08x ",current->num, current->name, current->num, current->addr, current->sec_size);
		for (int j = 0; j < current->flags.size(); j++) {
			fprintf(file, "%c", current->flags[j]);
		}
		fprintf(file, "\n");
    }
    for(int i = 0; i < NUM_BUCKETS - 1; i++){
        int sz = symtbl[i].size();
        for(int j = 0; j < sz; j++){
            Symbol* current = symtbl[i][j];
            fprintf(file, "SYM %u %s %d 0x%08x %c\n", current->num, current->name, current->sec_num, current->addr, current->flag);
        }
    }
}

void create_rel_tables(vector<RelTable*>& rels, vector<vector<Symbol*>>& symtbl){
    int num_sec = symtbl[NUM_BUCKETS - 1].size();
    int secs = NUM_BUCKETS - 1;
    for(int i = 0; i < num_sec; i++){
        RelTable* t;
		t = new (nothrow)RelTable();
		if (t == NULL) {
			alloc_fail();
		}
        if(char_vector_contains(symtbl[secs][i]->flags, 'P')){
            
            t->section_content = new uint8_t[symtbl[secs][i]->sec_size];
            if(t->section_content == NULL){
                alloc_fail();
            }
        }
        rels.push_back(t);
    }
}

void add_rel_symbol(RelTable& table, uint32_t addr, char type, uint32_t sym_num){
    RelSymbol* sym = new (nothrow) RelSymbol(addr, type, sym_num);
    if(sym == NULL){
        alloc_fail();
    }
    table.tbl.push_back(sym);
}

void free_rel_tables(vector<RelTable*>& rels){
    int sz = rels.size();
    for(int i = 0; i < sz; i++){
        RelTable* t = rels[i];
        int tsz = t->tbl.size();
        for(int j = 0; j < tsz; j++){
            RelSymbol* sym = t->tbl[j];
            delete sym;
        }
        t->tbl.clear();
		if (t->section_content != NULL) {
			delete[] t->section_content;
		}
		delete t;
    }
}


void write_out(FILE* out, vector<vector<Symbol*>>& symtbl, vector<RelTable*>& rels){

    uint8_t sz = (uint8_t)strlen("#rel");
    int secs = NUM_BUCKETS - 1;
    int num_secs = symtbl[secs].size();
    int k = 0;
    RelTable* rel_table;
    for(int i = 0; i < num_secs; i++){
        Symbol* sym = symtbl[secs][i];
        //section
        if(char_vector_contains(sym->flags, 'P')){
            char* header = (char*)malloc(sizeof(char)*(sz + strlen(sym->name) + 1));
            strcpy(header, "#rel");
            strcat(header, sym->name);//header = #rel.sec_name
            fprintf(out, "%s\n", header);
            
            rel_table = rels[i];
            int relsz = rel_table->tbl.size();
            for(int m = 0; m < relsz; m++){
                RelSymbol* rel = rel_table->tbl[m];
                fprintf(out, "0x%08x %c %u\n", rel->addr, rel->type, rel->sym_num);
            }
            
            //section name
            fprintf(out, "%s\n", sym->name);
            //contents of section
            for(int j = 1; j <= sym->sec_size; j++){
                
                fprintf(out, "%02x", rel_table->section_content[j-1]);
                if(j % 16 == 0 && j != sym->sec_size){
                    fprintf(out,"\n");
                }else{
                    fprintf(out," ");
                }
            }
			fprintf(out, "\n");
        }//end_if
		
    }//end_for
}

TNSymbol* create_TNSymbol(char* name, char* expr) {
	TNSymbol* in = new (nothrow)TNSymbol(name, expr);
	if (in == NULL) {
		alloc_fail();
	}
	return in;
}

void add_TNS(TNSymbol** tns, TNSymbol* ins) {
	if (*tns == NULL) {
		*tns = ins;
	}
	else {
		ins->next = *tns;
		*tns = ins;
	}
}
