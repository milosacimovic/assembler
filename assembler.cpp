#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "src/utils.h"
#include "src/translate.h"
#include "src/translate_utils.h"
#include "src/tables.h"

#include "assembler.h"

using namespace std;

void name_already_exists(const char* name){
    write_to_log("Error: name %s already exists", name);
}

void invalid_label(uint32_t line, const char* label){
    write_to_log("Error: invalid label on line %u: %s", line, label);
}

void inst_error(uint32_t line, const char* name, char** args, uint8_t num_args){
    write_to_log("Error: on line %u:", line);
    log_inst(name, args, num_args);
}

void skip_comment(char* str) {
    char* comment_start = strchr(str, ';');
    if (comment_start) {
        *comment_start = '\0';
    }
}

void extra_arg_error(uint32_t line, const char* extra){
    write_to_log("Error: extra argument on line %u: %s", line, extra);
}
/* If the @str:
    1. is not a label, it returns -1
    2. is a label, but not valid it returns 1
    3. is a valid label, but adding it to symbol table was unsuccessful, it returns 1
    4. is a valid label and adding it to symbol table was successful, it returns 0
*/
int add_if_label(uint32_t line, char* str, uint32_t location_counter, vector<vector<Symbol*>>& symtbl){
    Symbol* in;
    uint32_t sz = strlen(str);
    if(str[sz - 1] == ':'){
        str[sz - 1] = '\0';
        if(is_valid_label(str)){
            Symbol* out = get_symbol(symtbl, str);
            if(out != NULL){
                if(out->sec_num == 0){
                    int num_secs = symtbl[NUM_BUCKETS - 1].size();
                    out->sec_num =  num_secs;
                    out->addr = location_counter;
                }else{
                    name_already_exists(str);//withing this section
                    //name clashing
                }
                
            }else{
                if(add_to_table(symtbl, str, location_counter, &in) == 0){
                    return 0;
                }
            }
        }else{
            invalid_label(line, str);
            return 1;
        }
    }else{
        return -1;
    }
}
/* First pass of the assembler.

   This function should read each line, strip all comments, scan for labels,
   and pass instructions to write_pass_one(). The input file may or may not
   be valid. Here are some guidelines:

    1. Only one label may be present per line. It must be the first token present.
        Once you see a label, regardless of whether it is a valid label or invalid
        label, treat the NEXT token as the beginning of an instruction.
    2. If the first token is not a label, treat it as the name of an instruction.
    3. Everything after the instruction name should be treated as arguments to
        that instruction. If there are more than MAX_ARGS arguments, call
        raise_extra_arg_error() and pass in the first extra argument. Do not 
        write that instruction to the output file (eg. don't call write_pass_one())
    4. Only one instruction should be present per line. You do not need to do 
        anything extra to detect this - it should be handled by guideline 3. 
    5. A line containing only a label is valid. The address of the label should
        be the byte offset of the next instruction, regardless of whether there
        is a next instruction or not.

   Just like in pass_two(), if the function encounters an error it should NOT
   exit, but process the entire file and return -1. If no errors were encountered, 
   it should return 0.*/
bool pass_one(FILE* input, FILE* output, vector<vector<Symbol*>>& symtbl){
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
        skip_comment(buf);
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
        }
        char* name = to_lower(cur);
        if(is_directive(name)){

            if(strcmp(name, ".end") == 0){
                symtbl[NUM_BUCKETS - 1][symtbl[NUM_BUCKETS- 1].size() - 1]->sec_size = location_counter;
                fprintf(output, "%s", name);
                return error;
            }
            translate_directive_pass_one(name, buf, output, symtbl, location_counter, prev_name);           
            
        }else{//it is an instruction mnemonic or a def directive
            cur = strtok(NULL, IGNORE_CHARS);
            char *def = to_lower(cur);
            if(strcmp(def, "def")){
                def_directive(buf, symtbl);
                line++;
                continue;
            }
            //Should I check the arguments for label usage ???????????
            //add a label as extern if it is not in symbol table
            //if it is then nothing
            while(cur != NULL){
                args[num_args++] = to_lower(cur);
                remove_spaces(args[num_args-1]);
                cur = strtok(NULL, ",\f\n\r\t\v");
                if(num_args > MAX_ARGS) {
                    error = true;
                    write = true;
                    extra_arg_error(line, cur);
                    break;
                }
            }
            write_pass_one(output, name, args, num_args);
            if(!write){
                location_counter_update(args, num_args, location_counter);   
            }
            
        }
        if(prev_name != NULL){
            free(prev_name);
        }
        prev_name = strdup(name);
        line++;
    }
    symtbl[NUM_BUCKETS - 1][symtbl[NUM_BUCKETS- 1].size() - 1]->sec_size = location_counter;
    fprintf(output, ".end");
    return error;
}
/* Reads an intermediate file and translates it into machine code. You may assume:
    1. The input file contains NO comments
    2. The input file contains NO labels
    3. The input file contains at maximum one instruction per line
    4. All instructions have at maximum MAX_ARGS arguments
    5. The symbol table has been filled out already

   If an error is reached, DO NOT EXIT the function. Keep translating the rest of
   the document, and at the end, return -1. Return 0 if no errors were encountered. */
int pass_two(FILE* input, FILE* output, vector<vector<Symbol*>>& symtbl, vector<RelTable*>& reltbls){
    char buffer[BUF_SIZE];//line buffer
    char* buf;
    int error = 0; //error notice
    uint32_t line = 0;
    uint32_t location_counter = 0;
    int rel_ind = -1;//index in relocation tables array
    char* args[MAX_ARGS];//args array
    while(fgets(buffer, BUF_SIZE, input) != NULL){
        buf = strdup(buffer);
        char * cur = strtok(buffer, IGNORE_CHARS);
        char mnemonic[10];
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
                translate_directive_pass_two(mnemonic, buf, symtbl, reltbls, location_counter, rel_ind);
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
        write_to_log("Error: upon opening file %s\n", input);
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
    
    if(input){
        if(open_files(&in_file_handle, &inter_file_handle, input, tmp) != 0){
            free_table(symtbl);
            exit(1);
        }

        if(pass_one(in_file_handle, out_file_handle, symtbl)){
            err = 1;
        }
        create_rel_tables(reltbls, symtbl);
        close_files(in_file_handle, inter_file_handle);
    }
    if(out){
        if(open_files(&inter_file_handle, &out_file_handle, tmp, out) != 0){
            free_table(symtbl);
            free_rel_tables(reltbls);
            exit(1);
        }
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
    }

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