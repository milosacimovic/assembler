#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "utils.h"
#include "translate.h"
#include "translate_utils.h"
#include "tables.h"

#include "assembler.h"


const uint8_t MAX_ARGS = 3;
const uint16_t BUF_SIZE = 1024;
const char* IGNORE_CHARS = " \f\n\r\t\v,";
const int NUM_BUCKETS = 54;

using namespace std;


bool pass_one(FILE* input, FILE* output, vector<vector<Symbol*>>& symtbl, TNSymbol** TNS){
    char buffer[BUF_SIZE];
    char* buf;
    uint32_t location_counter = 0;
    uint32_t line = 1;
    bool error = false;
    char* prev_name = NULL;
    while(fgets(buffer, BUF_SIZE, input) != NULL){
        buf = strdup(buffer);
        char* args[MAX_ARGS + 1];
        uint8_t num_args = 0;
        int write = 0;
        skip_comment(buffer);
        if(strcmp(buffer, "") == 0){
            line++;
            continue;
        }
        char* cur = strtok(buffer, IGNORE_CHARS);
        if(cur == NULL){
            line++;
            continue;
        }

        int ret = add_if_label(line, cur, location_counter, symtbl);
        // if the first token is a label or an invalid label then
        // check if the next token
        // is a directive or a mnemonic and take appropriate action
        // if the first token is not a label then check if the
        // current token is a directive or a mnemonic
        if(ret == 0 || ret == 1){
            cur = strtok(NULL, IGNORE_CHARS);
			if (cur == NULL) {
				line++;
				continue;
			}
        }
        char* name = to_lower(cur);
        if(is_directive(name)){

            if(strcmp(name, ".end") == 0){
                symtbl[NUM_BUCKETS - 1][symtbl[NUM_BUCKETS- 1].size() - 1]->sec_size = location_counter;
                fprintf(output, "%s", name);
                return error;
            }
            translate_directive_pass_one(name, buf, output, symtbl, location_counter);

        }else{//it is an instruction mnemonic or a def directive
            cur = strtok(NULL, IGNORE_CHARS);
            char *def = cur;
			if (cur != NULL) {
				if (strcmp(def, "def") == 0 || strcmp(def, "DEF") == 0) {
					def_directive(buf, symtbl, TNS);
					line++;
					continue;
				}
			}
            while(cur != NULL){
                args[num_args++] = cur;
                remove_spaces(args[num_args-1]);
                cur = strtok(NULL, ",\f\n\r\t\v");
                if(num_args > MAX_ARGS) {
                    error = true;
                    write = true;
					if (cur != NULL) {
						extra_arg_error(line, cur);

					}
					else {
						extra_arg_error(line, args[num_args - 1]);

					}
                    break;
                }
            }

            if(!write){
				write_pass_one(output, name, args, num_args);
                location_counter_update(args, num_args, location_counter);
            }

        }
		free(name);
        line++;
    }
    symtbl[NUM_BUCKETS - 1][symtbl[NUM_BUCKETS- 1].size() - 1]->sec_size = location_counter - symtbl[NUM_BUCKETS - 1][symtbl[NUM_BUCKETS - 1].size() - 1]->addr;
    fprintf(output, ".end");
    return error;
}
/* Reads an intermediate file and translates it into machine code. Some assumptions:
    1. The input file contains NO comments
    2. The input file contains NO labels
    3. The input file contains at maximum one instruction per line
    4. All instructions have maximum MAX_ARGS arguments
    5. The symbol table has been filled out

   If an error is reached, NO EXIT the function. Keep translating the rest of
   the document, and at the end, return -1. Return 0 if no errors. */
