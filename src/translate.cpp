#include <stack>
#include <cstdlib>
#include <cstring>

#include "utils.h"
#include "translate_utils.h"
#include "translate.h"
#include "tables.h"

using namespace std;

static uint32_t org_expr = 0;

static uint32_t seg_addr = 0;

static uint32_t current_sec = 0;

void write_pass_one(FILE* output, const char* name, char** args, uint8_t num_args){
    write_inst_string(output, name, args, num_args);
}


int addressing_mode(char* str) {
	size_t len;
	int ret;
	switch (str[0]) {
	case '#':
		ret = 4;
		break;
	case '[':
		len = strlen(str);
		if (len > 5) {
			ret = 7;
		}
		else {
			ret = 2;
		}
		break;
	case '$':
		ret = 1;
		break;
	default:
		if (str_contains(str, '+') || str_contains(str, '-') || str_contains(str, '*') || str_contains(str, '/')) {
			ret = 6;
		}
		else if (translate_reg(str) == -1) {
			ret = 6;
		}
		else {
			ret = 0;
		}
		break;

	}
	return ret;
}
/*  Need to check if the expression is correct
so what I'll do is make a subexpression
consisting only of the 1s and 0s
1s for valid labels and 0s for absolute labels
if the sub expression resolves to 1 or 0
with the condition that subtraction is done on symbols
with sam sec_num then the expression is correct
otherwise it is not and return false
sym_num needs to be from the symbol whose 1 is the result
of the subexpression
*/
//resolve literals to symbols for subexpression checking
int32_t calculate_data_expression(char* arg, int32_t tmp, bool& ret, uint32_t& sym_num, vector<vector<Symbol*>>& symtbl, int32_t& sub_res) {
	char* out = strdup(arg);
	int i = 0; //operand counter
	int str_ind = 0;
	bool cur_operand = false; //boolean
	char* str = arg;
	char literal[35];
	int literal_ind = 0;
	char prev_operator = '?';
	vector<int32_t> literals;
	vector<Symbol*> sub_literals;
	char* sub_str = strdup(arg);
	int j = 0;
	int sub_str_ind = 0;
	char c;
	while (*str) {
		c = *str;
		if (str_contains("+-*/()", c)) {
			if (cur_operand) {
				cur_operand = false;
				//exchange literal with a letter
				char ins = ind_to_c(i++);
				out[str_ind++] = ins;
				literal[literal_ind] = '\0';

				char sub_ins = ind_to_c(j++);
				if (prev_operator == '+' || prev_operator == '-') {
					sub_str[sub_str_ind++] = prev_operator;
				}
				else if (prev_operator != '?') {
					ret = true;
					return 0;
				}
				sub_str[sub_str_ind++] = sub_ins;
				if (is_valid_label(literal)) {
					Symbol* sym = get_symbol(symtbl, literal);
					
					if (sym == NULL) {//must be an external symbol
									  //exchange it with 0
						add_to_table(symtbl, literal, 0, &sym);
						literals.push_back(0);
					}
					else {
						int32_t off = tmp;
						if (sym->flag == 'L' || sym->sec_num == -1) {
							off += sym->addr;
						}
						literals.push_back(off);
					}
					sub_literals.push_back(sym);
				}
				else {
					sub_literals.push_back(new Symbol("", 0, -1));
					literals.push_back(convert_to_num(literal));
				}
			}
			out[str_ind++] = c;
			prev_operator = c;
		}else {
			if (!cur_operand) {
				cur_operand = true;
				literal_ind = 0;
			}
			literal[literal_ind++] = c;
		}
		str++;
	}
	if (cur_operand) {
		//exchange literal with a letter
		char ins = ind_to_c(i++);
		out[str_ind++] = ins;
		literal[literal_ind] = '\0';
		char sub_ins = ind_to_c(j++);
		if (prev_operator == '+' || prev_operator == '-') {
			sub_str[sub_str_ind++] = prev_operator;
		}
		else if (prev_operator != '?') {
			ret = true;
			return 0;
		}
		sub_str[sub_str_ind++] = sub_ins;
		if (is_valid_label(literal)) {
			Symbol* sym = get_symbol(symtbl, literal);
			
			if (sym == NULL) {//must be an external symbol
							  //exchange it with 0
				add_to_table(symtbl, literal, 0, &sym);
				literals.push_back(0);
			}
			else {
				int32_t off = tmp;

				if (sym->flag == 'L' || sym->sec_num == -1) {
					off += sym->addr;
				}
				literals.push_back(off);
			}
			sub_literals.push_back(sym);
		}
		else {

			sub_literals.push_back(new Symbol("", 0, -1));
			literals.push_back(convert_to_num(literal));
		}
	}
	out[str_ind++] = '\0';
	sub_str[sub_str_ind++] = '\0';
	//check if the subexpression is valid
	char* sub_ex = to_postfix(sub_str);
	bool err;
	sub_res = check_sub(sub_ex, sub_literals, err, sym_num);
	if (err) {
		ret = true;
		return 0;
	}
	if (sub_res == 1) {
		ret = false;
	}
	else if (sub_res == 0) {
		ret = false;
	}
	else {
		ret = true;
		return 0;
	}
	
	char* ex = to_postfix(out);
	return eval_postfix(ex, literals);
}

