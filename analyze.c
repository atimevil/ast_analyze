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

void extract_function_info(FILE* fp) {
    char line[MAX_LINE_LENGTH];
    int current_func = -1;
    bool in_func_def = false;
    bool in_params = false;
    int param_idx = -1;

    while (fgets(line, sizeof(line), fp)) {
        // 함수 정의 시작
        if (strstr(line, "\"_nodetype\": \"FuncDef\"")) {
            current_func++;
            in_func_def = true;
            param_idx = -1;

            // 함수 정보 초기화
            functions[current_func].param_count = 0;
            functions[current_func].if_count = 0;
            strcpy(functions[current_func].name, DEFAULT_UNKNOWN_TYPE);
            strcpy(functions[current_func].return_type, DEFAULT_UNKNOWN_TYPE);
        }

        // 함수 이름 추출
        if (in_func_def && strstr(line, "\"declname\": \"")) {
            char* start = strstr(line, "\"declname\": \"") + 13;
            char* end = strchr(start, '\"');
            if (end) {
                strncpy(functions[current_func].name, start, end - start);
                functions[current_func].name[end - start] = '\0';
            }
        }

        // 반환 타입 추출
        if (in_func_def && strstr(line, "\"_nodetype\": \"TypeDecl\"")) {
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "\"names\": [")) {
                    char* start = strstr(line, "\"names\": [") + 10;
                    char* end = strchr(start, ']');
                    if (end) {
                        strncpy(functions[current_func].return_type, start + 1, end - start - 2);
                        functions[current_func].return_type[end - start - 2] = '\0';
                    }
                    break;
                }
            }
        }

        // 매개변수 처리 시작
        if (in_func_def && strstr(line, "\"_nodetype\": \"ParamList\"")) {
            in_params = true;
        }

        // 매개변수 정보 추출
        if (in_params && strstr(line, "\"_nodetype\": \"Decl\"")) {
            param_idx++;
            functions[current_func].param_count++;

            // 매개변수 이름 추출
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "\"declname\": \"")) {
                    char* start = strstr(line, "\"declname\": \"") + 13;
                    char* end = strchr(start, '\"');
                    if (end) {
                        strncpy(functions[current_func].param_names[param_idx], start, end - start);
                        functions[current_func].param_names[param_idx][end - start] = '\0';
                    }
                    break;
                }
            }

            // 매개변수 타입 추출
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "\"names\": [")) {
                    char* start = strstr(line, "\"names\": [") + 10;
                    char* end = strchr(start, ']');
                    if (end) {
                        strncpy(functions[current_func].param_types[param_idx], start + 1, end - start - 2);
                        functions[current_func].param_types[param_idx][end - start - 2] = '\0';
                    }
                    break;
                }
            }
        }

        // 매개변수 처리 종료
        if (in_params && strstr(line, "]")) {
            in_params = false;
        }

        // If 문 카운팅
        if (in_func_def && strstr(line, "\"_nodetype\": \"If\"")) {
            functions[current_func].if_count++;
        }

        // 함수 정의 종료
        if (in_func_def && strstr(line, "}")) {
            in_func_def = false;
        }
    }
}

void analyze_ast_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "파일 열기 실패: %s\n", filename);
        return;
    }

    // 함수 개수 카운트
    char line[MAX_LINE_LENGTH];
    int func_count = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "\"_nodetype\": \"FuncDef\""))
            func_count++;
    }

    functions = calloc(func_count, sizeof(FunctionInfo));
    function_count = func_count;

    rewind(fp);
    extract_function_info(fp);

    fclose(fp);
}

int main() {
    analyze_ast_file("ast.json");

    printf("[함수 분석 결과]\n");
    for (int i = 0; i < function_count; i++) {
        printf("%d. %s()\n", i + 1,
               strlen(functions[i].name) > 0 ? functions[i].name : DEFAULT_UNKNOWN_TYPE);
        printf("   - 반환 타입: %s\n", functions[i].return_type);
        printf("   - 매개변수: %d개\n", functions[i].param_count);

        for (int j = 0; j < functions[i].param_count; j++) {
            printf("     %d: %s %s\n", j + 1,
                   functions[i].param_types[j],
                   functions[i].param_names[j]);
        }

        printf("   - 조건문 수: %d\n\n", functions[i].if_count);
    }

    free(functions);
    return 0;
}
