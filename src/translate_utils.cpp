#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <stack>

#include "utils.h"
#include "translate_utils.h"
#include "tables.h"

using namespace std;

int c_to_ind(char c){
    return ((int)c-(int)'a');
}

char ind_to_c(int c){
    return 'a' + (char)c;
}


void write_inst_string(FILE* output, const char* name, char** args, int num_args) {
    fprintf(output, "%s", name);
    for (int i = 0; i < num_args; i++) {
        fprintf(output, " %s", args[i]);
    }
    fprintf(output, "\n");
}

bool is_valid_label(const char* str) {
    if (str == NULL) {
        return false;
    }

    bool first = true;
    while (*str) {
        if (first) {
            if (!isalpha((int) *str) && *str != '_') {
                return false;
            } else {
                first = false;
            }
        } else if (!isalnum((int) *str) && *str != '_') {
            return false;
        }
        str++;
    }
    return first ? false : true;
}

bool is_directive(const char* dir){
    char* str = strdup(dir);
    char* p = strrchr(str, '.');
    if(p){
        *p = '\0';
    }
    if(strcmp(str, ".global") == 0 ||
        strcmp(str, ".text") == 0 ||
        strcmp(str, ".data") == 0 ||
        strcmp(str, ".bss") == 0 ||
        strcmp(str, ".rodata") == 0 ||
        strcmp(str, "org") == 0 ||
        strcmp(str, ".end") == 0 ||
        strcmp(str, "db") == 0 ||
        strcmp(str, "dw") == 0 ||
        strcmp(str, "dd") == 0 ||
        strcmp(str, "def") == 0){
            return true;
    }else{
            return false;
    }
}

bool is_pass_two_directive(const char* dir){
    char* str = strdup(dir);
    char* p = strrchr(str, '.');
    if(p){
        *p = '\0';
    }
    if( strcmp(str, ".text") == 0 ||
        strcmp(str, ".data") == 0 ||
        strcmp(str, ".bss") == 0 ||
        strcmp(str, ".rodata") == 0 ||
        strcmp(str, "db") == 0 ||
        strcmp(str, "dw") == 0 ||
        strcmp(str, "dd") == 0){
            return true;
    }else{
            return false;
    }
}
bool is_operator(char c){
    if(c == '+' || c == '/' || c == '-' || c == '*')
        return true;
    else
        return false;
}

bool is_operand(char c){
    if(c >= 'a' && c <= 'z'){
        return true;
    }else{
        return false;
    }
}

int32_t convert_to_num(const char* str){
    int32_t res;
    switch(str[0]){
        case '0'://binary or hex
            switch(str[1]){
                case 'b'://binary
                    res = strtol(str, NULL, 2);
                    break;
                case 'x'://hex
                case 'X':
                    res = (int32_t)strtol(str, NULL, 16);
                    break;
            }
            break;
        case '\''://character
            res = (int32_t)str[1];
        default:
            res = atoi(str);
            break;
    }
    if(res == 0){
        write_to_log("Error: unallowed characters in constant %s.", str);
    }else{
        return res;
    }
}

int32_t bin_op(int32_t oper1, int32_t oper2, char op){
    int32_t res = 0;
    switch(op){
        case '+':
            res = oper1 + oper2;
            break;
        case '-':
            res = oper1 - oper2;
            break;
        case '*':
            res = oper1 * oper2;
            break;
        case '/':
            res = oper1 / oper2;
            break;
    }
}

void remove_spaces(char* source){
  char* i = source;
  char* j = source;
  while(*j){
    *i = *j++;
    if(*i != ' ')
      i++;
  }
  *i = '\0';
}

int ipr(char c){
    int ret;
    switch(c){
        case '+':
        case '-':
        ret = 2;
        break;
        case '*':
        case '/':
        ret = 3;
        break;
        case '(':
        ret = 6;
        break;
        case ')':
        ret = 1;
        break;
    }
    return ret;
}

int spr(char c){
    int ret;
    switch(c){
        case '+':
        case '-':
        ret = 2;
        break;
        case '*':
        case '/':
        ret = 3;
        break;
        case '(':
        ret = 0;
        break;
    }
    return ret;
}

int R(char c){
    int ret;
    switch(c){
        case '+':
        case '-':
        ret = -1;
        break;
        case '*':
        case '/':
        ret = -1;
        break;
    }
    return ret;
}