//PAY ATTENTION little endian
void data_pass_two(const char* name, char* buf, vector<RelTable*>& reltbls, uint32_t& location_counter, int& rel_ind, vector<vector<Symbol*>>& symtbl){
	char* tok = strdup(buf);
	char* cur = strtok(tok, IGNORE_CHARS);
    cur = strtok(NULL, IGNORE_CHARS);// cur = const_expr
	int32_t sub_res;
	uint32_t sym_num;
    int res;
	bool ret;
    char str[9];
    
    if(strstr(buf, "DUP")){
		// what if there is a following example dup DD dup label named dup ????
        // if dup or DUP is in the string then this points to
		// this can can have symbols in it
		res = calculate_data_expression(cur, 0, ret, sym_num, symtbl, sub_res);
		if (sub_res != 0) {
			write_to_log("Error: expression %s is not regular", cur);
			return;
		}
        int num_rept = res;
        switch(name[1]){
            case 'b':
				reltbls[rel_ind]->ind += num_rept;
				location_counter += num_rept;
                break;
            case 'w':
				reltbls[rel_ind]->ind += 2*num_rept;
                location_counter += 2*num_rept;
                break;
			case 'd':
				if (str_contains(buf, '?')) {
					reltbls[rel_ind]->ind += 4*num_rept;
					location_counter += 4 * num_rept;
				}
				break;
        }
        if(!str_contains(buf, '?')){
            cur = strtok(buf+2, "DUP");
            cur = strtok(NULL, "DUP"); //cur = const_expr that is repeated
            if(cur == NULL){
                write_to_log("Error: %s expected an expression\n", name);
            }else{
                
				uint8_t first;
				uint8_t second;
				uint8_t third = (uint8_t)strtol(str + 2, NULL, 16);
				str[2] = '\0';
				uint8_t fourth = (uint8_t)strtol(str, NULL, 16);
				int ind = reltbls[rel_ind]->ind;
                switch(name[1]){
                    case 'b':
						// still can have difference of two symbols or an absolute symbol
						res = calculate_data_expression(cur, 0, ret, sym_num, symtbl, sub_res);
						if (sub_res != 0) {
							write_to_log("Error: expression %s is not regular\n", cur);
							return;
						}
						sprintf(str, "%08x", res);
						first = (uint8_t)strtol(str + 6, NULL, 16);
						for (int i = 0; i < num_rept; i++) {
							reltbls[rel_ind]->section_content[ind++] = first;
						}
                        break;
                    case 'w':
						// still can have difference of two symbols or an absolute symbol
						res = calculate_data_expression(cur, 0, ret, sym_num, symtbl, sub_res);
						if (sub_res != 0) {
							write_to_log("Error: expression %s is not regular\n", cur);
							return;
						}
						sprintf(str, "%08x", res);
						first = (uint8_t)strtol(str + 6, NULL, 16);
						str[6] = '\0';
						second = (uint8_t)strtol(str + 4, NULL, 16);
						for (int i = 0; i < num_rept; i++) {
							reltbls[rel_ind]->section_content[ind++] = (uint8_t)strtol(str + 6, NULL, 16);
							str[6] = '\0';
							reltbls[rel_ind]->section_content[ind++] = (uint8_t)strtol(str + 4, NULL, 16);
						}
                        break;
                    case 'd':

						//calculate data expression and if needed make a relocation table entry
						res = calculate_data_expression(cur, 0, ret, sym_num, symtbl, sub_res);
						if (ret) {
							return;
						}
						sprintf(str, "%08x", res);
						first = (uint8_t)strtol(str + 6, NULL, 16);
						str[6] = '\0';
						second = (uint8_t)strtol(str + 4, NULL, 16);
						str[4] = '\0';
						third = (uint8_t)strtol(str + 2, NULL, 16);
						str[2] = '\0';
						fourth = (uint8_t)strtol(str, NULL, 16);
						for (int i = 0; i < num_rept; i++) {
							if (sub_res == 1) {
								add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
							}
							reltbls[rel_ind]->section_content[ind++] = first;
							reltbls[rel_ind]->section_content[ind++] = second;
							reltbls[rel_ind]->section_content[ind++] = third;
							reltbls[rel_ind]->section_content[ind++] = fourth;
							location_counter += 4;
						}
                        break;
                }
				reltbls[rel_ind]->ind = ind;
            }
        }
    }else{
		
        int ind = reltbls[rel_ind]->ind;
        switch(name[1]){
            case 'b':
				// still can have difference of two symbols or an absolute symbol
				res = calculate_data_expression(cur, 0, ret, sym_num, symtbl, sub_res);
				if (sub_res != 0) {
					write_to_log("Error: expression %s is not regular\n", cur);
					return;
				}
				sprintf(str, "%08x", res);
                reltbls[rel_ind]->section_content[ind++] = (uint8_t)strtol(str + 6,NULL,16);
                location_counter+=1;
                break;
            case 'w':
				// still can have difference of two symbols or an absolute symbol
				res = calculate_data_expression(cur, 0, ret, sym_num, symtbl, sub_res);
				if (sub_res != 0) {
					write_to_log("Error: expression %s is not regular\n", cur);
					return;
				}
				sprintf(str, "%08x", res);
                reltbls[rel_ind]->section_content[ind++] = (uint8_t)strtol(str + 6,NULL,16);
                str[6] = '\0';
                reltbls[rel_ind]->section_content[ind++] = (uint8_t)strtol(str + 4,NULL,16);
                location_counter+=2;
                break;
            case 'd':
				res = calculate_data_expression(cur, 0, ret, sym_num, symtbl, sub_res);
				if (ret) {
					return;
				}
				if (sub_res == 1) {
					add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
				}
				sprintf(str, "%08x", res);
                reltbls[rel_ind]->section_content[ind++] = (uint8_t)strtol(str + 6,NULL,16);
                str[6] = '\0';
                reltbls[rel_ind]->section_content[ind++] = (uint8_t)strtol(str + 4,NULL,16);
                str[4] = '\0';
                reltbls[rel_ind]->section_content[ind++] = (uint8_t)strtol(str + 2,NULL,16);
                str[2] = '\0';
                reltbls[rel_ind]->section_content[ind++] = (uint8_t)strtol(str,NULL,16);
                location_counter+=4;
                break;
        }
        reltbls[rel_ind]->ind = ind;
    }
	free(tok);
}

void global_directive(char* buf, vector<vector<Symbol*>>& symtbl, uint32_t& location_counter){
    char* cur = strtok(buf, IGNORE_CHARS);
    cur = strtok(NULL, ",\f\n\v\r");
    while(cur != NULL){
        remove_spaces(cur);
        if(is_valid_label(cur)){
            Symbol* sym = get_symbol(symtbl, cur);
            if(sym != NULL){
                sym->flag = 'G';
            }
            else{
                if(add_to_table(symtbl, cur, location_counter, &sym) == 0){
                    sym->flag = 'G';
                }
            }
        }

        cur = strtok(NULL, ",\f\n\v\r");
    }
}

