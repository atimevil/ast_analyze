#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 2048
#define MAX_NAME_LENGTH 256
#define DEFAULT_UNKNOWN_TYPE "알수없음"

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

void extract_type_info(char* line, char* type_buffer) {
    char* start = strstr(line, "\"names\": [");
    if (start) {
        start = strchr(start, '[') + 2;
        char* end = strchr(start, '"');
        if (end) {
            size_t len = end - start;
            strncpy(type_buffer, start, len);
            type_buffer[len] = '\0';
            return;
        }
    }
    strcpy(type_buffer, DEFAULT_UNKNOWN_TYPE);
}

void analyze_ast_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "파일 오픈 실패: %s\n", filename);
        return;
    }

    char line[MAX_LINE_LENGTH];
    int func_count = 0;
    
    // 함수 개수 사전 카운팅
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "\"_nodetype\": \"FuncDef\"")) func_count++;
    }
    
    functions = (FunctionInfo*)calloc(func_count, sizeof(FunctionInfo));
    function_count = func_count;
    rewind(fp);

    int current_func = -1;
    bool in_func_def = false;
    bool in_decl = false;
    bool in_typedecl = false;
    bool in_params = false;
    bool in_body = false;
    int brace_level = 0;
    int param_idx = -1;

    while (fgets(line, sizeof(line), fp)) {
        // FuncDef 진입 처리
        if (strstr(line, "\"_nodetype\": \"FuncDef\"")) {
            current_func++;
            in_func_def = true;
            in_decl = false;
            in_typedecl = false;
            in_params = false;
            in_body = false;
            brace_level = 0;
            param_idx = -1;
            memset(&functions[current_func], 0, sizeof(FunctionInfo));
            continue;
        }

        if (in_func_def) {
            // Decl 노드 처리
            if (strstr(line, "\"decl\": {")) {
                in_decl = true;
                brace_level = 1;
            }

            // Return 타입 추출
            if (in_decl && strstr(line, "\"type\": {")) {
                in_typedecl = true;
            }

            if (in_typedecl) {
                if (strstr(line, "\"names\": [")) {
                    extract_type_info(line, functions[current_func].return_type);
                }
                if (strstr(line, "}")) brace_level--;
                if (brace_level <= 0) in_typedecl = false;
            }

            // 파라미터 처리
            if (strstr(line, "\"params\": [")) {
                in_params = true;
                param_idx = 0;
                functions[current_func].param_count = 0;
            }

            if (in_params) {
                if (strstr(line, "\"_nodetype\": \"Decl\"")) {
                    if (functions[current_func].param_count < 20) {
                        param_idx = functions[current_func].param_count++;
                    }
                }

                if (param_idx >= 0 && strstr(line, "\"name\": \"")) {
                    char* start = strstr(line, "\"name\": \"") + 9;
                    char* end = strchr(start, '"');
                    if (end) {
                        strncpy(functions[current_func].param_names[param_idx], start, end - start);
                    }
                }

                if (param_idx >= 0 && strstr(line, "\"names\": [")) {
                    extract_type_info(line, functions[current_func].param_types[param_idx]);
                }
            }

            // 함수 본문 처리
            if (strstr(line, "\"body\": {")) {
                in_body = true;
                brace_level = 1;
            }

            if (in_body) {
                for (char* p = line; *p; p++) {
                    if (*p == '{') brace_level++;
                    if (*p == '}') brace_level--;
                }
                
                if (strstr(line, "\"_nodetype\": \"If\"")) {
                    functions[current_func].if_count++;
                }

                if (brace_level <= 0) {
                    in_body = false;
                    in_func_def = false;
                }
            }
        }
    }

    fclose(fp);
}

int main() {
    analyze_ast_file("ast.json");

    printf("[함수 분석 결과]\n");
    for (int i = 0; i < function_count; i++) {
        printf("%d. %s()\n", i+1, functions[i].name);
        printf("   - 반환 타입: %s\n", functions[i].return_type);
        printf("   - 매개변수: %d개\n", functions[i].param_count);
        for (int j = 0; j < functions[i].param_count; j++) {
            printf("     %d: %s %s\n", j+1, 
                   functions[i].param_types[j], 
                   functions[i].param_names[j]);
        }
        printf("   - 조건문 수: %d\n\n", functions[i].if_count);
    }

    free(functions);
    return 0;
}
