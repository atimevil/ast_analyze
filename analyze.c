#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 2048
#define MAX_NAME_LENGTH 256

typedef struct {
    char name[MAX_NAME_LENGTH];        
    char return_type[MAX_NAME_LENGTH];   
    int param_count;                    
    char param_types[20][MAX_NAME_LENGTH]; 
    char param_names[20][MAX_NAME_LENGTH]; 
    int if_count;                      
} FunctionInfo;

FunctionInfo* functions = NULL;
int function_count = 0;

void analyze_ast_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "파일을 열 수 없습니다: %s\n", filename);
        return;
    }
    
    char line[MAX_LINE_LENGTH];
    int func_count = 0;
    
    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        if (strstr(line, "\"_nodetype\": \"FuncDef\"")) {
            func_count++;
        }
    }
    
    functions = (FunctionInfo*)calloc(func_count, sizeof(FunctionInfo));
    function_count = func_count;
    
    rewind(fp);
    
    int current_func = -1;
    bool in_func_def = false;
    bool in_func_body = false;
    bool in_params = false;
    int brace_level = 0;
    int param_idx = -1;
    
    while (fgets(line, MAX_LINE_LENGTH, fp)) {
        if (strstr(line, "\"_nodetype\": \"FuncDef\"")) {
            current_func++;
            in_func_def = true;
            in_func_body = false;
            in_params = false;
            brace_level = 0;
            param_idx = -1;
            
            functions[current_func].param_count = 0;
            functions[current_func].if_count = 0;
        }
        
        if (in_func_def && strstr(line, "\"name\": \"")) {
            char* start = strstr(line, "\"name\": \"") + 9;
            char* end = strchr(start, '"');
            if (end != NULL) {
                int len = end - start;
                strncpy(functions[current_func].name, start, len);
                functions[current_func].name[len] = '\0';
            }
        }
        
        if (in_func_def && strstr(line, "\"names\": [") && functions[current_func].return_type[0] == '\0') {
        char* start = strstr(line, "\"names\": [");
        if (start) {
        start = strchr(start, '"') + 1; 
        char* end = strchr(start, '"');
        if (end) {
            int len = end - start;
            strncpy(functions[current_func].return_type, start, len);
            functions[current_func].return_type[len] = '\0';
        }
        } else {
            fprintf(stderr, "경고: 'names' 필드를 찾을 수 없습니다.\n");
        }

        }
        
        if (in_func_def && strstr(line, "\"params\": [")) {
            in_params = true;
        }
        
        if (in_params && strstr(line, "\"_nodetype\": \"Decl\"")) {
            param_idx = functions[current_func].param_count++;
        }
        
        if (in_params && param_idx >= 0 && strstr(line, "\"name\": \"")) {
            char* start = strstr(line, "\"name\": \"") + 9;
            char* end = strchr(start, '"');
            if (end != NULL) {
                int len = end - start;
                strncpy(functions[current_func].param_names[param_idx], start, len);
                functions[current_func].param_names[param_idx][len] = '\0';
            }
        }
        
        if (in_params && strstr(line, "\"_nodetype\": \"Decl\"")) {
        if (functions[current_func].param_count < 20) {
            param_idx = functions[current_func].param_count++;
        } else {
            fprintf(stderr, "경고: 파라미터 개수 초과 (함수: %s)\n", functions[current_func].name);
            param_idx = -1; 
        }
    }

        if (param_idx >= 0 && param_idx < 20) { 
            strncpy(functions[current_func].param_types[param_idx], ...);
        }

        
        if (in_params && strstr(line, "]")) {
            in_params = false;
        }
        
        if (in_func_def && strstr(line, "\"body\": {")) {
            in_func_body = true;
            brace_level = 1;
        }
        
        if (in_func_body) {
            for (char* c = line; *c; c++) {
                if (*c == '{') brace_level++;
                if (*c == '}') brace_level--;
            }
            
            if (strstr(line, "\"_nodetype\": \"If\"")) {
                functions[current_func].if_count++;
            }
            
            if (brace_level <= 0) {
                in_func_body = false;
                in_func_def = false;
            }
        }
    }
    
    fclose(fp);
}

int main() {
    analyze_ast_file("ast.json");
    
    printf("총 함수 개수: %d\n\n", function_count);
    
    for (int i = 0; i < function_count; i++) {
        FunctionInfo* func = &functions[i];
        
        printf("함수 이름: %s\n", func->name);
        printf("리턴 타입: %s\n", func->return_type);
        printf("파라미터 개수: %d\n", func->param_count);
        
        for (int j = 0; j < func->param_count; j++) {
            printf("  파라미터 %d: %s %s\n", j + 1, func->param_types[j], func->param_names[j]);
        }
        
        printf("if 조건 개수: %d\n\n", func->if_count);
    }
    
    free(functions);
    
    return 0;
}