void section_directive(const char* sec, vector<vector<Symbol*>>& symtbl, uint32_t& location_counter){

    Symbol* sym;
    if(add_to_table(symtbl, sec, 0, &sym) == 0){

        int num_secs = symtbl[NUM_BUCKETS - 1].size();
        if(num_secs - 2 >= 0){
            symtbl[NUM_BUCKETS - 1][num_secs - 2]->sec_size = location_counter;
        }
        sym->num = symtbl[NUM_BUCKETS - 1].size();
        sym->sec_num = sym->num;
        
		location_counter = 0;
		
		char* str = strdup(sec);
		char* p = strrchr(str, '.');
		if (p && p != str) {
			*p = '\0';
		}
        if(strcmp(str, ".text") == 0){
            		sym->flags.push_back('A');
			sym->flags.push_back('X');
			sym->flags.push_back('P');
        } 
        else if(strcmp(str, ".data") == 0){
            		sym->flags.push_back('A');
			sym->flags.push_back('W');
			sym->flags.push_back('P');
        }
	else if(strcmp(str, ".bss") == 0){
			sym->flags.push_back('A');
			sym->flags.push_back('W');		
	}
        else if(strcmp(str, ".rodata") == 0){
            		sym->flags.push_back('A');
			sym->flags.push_back('P');
        }
    }
}

void data_directive(char* buf, vector<vector<Symbol*>>& symtbl, uint32_t& location_counter){
	char* cur = strdup(buf);
	char* start = strtok(cur, IGNORE_CHARS);
	int32_t res;
	bool ret;
	uint32_t sym_num;
	int32_t sub_res;
	int num_rept;
	size_t len = strlen(start);
	if (start[len - 1] == ':') {
		cur = strtok(NULL, IGNORE_CHARS);
	}
	char* name = strdup(cur);
	cur = strtok(NULL, IGNORE_CHARS);
	if (strstr(buf, "DUP")) {
		// what if there is a label named DUP used in an expression without DUP
		res = calculate_data_expression(cur, 0, ret, sym_num, symtbl, sub_res);
		if (sub_res != 0) {
			write_to_log("Error: expression %s is not regular\n", cur);
			return;
		}
		num_rept = res;
	}
	else {
		num_rept = 1;
	}
	switch (name[1]) {
	case 'B':
		location_counter += num_rept;
		break;
	case 'W':
		location_counter += 2 * num_rept;
		break;
	case 'D':
		location_counter += 4 * num_rept;
	}
	
	
}


int32_t def_expression(char* arg, bool& ret, vector<vector<Symbol*>>& symtbl, int32_t& sub_res) {
	char* out = strdup(arg);
	int i = 0; //operand counter
	int str_ind = 0; //out index
	bool cur_operand = false; //currently inside operand 
	char* str = arg;
	char literal[35];
	int literal_ind = 0;
	char prev_operator = '?';
	uint32_t sym_num;
	char* sub_str = strdup(arg);
	int j = 0;
	int sub_str_ind = 0;
	char c;
	vector<int32_t> literals;
	vector<Symbol*> sub_literals;
	while (*str) {
		c = *str;
		if (str_contains("+-*/()", c)) {
			if (cur_operand) {
				cur_operand = false;
				//exchange literal with a letter
				char ins = ind_to_c(i++);
				out[str_ind++] = ins;
				literal[literal_ind] = '\0';
				char sub_ins = ind_to_c(j++);
				if (prev_operator == '+' || prev_operator == '-') {
					sub_str[sub_str_ind++] = prev_operator;
				}
				else if (prev_operator != '?') {
					ret = true;
					//write_to_log... not allowed operation with symbol 
					return 0;
				}
				sub_str[sub_str_ind++] = sub_ins;
				if (is_valid_label(literal)) {
					Symbol* sym = get_symbol(symtbl, literal);
					
					if (sym == NULL) {
						sub_res = -1;
						return 0;
					}
					else {
						int32_t off = 0;
						if (sym->flag == 'L' || sym->sec_num == -1) {
							off += sym->addr;
						}
						literals.push_back(off);
					}
					sub_literals.push_back(sym);
				}
				else {
					sub_literals.push_back(new Symbol("", 0, -1));
					literals.push_back(convert_to_num(literal));
				}
			}
			prev_operator = c;
			out[str_ind++] = c;
		}
		else {
			if (!cur_operand) {
				cur_operand = true;
				literal_ind = 0;
			}
			literal[literal_ind++] = c;
		}
		str++;
	}
	if (cur_operand) {
		//exchange literal with a letter
		char ins = ind_to_c(i++);
		out[str_ind++] = ins;
		literal[literal_ind] = '\0';
		char sub_ins = ind_to_c(j++);
		if (prev_operator == '+' || prev_operator == '-') {
			sub_str[sub_str_ind++] = prev_operator;
		}
		else if (prev_operator != '?') {
			ret = true;
			return 0;
		}
		sub_str[sub_str_ind++] = sub_ins;
		if (is_valid_label(literal)) {
			Symbol* sym = get_symbol(symtbl, literal);
			
			if (sym == NULL) {
				sub_res = -1;
				return 0;
			}
			else {
				int32_t off = 0;

				if (sym->flag == 'L' || sym->sec_num == -1) {
					off += sym->addr;
				}
				literals.push_back(off);
			}
			sub_literals.push_back(sym);
		}
		else {
			sub_literals.push_back(new Symbol("", 0, -1));
			literals.push_back(convert_to_num(literal));
		}
	}
	out[str_ind++] = '\0';
	sub_str[sub_str_ind++] = '\0';
	//check if the subexpression is valid
	char* sub_ex = to_postfix(sub_str);
	bool err;
	sub_res = check_sub(sub_ex, sub_literals, err, sym_num);
	if (err) {
		ret = true;
		return 0;
	}
	if (sub_res == 1) {
		ret = false;
	}
	else if (sub_res == 0) {
		ret = false;
	}
	else {
		ret = true;
		return 0;
	}

	char* ex = to_postfix(out);
	return eval_postfix(ex, literals);
}

/**/
void def_directive(char* buf, vector<vector<Symbol*>>& symtbl, TNSymbol** TNS){
    Symbol* sym;
	bool ret;
	uint32_t sym_num;
	int32_t sub_res;
    char* cur = strtok(buf, IGNORE_CHARS); //cur = sym
    char* name = strdup(cur);
    if(is_valid_label(name)){
        cur = strtok(NULL, IGNORE_CHARS);//cur = DEF
        cur = strtok(NULL, IGNORE_CHARS);//cur = const_expr
        if(cur == NULL){
            write_to_log("Error: %s not initialized in DEF directive.\n", name);
            return;
        }
        char* expr = strdup(cur);
        int32_t ex = def_expression(expr, ret, symtbl, sub_res);
		if (sub_res == -1) {// it is not possible to get the value of the symbol now
				  //add it to the TNS table
				  //along with the expression
			TNSymbol* s = create_TNSymbol(name, expr);
			add_TNS(TNS, s);
		}
		else if (sub_res != 0) {
				write_to_log("Error: expression %s is not regular\n", cur);
				return;
		}else if (!ret){
			if (add_to_table(symtbl, name, ex, &sym) == 0) {
				sym->sec_num = -1;
			}
		}
		else {
			write_to_log("Error upon calculating the subexpression.\n");
		}
    }
}