char* to_postfix(char* expr){
    stack<char> s;
    char x;
    int rank = 0;
    int i = 0;
    char* out = strdup(expr);    
    char next = *expr;
    expr++;
    while(next){
        if(is_operand(next)){
            out[i++] = next;
            rank++;
        }else{
            while(!s.empty() && ipr(next) <= spr(s.top())){
                x = s.top();
                s.pop();
                out[i++] = x;
                rank = rank + R(x);
                if(rank < 1){
                    write_to_log("Error: expression %s is irregular.", expr);
                }
            }
            if(next != ')'){
                s.push(next);
            }else{
                x = s.top();
                s.pop();
            }
        }
        next = *expr;
        expr++;
            
    }
    while(!s.empty()){
        x = s.top();
        s.pop();
        out[i++] = x;
        rank = rank + R(x);
    }
    if(rank != 1){
        write_to_log("Error: expression %s is irregular.", expr);
    }
    return out;
}

int32_t eval_postfix(char* expr, vector<int32_t> literals){
    stack<int32_t> s;
    int32_t res;
    char op;
    while(*expr){
        char x = *expr;
        if(is_operand(x)){
            char x;
            s.push(literals[c_to_ind(x)]);
        }else if(is_operator(x)){
            int32_t oper2 = s.top();
            s.pop();
            int32_t oper1 = s.top();
            s.pop();
            res = bin_op(oper1, oper2, x);
            s.push(res);
        }
        expr++;
    }
    res = s.top();
    s.pop();
    if(s.empty()){
        return res;
    }else{
        write_to_log("Error: expression %s is irregular.", expr);
    }
}

int32_t calculate_expression(char* expr){
    vector<int32_t> literals;
    char* out = strdup(expr);
    int i = 0;//literal counter
    int str_ind = 0;
    bool cur_literal = false;
    char* str = expr;
    char literal[35];
    int literal_ind = 0;
    while(*str){
        char c = *str;
        if(str_contains("+-*/()",c)){
            if(cur_literal){
                cur_literal = false;
                //exchange literal with a letter
                char ins = ind_to_c(i++);
                out[str_ind++] = ins;
                literal[literal_ind] = '\0';
                literals.push_back(convert_to_num(literal));
            }
            out[str_ind++] = c;
        }else{
            if(!cur_literal){
                cur_literal = true;
                literal_ind = 0;
            }
            literal[literal_ind++] = c;
        }
        str++;
    }
    char* ex = to_postfix(out);
    free(out);
    return eval_postfix(ex, literals);
}

void assign_ranks(vector<vector<Symbol*>>& symtbl){
    int rank = symtbl[NUM_BUCKETS - 1].size() + 1;
    for(int i = 0; i < NUM_BUCKETS-1; i++){
        int sz = symtbl[i].size();
        for(int j = 0; j < sz; j++){
            symtbl[i][j]->num = rank++;
        }
    }
}


int translate_reg(const char* str) {
    if (strcmp(str, "r0") == 0)      return 0;
    else if (strcmp(str, "r1") == 0)    return 1;
    else if (strcmp(str, "r2") == 0)   return 2;
    else if (strcmp(str, "r3") == 0)   return 3;
    else if (strcmp(str, "r4") == 0)   return 4;
    else if (strcmp(str, "r5") == 0)   return 5;
    else if (strcmp(str, "r6") == 0)   return 6;
    else if (strcmp(str, "r7") == 0)   return 7;
    else if (strcmp(str, "r8") == 0)   return 8;
    else if (strcmp(str, "r9") == 0)   return 9;
    else if (strcmp(str, "r10") == 0)   return 10;
    else if (strcmp(str, "r11") == 0)   return 11;
    else if (strcmp(str, "r12") == 0)   return 12;
    else if (strcmp(str, "r13") == 0)   return 13;
    else if (strcmp(str, "r14") == 0)   return 14;
    else if (strcmp(str, "r15") == 0)   return 15;
    else if (strcmp(str, "sp") == 0)   return 16;
    else if (strcmp(str, "pc") == 0)   return 17;
    else return -1;
}

void write_hex_inst(uint32_t inst, RelTable* rel){
    char str[9];  
    sprintf(str, "%08x", inst);
    uint8_t first = strtol(str+6,NULL,16);
    str[6] = '\0';
    uint8_t second = strtol(str+4,NULL,16);
    str[4] = '\0';
    uint8_t third = strtol(str+2,NULL,16);
    str[2] = '\0';
    uint8_t fourth = strtol(str,NULL,16);;
    rel->section_content[rel->ind++] = first;
    rel->section_content[rel->ind++] = second;
    rel->section_content[rel->ind++] = third;
    rel->section_content[rel->ind++] = fourth;
    
}