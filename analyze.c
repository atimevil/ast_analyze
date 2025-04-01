#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 4096
#define MAX_FUNCTIONS 100
#define MAX_PARAMS 20
#define MAX_NAME_LENGTH 256

typedef struct {
    char name[MAX_NAME_LENGTH];
    char return_type[MAX_NAME_LENGTH];
    int param_count;
    char param_types[MAX_PARAMS][MAX_NAME_LENGTH];
    char param_names[MAX_PARAMS][MAX_NAME_LENGTH];
    int if_count;
} FunctionInfo;

FunctionInfo functions[MAX_FUNCTIONS];
int function_count = 0;

void analyze_ast(FILE *file);
void print_results();
void extract_value(const char *line, const char *key, char *value, size_t max_len);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ast.json>\n", argv[0]);
        return 1;
    }
    
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", argv[1]);
        return 1;
    }
    
    analyze_ast(file);
    fclose(file);
    
    print_results();
    
    return 0;
}

void extract_value(const char *line, const char *key, char *value, size_t max_len) {
    char *key_pos = strstr(line, key);
    if (!key_pos) return;
    
    char *value_start = key_pos + strlen(key);
    while (*value_start && (*value_start == ' ' || *value_start == ':' || *value_start == '"')) value_start++;
    
    char *value_end = value_start;
    while (*value_end && *value_end != '"' && *value_end != ',' && *value_end != '}' && *value_end != ']') value_end++;
    
    size_t len = value_end - value_start;
    if (len >= max_len) len = max_len - 1;
    
    strncpy(value, value_start, len);
    value[len] = '\0';
}

void analyze_ast(FILE *file) {
    char line[MAX_LINE];
    int brace_level = 0;
    int in_func_def = 0;
    int current_func = -1;
    int in_func_decl = 0;
    int in_param_list = 0;
    int current_param = -1;
    
    while (fgets(line, MAX_LINE, file)) {
        for (int i = 0; line[i]; i++) {
            if (line[i] == '{') brace_level++;
            if (line[i] == '}') {
                brace_level--;
                if (brace_level == 0 && in_func_def) {
                    in_func_def = 0;
                }
            }
        }
        
        if (strstr(line, "\"_nodetype\": \"FuncDef\"")) {
            in_func_def = 1;
            current_func = function_count++;
            memset(&functions[current_func], 0, sizeof(FunctionInfo));
            strcpy(functions[current_func].name, "unknown");
            strcpy(functions[current_func].return_type, "unknown");
        }
        
        if (in_func_def && strstr(line, "\"_nodetype\": \"FuncDecl\"")) {
            in_func_decl = 1;
        }
        
        if (in_func_def && strstr(line, "\"name\":")) {
            char name[MAX_NAME_LENGTH] = {0};
            extract_value(line, "\"name\":", name, MAX_NAME_LENGTH);
            
            if (name[0] != '\0' && strcmp(functions[current_func].name, "unknown") == 0) {
                strcpy(functions[current_func].name, name);
            }
        }
        
        if (in_func_def && in_func_decl && strstr(line, "\"names\": [")) {
            char type[MAX_NAME_LENGTH] = {0};
            extract_value(line, "\"names\": [", type, MAX_NAME_LENGTH);
            
            if (type[0] != '\0' && strcmp(functions[current_func].return_type, "unknown") == 0 && !in_param_list) {
                strcpy(functions[current_func].return_type, type);
            }
        }
        
        if (in_func_def && in_func_decl && strstr(line, "\"_nodetype\": \"ParamList\"")) {
            in_param_list = 1;
        }
        
        if (in_func_def && in_param_list && strstr(line, "\"_nodetype\": \"Decl\"")) {
            current_param = functions[current_func].param_count++;
            strcpy(functions[current_func].param_names[current_param], "unnamed");
            strcpy(functions[current_func].param_types[current_param], "unknown");
        }
        
        if (in_func_def && in_param_list && current_param >= 0 && strstr(line, "\"name\":")) {
            char name[MAX_NAME_LENGTH] = {0};
            extract_value(line, "\"name\":", name, MAX_NAME_LENGTH);
            
            if (name[0] != '\0') {
                strcpy(functions[current_func].param_names[current_param], name);
            }
        }
        
        if (in_func_def && in_param_list && current_param >= 0 && strstr(line, "\"names\": [")) {
            char type[MAX_NAME_LENGTH] = {0};
            extract_value(line, "\"names\": [", type, MAX_NAME_LENGTH);
            
            if (type[0] != '\0') {
                strcpy(functions[current_func].param_types[current_param], type);
            }
        }
        
        if (in_func_def && strstr(line, "\"_nodetype\": \"If\"")) {
            functions[current_func].if_count++;
        }
        
        if (in_param_list && strstr(line, "]")) {
            in_param_list = 0;
            current_param = -1;
        }
        
        if (in_func_decl && brace_level <= 1) {
            in_func_decl = 0;
        }
    }
}

void print_results() {
    printf("함수 개수: %d\n\n", function_count);
    
    for (int i = 0; i < function_count; i++) {
        printf("함수 이름: %s\n", functions[i].name);
        printf("리턴 타입: %s\n", functions[i].return_type);
        
        printf("파라미터: ");
        if (functions[i].param_count == 0) {
            printf("없음\n");
        } else {
            printf("\n");
            for (int j = 0; j < functions[i].param_count; j++) {
                printf("  - 타입: %s, 변수명: %s\n", 
                       functions[i].param_types[j], functions[i].param_names[j]);
            }
        }
        
        printf("if 조건문 개수: %d\n\n", functions[i].if_count);
    }
}
