#include <cctype>
#include <cstring>
#include <cstdlib>

#include "utils.h"

using namespace std;

static const char* output_file = NULL;

void alloc_fail(){
    write_to_log("Error: allocation failed.\n");
    exit(2);
}

int is_log_file_set() {
    return output_file != NULL;
}

void set_log_file(const char* filename) {
    if (filename) {
        output_file = filename;
    } else {
        output_file = NULL;
    }
}

void write_to_log(char* fmt, ...) {
    va_list args;

    if (output_file) {
        FILE* f = fopen(output_file, "a");
        if (!f) {
            return;
        }
        
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fclose(f);
    } else {
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
}

void log_inst(const char* name, char** args, uint8_t num_args) {
    if (output_file) {
        FILE* f = fopen(output_file, "a");
        if (!f) {
            return;
        }
        
        fprintf(f, "%s", name);
        for (int i = 0; i < num_args; i++) {
            fprintf(f, " %s", args[i]);
        }
        fprintf(f, "\n");
        fclose(f);
    } else {
        fprintf(stderr, "%s", name);
        for (int i = 0; i < num_args; i++) {
            fprintf(stderr, " %s", args[i]);
        }
        fprintf(stderr, "\n");
    }
}

char* to_lower(const char* str){
    char* cpy = strdup(str);
    int len = strlen(cpy);
    for(int i = 0; i < len; i++){
        cpy[i] = tolower(cpy[i]);
    }
    return cpy;
}


bool str_contains(const char* str, char c){
    size_t sz = strlen(str);
    for(int i = 0; i < sz; i++){
        if(str[i] == c){
            return true;
        }
    }
    return false;
}

bool char_vector_contains(vector<char>& str, char c){
    size_t sz = str.size();
    for(int i = 0; i < sz; i++){
        if(str[i] == c){
            return true;
        }
    }
    return false;
}