void resolve_TNS(TNSymbol** tns, vector<vector<Symbol*>>& symtbl) {
	if (*tns == NULL) return;
	bool change = true;
	bool ret;
	int32_t sub_res;
	while (change) {
		change = false;
		TNSymbol* cur = *tns;
		TNSymbol* prev = NULL;
		TNSymbol* del;
		Symbol* in;
		while (cur != NULL) {
			int32_t res = def_expression(cur->expr, ret, symtbl, sub_res);
			if (sub_res == 0) {
				change = true;
				add_to_table(symtbl, cur->def_sym, res, &in);
				in->sec_num = -1;
				del = cur;
				if (prev == NULL) {
					*tns = (*tns)->next;
				}
				else {
					prev->next = cur->next;
				}
				cur = cur->next;
				delete del;
			}
			else {
				prev = cur;
				cur = cur->next;
			}
			
		}
	}
}

void org_directive(char* buf, vector<vector<Symbol*>>& symtbl){
	bool ret;
	uint32_t sym_num;
	int32_t sub_res;
    char* cur = strtok(buf, IGNORE_CHARS);
    cur = strtok(NULL, IGNORE_CHARS);//cur = const_expr
    if(cur == NULL){
        write_to_log("Error: no expression in ORG directive.\n");
        return;
    }
	remove_spaces(cur);
    org_expr = calculate_data_expression(cur, 0, ret, sym_num, symtbl, sub_res);
	if (sub_res != 0) {
		write_to_log("Error: expression %s is not regular\n", cur);
		return;
	}
}



/* There are two possible sizes for the instruction
    1. Double word (Adressing modes:
                    1.register "reg" 0
                    2. register indirect "regi" 2)
    2. Two double words(Adressing modes:
                    1.immediate "imm" 4
                    2.memory direct "mem" 6
                    3.register indirect with an offset "regio" 7)
*/
void location_counter_update(char** args, int num_args, uint32_t& location_counter){
    int two = 0;
    for(int i = 0; i < num_args; i++){
        int mode = addressing_mode(args[i]);
        if(mode >=4 && mode < 8 || mode ==1){
                two = 1;
        }
    }
	if (two) {
		location_counter += 8;
	}
	else {
		location_counter += 4;
	}
    
    
}

void write_wout_label(char* buf, FILE* output) {
	char* cur = strdup(buf);
	char* start = strtok(cur, IGNORE_CHARS);
	size_t len = strlen(start);
	if (start[len - 1] != ':') {
		fprintf(output, buf);
	}
	else {
		cur = strtok(NULL, IGNORE_CHARS);
		fprintf(output, buf + (cur-start));
	}
	fprintf(output, "\n");
}

void translate_directive_pass_one(const char* name, char* buf, FILE* output, vector<vector<Symbol*>>& symtbl, uint32_t& location_counter){
    char* str = strdup(name);
	skip_comment(buf);
	int len = strlen(buf);
	buf[len - 1] = '\n';
	char* p = strrchr(str, '.');
    if(p && p != str){
        *p = '\0';
    }
    if(strcmp(str, ".global") == 0){ 
		global_directive(buf, symtbl, location_counter);}
    else if(strcmp(str, ".text") == 0){ 
		section_directive(name, symtbl, location_counter); fprintf(output,buf); } 
    else if(strcmp(str, ".data") == 0){
		section_directive(name, symtbl, location_counter); fprintf(output,buf); }
    else if(strcmp(str, ".bss") == 0){ 
		section_directive(name, symtbl, location_counter); fprintf(output,buf); }
    else if(strcmp(str, ".rodata") == 0){
		section_directive(name, symtbl, location_counter); fprintf(output,buf); }
    else if(strcmp(str, "org") == 0){ 
		fprintf(output, buf); }
	else if (strcmp(str, "db") == 0) { 
		data_directive(buf, symtbl, location_counter); write_wout_label(buf, output);}
    else if(strcmp(str, "dw") == 0){ 
		data_directive(buf, symtbl, location_counter); write_wout_label(buf, output);}
    else if(strcmp(str, "dd") == 0){ 
		data_directive(buf, symtbl, location_counter); write_wout_label(buf, output);}
	free(str);
}
void translate_directive_pass_two(const char* name, char* buf, vector<vector<Symbol*>>& symtbl, vector<RelTable*>& reltbls, uint32_t& location_counter, int& rel_ind, char* prev_name){
    if(name[0] == '.'){
		rel_ind++; 
		Symbol* sym = get_symbol(symtbl, name);
		if (prev_name != NULL) {
			if (strcmp(prev_name, "org") == 0) {
				sym->flags.push_back('F');
				sym->addr = org_expr;
				for (int i = 0; i < NUM_BUCKETS - 1; i++) {
					int sz = symtbl[i].size();
					for (int j = 0; j < sz; j++) {
						if (symtbl[i][j]->sec_num == sym->num) {
							symtbl[i][j]->addr += org_expr;
						}
					}
				}
			}
		}
		if (sym != NULL) {
			seg_addr = sym->addr;
		}
		current_sec = sym->num;
		location_counter = 0;
	}
    else if(strcmp(name, "db") == 0){
		data_pass_two(name, buf, reltbls, location_counter, rel_ind, symtbl);}
    else if(strcmp(name, "dw") == 0 ){
		data_pass_two(name, buf, reltbls, location_counter, rel_ind, symtbl);}
    else if(strcmp(name, "dd") == 0){
		data_pass_two(name, buf, reltbls, location_counter, rel_ind, symtbl);}
	else if (strcmp(name, "org") == 0) {
		org_directive(buf, symtbl); }
}

