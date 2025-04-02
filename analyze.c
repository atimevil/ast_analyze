#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 4096
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

// AST 노드의 깊이를 추적하기 위한 변수들
int current_depth = 0;
int func_def_depth = -1;
int decl_depth = -1;
int type_depth = -1;
int param_list_depth = -1;

void analyze_ast_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "파일 열기 실패: %s\n", filename);
        return;
    }
    
    // 함수 개수 먼저 확인
    char line[MAX_LINE_LENGTH];
    int func_count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "\"_nodetype\": \"FuncDef\""))
            func_count++;
    }
    
    // 메모리 할당
    functions = (FunctionInfo*)calloc(func_count, sizeof(FunctionInfo));
    function_count = func_count;
    
    rewind(fp);
    
    int current_func = -1;
    int current_param = -1;
    bool in_func_name = false;
    bool in_return_type = false;
    bool in_param_name = false;
    bool in_param_type = false;
    
    while (fgets(line, sizeof(line), fp)) {
        // 중괄호로 깊이 추적
        for (char* c = line; *c; c++) {
            if (*c == '{') current_depth++;
            if (*c == '}') current_depth--;
        }
        
        // 함수 정의 시작
        if (strstr(line, "\"_nodetype\": \"FuncDef\"")) {
            current_func++;
            func_def_depth = current_depth;
            
            // 함수 정보 초기화
            strcpy(functions[current_func].name, "");
            strcpy(functions[current_func].return_type, DEFAULT_UNKNOWN_TYPE);
            functions[current_func].param_count = 0;
            functions[current_func].if_count = 0;
            
            current_param = -1;
            continue;
        }
        
        // 현재 함수 내부에 있는지 확인
        if (current_func >= 0 && current_depth > func_def_depth) {
            // 함수 이름 추출 (Decl -> name 패턴)
            if (strstr(line, "\"name\": \"") && functions[current_func].name[0] == '\0') {
                char* start = strstr(line, "\"name\": \"") + 9;
                char* end = strchr(start, '\"');
                if (end) {
                    int len = end - start;
                    strncpy(functions[current_func].name, start, len);
                    functions[current_func].name[len] = '\0';
                }
            }
            
            // 리턴 타입 추출 시도
            if (strstr(line, "\"type\": \"") && 
                strcmp(functions[current_func].return_type, DEFAULT_UNKNOWN_TYPE) == 0) {
                char* start = strstr(line, "\"type\": \"") + 9;
                char* end = strchr(start, '\"');
                if (end) {
                    int len = end - start;
                    strncpy(functions[current_func].return_type, start, len);
                    functions[current_func].return_type[len] = '\0';
                }
            }
            
            // 파라미터 처리
            if (strstr(line, "\"params\":")) {
                param_list_depth = current_depth;
                continue;
            }
            
            // 파라미터 내부에 있는지 확인
            if (param_list_depth > 0 && current_depth > param_list_depth) {
                // 새 파라미터 발견
                if (strstr(line, "\"_nodetype\": \"Decl\"")) {
                    current_param = functions[current_func].param_count++;
                    if (current_param < 20) {
                        strcpy(functions[current_func].param_types[current_param], "");
                        strcpy(functions[current_func].param_names[current_param], "");
                    }
                }
                
                // 파라미터 이름 추출
                if (current_param >= 0 && current_param < 20) {
                    if (strstr(line, "\"name\": \"") && 
                        functions[current_func].param_names[current_param][0] == '\0') {
                        char* start = strstr(line, "\"name\": \"") + 9;
                        char* end = strchr(start, '\"');
                        if (end) {
                            int len = end - start;
                            strncpy(functions[current_func].param_names[current_param], start, len);
                            functions[current_func].param_names[current_param][len] = '\0';
                        }
                    }
                    
                    // 파라미터 타입 추출 시도
                    if (strstr(line, "\"names\": [\"") && 
                        strcmp(functions[current_func].param_types[current_param], DEFAULT_UNKNOWN_TYPE) == 0) {
                        char* start = strstr(line, "\"names\": [\"") + 11;
                        char* end = strchr(start, '\"');
                        if (end) {
                            int len = end - start;
                            strncpy(functions[current_func].param_types[current_param], start, len);
                            functions[current_func].param_types[current_param][len] = '\0';
                        }
                    }
                }
            }
            
            // If 문 카운팅
            if (strstr(line, "\"_nodetype\": \"If\"")) {
                functions[current_func].if_count++;
            }
        }
        
        // 깊이가 함수 정의 수준 이하로 내려가면 함수 처리 종료
        if (current_func >= 0 && func_def_depth >= 0 && current_depth <= func_def_depth) {
            func_def_depth = -1;
            param_list_depth = -1;
        }
    }
    
    fclose(fp);
}

int main() {
    analyze_ast_file("ast.json");

    printf("총 함수 개수 : %d", function_count);
    
    printf("[함수 분석 결과]\n");
    
    for (int i = 0; i < function_count; i++) {
        printf("%d. %s()\n", i+1, strlen(functions[i].name) > 0 ? functions[i].name : "");
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