int pass_two(FILE* input, FILE* output, vector<vector<Symbol*>>& symtbl, vector<RelTable*>& reltbls){
    char buffer[BUF_SIZE];//line buffer
    char* buf;
    int error = 0; //error notice
    uint32_t line = 0;
    uint32_t location_counter = 0;
    int rel_ind = -1;//index in relocation tables array
    char* args[MAX_ARGS];//args array
	char* prev_name = NULL;
    while(fgets(buffer, BUF_SIZE, input) != NULL){
        buf = strdup(buffer);
        char * cur = strtok(buffer, IGNORE_CHARS);
		if (cur == NULL) {
			line++;
			continue;
		}
		char* mnemonic = to_lower(cur);
        if(is_directive(mnemonic)){
            if(strcmp(mnemonic, ".end") == 0){
                return error;
            }
            if(is_pass_two_directive(mnemonic)){
                /* Translate_inst needs to know which section the current
                    instruction is in and then make a decision
                    what section to insert its content into
                    it is most convenient to pass it an index
                    to relocation tables array
                */
                translate_directive_pass_two(mnemonic, buf, symtbl, reltbls, location_counter, rel_ind, prev_name);
            }

        }else{
            cur = strtok(NULL, IGNORE_CHARS);
            uint8_t num_args = 0;
            while(cur != NULL){
                args[num_args++] = cur;
                cur = strtok(NULL, IGNORE_CHARS);
            }

            int ret = translate_inst(mnemonic, args, num_args, symtbl, reltbls, rel_ind, location_counter);
            if(ret){
                inst_error(line+1, mnemonic, args, num_args);
                error = 1;
            }
        }
		if (prev_name != NULL) {
			free(prev_name);
		}
		prev_name = strdup(mnemonic);
		free(mnemonic);
        line++;
    }

    if(error){
        return -1;
    }

    return 0;
}

static int open_files(FILE** input, FILE** output, const char* in_name, const char* out_name){

    *input = fopen(in_name, "r");
    *output = fopen(out_name, "w");

    if(*input == NULL){
        write_to_log("Error: upon opening file %s\n", in_name);
        return -1;
    }
    if(*output == NULL){
        write_to_log("Error: upon opening file %s\n", out_name);
        return -1;
    }
    return 0;
}

static void close_files(FILE* input, FILE* output){
    fclose(input);
    fclose(output);
}

int assemble(const char* input, const char* tmp, const char* out){
    int err = 0;
    FILE* in_file_handle;
    FILE* inter_file_handle;
    FILE* out_file_handle;
    vector<vector<Symbol*>> symtbl(NUM_BUCKETS);
    vector<RelTable*> reltbls;
	TNSymbol* TNS = NULL;

    if(open_files(&in_file_handle, &inter_file_handle, input, tmp) != 0){
        free_table(symtbl);
        exit(1);
    }

    if(pass_one(in_file_handle, inter_file_handle, symtbl, &TNS)){
        err = 1;
    }
    create_rel_tables(reltbls, symtbl);
    close_files(in_file_handle, inter_file_handle);

    if(open_files(&inter_file_handle, &out_file_handle, tmp, out) != 0){
        free_table(symtbl);
        free_rel_tables(reltbls);
        exit(1);
    }
	resolve_TNS(&TNS, symtbl);

    assign_ranks(symtbl);
    //leave the checking if there are any undefined symbols
    //to the emulator
    if(pass_two(inter_file_handle, out_file_handle, symtbl, reltbls) != 0){
        err = 2;
    }

    write_table(symtbl, out_file_handle);

    //reltbl now has relocation tables and contents of corresponding sections
    write_out(out_file_handle, symtbl, reltbls);


    fprintf(out_file_handle, "#end");
    close_files(inter_file_handle, out_file_handle);


    free_table(symtbl);
    free_rel_tables(reltbls);

    return err;

}


void print_usage(){
    printf("Usage:\n\tassembler input_file\n");
    printf("or\n\tassembler input_file -log log_file\n");
}

int main(int argc, char* argv[]){

    if(argc < 2 || argc == 3 || argc >=5){
        print_usage();
        exit(1);
    }

    int in_file_len = strlen(argv[1]);
    char* in_file = strdup(argv[1]);

    char* inter_file = (char*)malloc(in_file_len*sizeof(char)+3);
	strcpy(inter_file, in_file);
    char* out_file = strdup(argv[1]);
    inter_file[in_file_len - 1] = 'i';
    inter_file[in_file_len] = 'n';
    inter_file[in_file_len + 1] = 't';
    inter_file[in_file_len + 2] = '\0';

    out_file[in_file_len - 1] = 'o';

    if(argc == 4){
        if(strcmp(argv[2], "-log") == 0){
            set_log_file(argv[3]);
        }else{
            print_usage();
            exit(1);
        }
    }

    int err = assemble(in_file, inter_file, out_file);
    if(err){
        write_to_log("One or more errors happend during assembly.\n");
    }else{
        write_to_log("Assembly of the source file completed successfully.\n");
    }
    if(is_log_file_set()){
        printf("Results saved to file: %s\n", argv[3]);
    }

    return err;
}