int translate_inst(const char* name, char** args, int num_args, vector<vector<Symbol*>>& symtbl, vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter){
    if(strcmp(name, "add") == 0){return trans_ar_log(0x30, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "sub") == 0){return trans_ar_log(0x31, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "mul") == 0){return trans_ar_log(0x32, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "div") == 0){return trans_ar_log(0x33, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "mod") == 0){return trans_ar_log(0x34, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "and") == 0){return trans_ar_log(0x35, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "or") == 0){ return trans_ar_log(0x36, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "xor") == 0){return trans_ar_log(0x37, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "not") == 0){return trans_not(0x38, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "asl") == 0){return trans_ar_log(0x39, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "asr") == 0){return trans_ar_log(0x3A, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "push") == 0){return trans_stack(0x20, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "pop") == 0){return trans_stack(0x21, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "load") == 0 || 
        strcmp(name, "loadub") == 0 || 
        strcmp(name, "loadsb") == 0 || 
        strcmp(name, "loaduw") == 0 || 
        strcmp(name, "loadsw") == 0){return trans_load(0x10, name, args, num_args, symtbl, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "store") == 0 ||
        strcmp(name, "storeb") == 0 || 
        strcmp(name, "storew") == 0){return trans_store(0x11, name, args, num_args, symtbl, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "int") == 0){return trans_int(0x00, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "jmp") == 0){return trans_uncond_branch(0x02, args, num_args, symtbl, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "call") == 0){return trans_uncond_branch(0x03, args, num_args, symtbl, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "ret") == 0){return trans_ret(0x01, args, num_args, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "jz") == 0){return trans_cond_branch(0x04, args, num_args, symtbl, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "jnz") == 0){return trans_cond_branch(0x05, args, num_args, symtbl, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "jgz") == 0){return trans_cond_branch(0x06, args, num_args, symtbl, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "jgez") == 0){return trans_cond_branch(0x07, args, num_args, symtbl, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "jlz") == 0){return trans_cond_branch(0x08, args, num_args, symtbl, reltbls, rel_ind, location_counter);}
    else if(strcmp(name, "jlez") == 0){return trans_cond_branch(0x09, args, num_args, symtbl, reltbls, rel_ind, location_counter);}
    else return -1;

}

int trans_ar_log(uint8_t opcode, char** args, int num_args, vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter) {
  if (num_args != 3) 
    return -1;
  int rt = translate_reg(args[0]);
  int rs1 = translate_reg(args[1]);
  int rs2 = translate_reg(args[2]);
  if (rs1 == -1 || rs2 == -1 || rt == -1) {
	  write_to_log("Error: register doesn't exist.\n");
	  return -1;
  }
  uint32_t instruction = (opcode<<24) | (rt<<16) | (rs1<<11) | (rs2<<6);
  write_hex_inst(instruction, reltbls[rel_ind]);
  location_counter+=4;
  return 0;
}

int trans_not(uint8_t opcode, char** args, int num_args, vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter) {
  if (num_args != 2) 
    return -1;
  int rt = translate_reg(args[0]);
  int rs = translate_reg(args[1]);
  if (rs == -1 || rt == -1) {
	  write_to_log("Error: register doesn't exist.\n");
	  return -1;
  }
  uint32_t instruction = (opcode<<24) | (rt<<16) | (rs<<11);
  write_hex_inst(instruction, reltbls[rel_ind]);
  location_counter+=4;
  return 0;
}

int trans_stack(uint8_t opcode, char** args, int num_args, vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter) {
  if (num_args != 1) 
    return -1;
  int rt = translate_reg(args[0]);
  
  if (rt == -1) {
	  write_to_log("Error: register doesn't exist.\n");
	  return -1;
  }
  uint32_t instruction = (opcode<<24) | (rt<<16);
  write_hex_inst(instruction, reltbls[rel_ind]);
  location_counter+=4;
  return 0;
}

int trans_int(uint8_t opcode, char** args, int num_args, vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter) {
    if (num_args != 1) 
    return -1;
    int rt = translate_reg(args[0]);
  
	if (rt == -1) {
		write_to_log("Error: register doesn't exist.\n");
		return -1;
	}
    uint32_t instruction = (opcode<<24) | (rt<<16);
    write_hex_inst(instruction, reltbls[rel_ind]);
    location_counter+=4;
    return 0;
}

int trans_ret(uint8_t opcode, char** args, int num_args, vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter){
    if(num_args != 0){
        return -1;
    }
    uint32_t instruction = opcode<<24;
    write_hex_inst(instruction, reltbls[rel_ind]);
    location_counter+=4;
    return 0;
}

/* There are two possible sizes for the instruction
    1. Double word (Adressing modes:
                    1.register "reg" 0
                    2. register indirect "regi" 2)
    2. Two double words(Adressing modes:
                    1.immediate "imm" 4
                    2.memory direct "mem" 6
                    3.register indirect with an offset "regio" 7)
*/
void remove_brackets(char* source){
    char* i = source;
    char* j = source;
    while(*j){
        *i = *j++;
        if(*i != '[' && *i != ']')
            i++;
    }
    *i = '\0';
}

int32_t check_sub(char* expr, vector<Symbol*>& symbols, bool& ret, uint32_t& sym_num){
    stack<Symbol*> s;
    Symbol* res;
	ret = false;
    char op;
	if (strlen(expr) == 1) {
		Symbol* sym = symbols[c_to_ind(expr[0])];
		if (sym->sec_num == -1) {
			if (sym->flag == 'C') {
				delete sym;
			}
			return 0;
		}
		else {
			if (sym->flag == 'G') {
				sym_num = sym->num;
			}
			else {
				sym_num = sym->sec_num;
			}
		}
		return 1;
	}
	bool global = false;
    while(*expr != NULL){
        char x = *expr;
        if(is_operand(x)){
            s.push(symbols[c_to_ind(x)]);
        }else if(is_operator(x)){
            Symbol* sym2 = s.top();
            s.pop();
            Symbol* sym1 = s.top();
            s.pop();
            int32_t oper1;
            int32_t oper2;
            if(sym2->sec_num == -1){
                oper2 = 0;
            }else if(sym2->sec_num > 0){
                oper2 = 1;
				if (sym2->flag == 'G') {
					if(!global)
						global = true;
					else {
						ret = true;
						write_to_log("Error: more than one global symbol.\n");
						return 0;
					}
					sym_num = sym2->num;
				}
				else {
					if(!global)
						sym_num = sym2->sec_num;
				}
            }else{
                oper2 = sym2->addr;
            }
            if(sym1->sec_num == -1){
                oper1 = 0;
            }else if (sym1->sec_num > 0){
                oper1 = 1;
				if (sym1->flag == 'G') {
					if (!global)
						global = true;
					else {
						ret = true;
						write_to_log("Error: more than one global symbol.\n");
						return 0;
					}
					sym_num = sym1->num;
				}
				else {
					if (!global)
						sym_num = sym1->sec_num;
				}
            }else{
                oper1 = sym1->addr;
            }
			if (sym1->sec_num && sym2->sec_num && (x == '-' || x == '+') || sym1->sec_num == -1 || sym2->sec_num == -1 || sym1->flag == 'G' || sym2->flag == 'G') {
				res = new (nothrow) Symbol("", bin_op(oper1, oper2, x), -1);
			}
			else {
				write_to_log("Error: expression is irregular.\n", expr);
				ret = true;
				return 0;
			}
                
			if (sym2->flag == 'C') {
				delete sym2;
			}
			if (sym1->flag == 'C') {
				delete sym1;
			}
            s.push(res);
        }
        expr++;
    }
    res = s.top();
    s.pop();
    if(s.empty()){
        ret = false;
		return res->addr;
    }else{
        ret = true;
        write_to_log("Error: expression is irregular.\n", expr);
        
    }
    return 0;
}

int32_t resolve_addr_mode(char* arg, vector<int32_t>& literals, int32_t tmp,bool& ret, uint32_t& sym_num, vector<vector<Symbol*>>& symtbl, int32_t& sub_res){
    char* out = strdup(arg);
    int i = 0; //operand counter
    int str_ind = 0;
    bool cur_operand = false; //boolean
    char* str = arg;
    char literal[35];
    int literal_ind = 0;
	char prev_operator = '?';
    vector<Symbol*> sub_literals;
    char* sub_str = strdup(arg);
    int j = 0;
    int sub_str_ind = 0;
	char c;
    while(*str){
        c = *str;
        if(str_contains("+-*/()",c)){
            if(cur_operand){
                cur_operand = false;
                //exchange literal with a letter
                char ins = ind_to_c(i++);
                out[str_ind++] = ins;
                literal[literal_ind] = '\0';
				char sub_ins = ind_to_c(j++);
				if (prev_operator == '+' || prev_operator == '-') {
					sub_str[sub_str_ind++] = prev_operator;
				}
				else if (prev_operator != '?') {
					ret = true;
					//write_to_log ...
					return 0;
				}
				sub_str[sub_str_ind++] = sub_ins;
                if(is_valid_label(literal)){
                    Symbol* sym = get_symbol(symtbl, literal);
                    if(sym == NULL){//must be an external symbol
                    //exchange it with 0
                        add_to_table(symtbl, literal, 0, &sym);
                        literals.push_back(0);
                    }else{
                        int32_t off = tmp;
                        
						if (sym->flag == 'L' || sym->sec_num == -1) {
							off += sym->addr;
						}
                        literals.push_back(off);
                    }            
					sub_literals.push_back(sym);
                }else{
					sub_literals.push_back(new Symbol("", 0, -1));
                    literals.push_back(convert_to_num(literal));
                }
            }
			prev_operator = c;
            out[str_ind++] = c;
        }else{
            if(!cur_operand){
                cur_operand = true;
                literal_ind = 0;
            }
            literal[literal_ind++] = c;
        }
        str++;
    }
	if (cur_operand) {
		//exchange literal with a letter
		char ins = ind_to_c(i++);
		out[str_ind++] = ins;
		literal[literal_ind] = '\0';
		char sub_ins = ind_to_c(j++);
		if (prev_operator == '+' || prev_operator == '-') {
			sub_str[sub_str_ind++] = prev_operator;
		}
		else if (prev_operator != '?') {
			ret = true;
			//write to log...
			return 0;
		}
		sub_str[sub_str_ind++] = sub_ins;
		if (is_valid_label(literal)) {
			Symbol* sym = get_symbol(symtbl, literal);
			
			if (sym == NULL) {//must be an external symbol
							  //exchange it with 0
				add_to_table(symtbl, literal, 0, &sym);
				literals.push_back(0);
			}
			else {
				int32_t off = tmp;

				if (sym->flag == 'L' || sym->sec_num == -1) {
					off += sym->addr;
				}
				literals.push_back(off);
			}
			sub_literals.push_back(sym);
		}
		else {
			sub_literals.push_back(new Symbol("", 0, -1));
			literals.push_back(convert_to_num(literal));
		}
	}
	out[str_ind++] = '\0';
	sub_str[sub_str_ind++] = '\0';
	//check if the subexpression is valid
	char* sub_ex = to_postfix(sub_str);
	bool err;
	sub_res = check_sub(sub_ex, sub_literals, err, sym_num);
	if (err) {
		ret = true;
		return 0;
	}
	if (sub_res == 1) {
		ret = false;
	}
	else if (sub_res == 0) {
		ret = false;
	}
	else {
		ret = true;
		return 0;
	}
    char* ex = to_postfix(out);
    return eval_postfix(ex, literals);
}

int32_t resolve_addr_mode(char* arg, vector<int32_t>& literals, int32_t tmp,bool& ret, uint32_t& sym_num, int& r, vector<vector<Symbol*>>& symtbl, int32_t& sub_res){
    char* out = strdup(arg);
    int i = 0; //operand counter
    int str_ind = 0;
    bool cur_operand = false; //boolean
    char* str = arg;
    char literal[35];
    int literal_ind = 0;
	char prev_operator = '?';
    vector<Symbol*> sub_literals;
    char* sub_str = strdup(arg);
    int j = 0;
    int sub_str_ind = 0;
	char c;
    while(*str){
        c = *str;
        if(str_contains("+-*/()",c)){
            if(cur_operand){
                cur_operand = false;
                //exchange literal with a letter
                char ins = ind_to_c(i++);
                out[str_ind++] = ins;
                literal[literal_ind] = '\0';
				char sub_ins = ind_to_c(j++);
				if (prev_operator == '+' || prev_operator == '-') {
					sub_str[sub_str_ind++] = prev_operator;
				}
				else if (prev_operator != '?') {
					ret = true;
					//write to log...
					return 0;
				}
				sub_str[sub_str_ind++] = sub_ins;
                if(is_valid_label(literal)){
                    Symbol* sym = get_symbol(symtbl, literal);
                    
                    if(sym == NULL){//must be an external symbol
                    //exchange it with 0
                        r = translate_reg(literal);
                        if(r == -1){
                            add_to_table(symtbl, literal, 0, &sym);
                           
						}
						sym = new Symbol("", 0, -1);
						literals.push_back(0);
                    }else{
                        int32_t off = tmp;
                        
						if (sym->flag == 'L' ||sym->sec_num == -1) {
							off += sym->addr;
						}
                        literals.push_back(off);
                    }       
					sub_literals.push_back(sym);
                }else{
					sub_literals.push_back(new Symbol("", 0, -1));
                    literals.push_back(convert_to_num(literal));
                }
            }
			prev_operator = c;
            out[str_ind++] = c;
        }else{
            if(!cur_operand){
                cur_operand = true;
                literal_ind = 0;
            }
            literal[literal_ind++] = c;
        }
        str++;
    }
    if(r == -1){
		write_to_log("Error: register doesn't exist and it should.\n");
        ret = true;
        return 0;
    }
	if (cur_operand) {
		//exchange literal with a letter
		char ins = ind_to_c(i++);
		out[str_ind++] = ins;
		literal[literal_ind] = '\0';
		char sub_ins = ind_to_c(j++);
		if (prev_operator == '+' || prev_operator == '-') {
			sub_str[sub_str_ind++] = prev_operator;
		}
		else if (prev_operator != '?') {
			ret = true;
			//write to log...
			return 0;
		}
		sub_str[sub_str_ind++] = sub_ins;
		if (is_valid_label(literal)) {
			Symbol* sym = get_symbol(symtbl, literal);
			
			if (sym == NULL) {//must be an external symbol
							  //exchange it with 0
				add_to_table(symtbl, literal, 0, &sym);
				literals.push_back(0);
			}
			else {
				int32_t off = tmp;

				if (sym->flag == 'L' ||sym->sec_num == -1) {
					off += sym->addr;
				}
				literals.push_back(off);
			}
			sub_literals.push_back(sym);
		}
		else {
			sub_literals.push_back(new Symbol("", 0, -1));
			literals.push_back(convert_to_num(literal));
		}
	}
	out[str_ind++] = '\0';
	sub_str[sub_str_ind++] = '\0';
	//check if the subexpression is valid
	char* sub_ex = to_postfix(sub_str);
	bool err;
	sub_res = check_sub(sub_ex, sub_literals, err, sym_num);
	if (err) {
		ret = true;
		return 0;
	}
	if (sub_res == 1) {
		ret = false;
	}
	else if (sub_res == 0) {
		ret = false;
	}
	else {
		ret = true;
		return 0;
	}
    char* ex = to_postfix(out);
    return eval_postfix(ex, literals);
}

int trans_uncond_branch(uint8_t opcode, char** args, int num_args, vector<vector<Symbol*>>& symtbl, vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter){
    int addr_mode = addressing_mode(args[0]);
    uint32_t instruction1;
    uint32_t instruction2;
    vector<int32_t> literals;
    uint32_t sym_num;
	int32_t sub_res;
    bool ret;
    /*  args[0] can use following address modes:
        2. register indirect
        6. memory direct 
        7. register indirect with offset
        1. $address(pc indirect with offset) 0b111
    */
    int rt;
    int r;
    location_counter+=4;
    switch(addr_mode){
        case 1:/*resolve the use of a symbol and depending if it is global or not
        build in corresponding value in the second dword of the instruction
        and create a relative relocation table entry

        if the symbol is not used then just build in the appropriate value
        depending on the location of this section the offset/immediate/address value will change*/
            instruction2 = resolve_addr_mode(args[0] + 1, literals, -4, ret, sym_num, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'R', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (7<<21) | (translate_reg("pc")<<16);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        case 2://the simplest case
            remove_brackets(args[0]);
            rt = translate_reg(args[0]);
            if(rt == -1){
				write_to_log("Error: register doesn't exist.\n");
                return -1;
            }
            instruction1 = (opcode<<23) | (addr_mode<<20) | (rt<<15);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            break;
        case 6://resolve the possible use of difference of two symbols
            instruction2 = resolve_addr_mode(args[0], literals, 0, ret, sym_num, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (addr_mode<<21);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        case 7://resolve the possible use of difference of two symbols
                // find register
            remove_brackets(args[0]);
            instruction2 = resolve_addr_mode(args[0], literals, 0, ret, sym_num, r, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (addr_mode<<21) | (r<<16);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        default:
            return -1;
    }
    return 0;
}

int trans_cond_branch(uint8_t opcode, char** args, int num_args, vector<vector<Symbol*>>& symtbl, vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter){
	if (num_args == 1) {
		write_to_log("Error: to few arguments.\n");
		return -1;
	}
	int addr_mode = addressing_mode(args[1]);
    uint32_t instruction1;
    uint32_t instruction2;
    vector<int32_t> literals;
	int32_t sub_res;
    uint32_t sym_num;
    bool ret;
    int rt;
    int r;
    int r1 = translate_reg(args[0]);
    if(r1 == -1){
		write_to_log("Error: register doesn't exist.\n");
        return -1;
    }
    /*  args[1] can use following address modes:
        2. register indirect 0b010 done
        4. memory direct 0b110
        5. register indirect with offset 0b111
        1. $address
    */
    location_counter+=4;
    switch(addr_mode){
        case 1:/*resolve the use of a symbol and depending if it is global or not
                build in corresponding value in the second dword of the instruction
                and create a relative relocation table entry

                if the symbol is not used then just build in the appropriate value
                depending on the location of this section the offset/immediate/address value will change*/
            instruction2 = resolve_addr_mode(args[1] + 1, literals, -4, ret, sym_num, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'R', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (7<<21) | (translate_reg("pc")<<16) | (r1 << 11);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        case 2://the simplest case
            remove_brackets(args[1]);
            rt = translate_reg(args[1]);
            if(rt == -1){
				write_to_log("Error: register doesn't exist.\n");

                return -1;
            }
            instruction1 = (opcode<<24) | (addr_mode<<21) | (rt<<16) | (r1<<11);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            break;
        case 6://resolve the possible use of difference of two symbols
            instruction2 = resolve_addr_mode(args[1], literals, 0, ret, sym_num, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (addr_mode<<21) | (r1 << 11);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        case 7://resolve the possible use of difference of two symbols
                // find register
            remove_brackets(args[1]);
            instruction2 = resolve_addr_mode(args[1], literals, 0, ret, sym_num, r, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (addr_mode<<21) | (r<<16) | (r1<<11);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        default:
            return -1;
    }
    return 0;
}

int trans_load(uint8_t opcode,const char* name, char** args, int num_args, vector<vector<Symbol*>>& symtbl, vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter){
    int addr_mode = addressing_mode(args[1]);
    uint32_t instruction1;
    uint32_t instruction2;
    vector<int32_t> literals;
	int32_t sub_res;
    uint32_t sym_num;
    bool ret;
    uint8_t type = 0;
    if(strstr(name, "ub")){
        type = 3;
    }else if(strstr(name, "sb")){
        type = 7;
    }else if(strstr(name, "uw")){
        type = 1;
    }else if(strstr(name, "sw")){
        type = 5;
    }
    int rs;
    int r;
    int rt = translate_reg(args[0]);
    if(rt == -1){
		write_to_log("Error: register doesn't exist.\n");

        return -1;
    }
    /* Load can deal with following addressing modes args[1]:
        0. register direct 0b000 done
        2. register indirect 0b010 done
        4. memory direct 0b110
        5. register indirect with offset 0b111
        3. immediate 0b100
        1. $address
    */
	location_counter += 4;
    switch(addr_mode){
        case 0:
            rs = translate_reg(args[1]);
            if(rs == -1){
				write_to_log("Error: register doesn't exist.\n");
                return -1;
            }
            instruction1 = (opcode<<24) | (addr_mode<<21) | (rs << 16) | (rt << 11) | (type<<3);
			write_hex_inst(instruction1, reltbls[rel_ind]);
            break;
        /*insert*/
        case 2://the simplest case
            remove_brackets(args[1]);
            rs = translate_reg(args[1]);
            if(rs == -1){
				write_to_log("Error: register doesn't exist.\n");
                return -1;
            }
            instruction1 = (opcode<<24) | (addr_mode<<21) | (rs<<16) | (rt << 11) | (type << 3);
            location_counter+=4;
            write_hex_inst(instruction1, reltbls[rel_ind]);
            break;
        case 1:/*resolve the use of a symbol and depending if it is global or not
        build in corresponding value in the second dword of the instruction
        and create a relative relocation table entry

        if the symbol is not used then just build in the appropriate value
        depending on the location of this section the offset/immediate/address value will change*/
            instruction2 = resolve_addr_mode(args[1] + 1, literals, -4, ret, sym_num, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'R', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (7<<21) | (translate_reg("pc")<<16) | (rt << 11) | (type << 3);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        case 6://resolve the possible use of difference of two symbols
            instruction2 = resolve_addr_mode(args[1], literals, 0, ret, sym_num, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (addr_mode<<21) | (rt << 11) | (type << 3);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        case 7://resolve the possible use of difference of two symbols
                // find register
            remove_brackets(args[1]);
            instruction2 = resolve_addr_mode(args[1], literals, 0, ret, sym_num, r, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (addr_mode<<21) | (r<<16) | (rt << 11) | (type << 3);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        case 3:
            instruction2 = resolve_addr_mode(args[1] + 1, literals, 0, ret, sym_num, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (addr_mode<<21) | (rt << 11) | (type << 3);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
		case 4:
			instruction2 = resolve_addr_mode(args[1] + 1, literals, 0, ret, sym_num, symtbl, sub_res);
			if (ret) {
				return -1;
			}
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
			}
			location_counter += 4;
			instruction1 = (opcode << 24) | (addr_mode << 21) | (rt << 11) | (type << 3);
			write_hex_inst(instruction1, reltbls[rel_ind]);
			write_hex_inst(instruction2, reltbls[rel_ind]);
			break;
        default:
            return -1;
    }
    return 0;
}

int trans_store(uint8_t opcode,const char* name, char** args, int num_args, vector<vector<Symbol*>>& symtbl, vector<RelTable*>& reltbls, int rel_ind, uint32_t& location_counter){
    int addr_mode = addressing_mode(args[1]);
    uint32_t instruction1;
    uint32_t instruction2;
    vector<int32_t> literals;
	int32_t sub_res;
    uint32_t sym_num;
    bool ret;
    uint8_t type = 0;
    if(strstr(name, "b")){
        type = 3;
    }else if(strstr(name, "w")){
        type = 1;
    }
    int rt;
    int r;
    int rs = translate_reg(args[0]);
    if(rs == -1){
		write_to_log("Error: register doesn't exist.\n");
        return -1;
    }
    /* Store can deal with following addressing modes:
        0. register direct 0b000 done
        2. register indirect 0b010 done
        4. memory direct 0b110
        5. register indirect with offset 0b111
        1. $address
    */
    location_counter+=4;
    switch(addr_mode){
        case 0: // regdir
            rt = translate_reg(args[1]);
            if(rt == -1){
				write_to_log("Error: register doesn't exist.\n");
                return -1;
            }
            instruction1 = (opcode<<24) | (addr_mode<<21) | (rt<<16) | (rs << 11) | (type << 3);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            break;
        case 2://regind
            remove_brackets(args[1]);
            rt = translate_reg(args[1]);
            if(rt == -1){
                return -1;
            }
            location_counter+=4;
            instruction1 = (opcode<<24) | (addr_mode<<21) | (rt<<16) | (rs << 11) | (type << 3);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            break;
        case 1://pc relative
            instruction2 = resolve_addr_mode(args[1] + 1, literals, -4, ret, sym_num, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'R', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (7<<21) | (translate_reg("pc")<<16) | (rs << 11) | (type << 3);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        case 6://memdir
            instruction2 = resolve_addr_mode(args[1], literals, 0, ret, sym_num, symtbl, sub_res);
            if(ret){
                return -1;
            }
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (addr_mode<<21) | (rs << 11) | (type << 3);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        case 7://regind + off
            remove_brackets(args[1]);
            instruction2 = resolve_addr_mode(args[1], literals, 0, ret, sym_num, r, symtbl, sub_res);
            if(ret){
                return -1;
			}
			if (sub_res == 1) {
				add_rel_symbol(*(reltbls[rel_ind]), seg_addr + location_counter, 'A', sym_num);
			}
            location_counter+=4;
            instruction1 = (opcode<<24) | (addr_mode<<21) | (r<<16) | (rs << 11) | (type << 3);
            write_hex_inst(instruction1, reltbls[rel_ind]);
            write_hex_inst(instruction2, reltbls[rel_ind]);
            break;
        default:
            return -1;
    }
    return 0;
}
