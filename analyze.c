#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* name;
    char* type;
} Parameter;

typedef struct {
    char* name;
    char* return_type;
    Parameter** parameters;
    int parameter_count;
    int if_condition_count;
} FunctionInfo;

void free_function_info(FunctionInfo* info) {
    if (info == NULL) return;

    free(info->name);
    free(info->return_type);

    for (int i = 0; i < info->parameter_count; i++) {
        free(info->parameters[i]->name);
        free(info->parameters[i]->type);
        free(info->parameters[i]);
    }
    free(info->parameters);
}

int count_functions(const char* json_string) {
    int count = 0;
    const char* func_keyword = "\"FuncDef\"";
    const char* ptr = json_string;

    while ((ptr = strstr(ptr, func_keyword)) != NULL) {
        count++;
        ptr++;
    }

    return count;
}

char* extract_function_return_type(const char* func_start) {
    const char* return_type_keyword = "\"type\":"; // JSON 형식에 맞게 수정
    const char* return_type_pos = strstr(func_start, return_type_keyword);
    if (!return_type_pos) return strdup("Unknown");

    return_type_pos += strlen(return_type_keyword);
    while (*return_type_pos == ' ' || *return_type_pos == '\"') return_type_pos++;

    const char* end_pos = strchr(return_type_pos, '\"');
    if (!end_pos) return strdup("Unknown");

    int len = end_pos - return_type_pos;
    char* return_type = malloc(len + 1);
    if (!return_type) {
        printf("메모리 할당 실패\n");
        exit(1);
    }
    strncpy(return_type, return_type_pos, len);
    return_type[len] = '\0';

    return return_type;
}

char* extract_function_name(const char* func_start) {
    const char* name_keyword = "\"name\":"; // JSON 형식에 맞게 수정
    const char* name_pos = strstr(func_start, name_keyword);
    if (!name_pos) return strdup("Unknown");

    name_pos += strlen(name_keyword);
    while (*name_pos == ' ' || *name_pos == '\"') name_pos++;

    const char* end_pos = strchr(name_pos, '\"');
    if (!end_pos) return strdup("Unknown");

    int len = end_pos - name_pos;
    char* name = malloc(len + 1);
    if (!name) {
        printf("메모리 할당 실패\n");
        exit(1);
    }
    strncpy(name, name_pos, len);
    name[len] = '\0';

    return name;
}

Parameter** extract_function_parameters(const char* func_start, int* param_count) {
    const char* params_keyword = "\"params\":"; // JSON 형식에 맞게 수정
    const char* params_pos = strstr(func_start, params_keyword);
    if (!params_pos) {
        *param_count = 0;
        return NULL;
    }

    params_pos += strlen(params_keyword);
    while (*params_pos == ' ' || *params_pos == '[') params_pos++;

    Parameter** parameters = NULL;
    *param_count = 0;

    while (*params_pos != ']' && *params_pos != '\0') {
        parameters = realloc(parameters, (*param_count + 1) * sizeof(Parameter*));
        if (!parameters) {
            printf("메모리 할당 실패!\n");
            return NULL;
        }

        parameters[*param_count] = malloc(sizeof(Parameter));
        if (!parameters[*param_count]) {
            printf("메모리 할당 실패!\n");
            return NULL;
        }

        const char* param_type_keyword = "\"type\":"; // JSON 형식에 맞게 수정
        const char* param_type_pos = strstr(params_pos, param_type_keyword);
        if (!param_type_pos) {
            printf("파라미터 타입을 찾을 수 없음!\n");
            free(parameters[*param_count]);
            continue;
        }

        param_type_pos += strlen(param_type_keyword);
        while (*param_type_pos == ' ' || *param_type_pos == '\"') param_type_pos++;

        const char* end_pos = strchr(param_type_pos, '\"');
        if (!end_pos) {
            printf("파라미터 타입 종료 문자를 찾을 수 없음!\n");
            free(parameters[*param_count]);
            continue;
        }

        int len = end_pos - param_type_pos;
        parameters[*param_count]->type = (char*)malloc(len + 1);
        if (!parameters[*param_count]->type) {
            printf("파라미터 타입 메모리 할당 실패!\n");
            free(parameters[*param_count]);
            continue;
        }

        strncpy(parameters[*param_count]->type, param_type_pos, len);
        parameters[*param_count]->type[len] = '\0';

        const char* param_name_keyword = "\"name\":"; // JSON 형식에 맞게 수정
        const char* param_name_pos = strstr(params_pos, param_name_keyword);
        if (!param_name_pos) {
            printf("파라미터 이름을 찾을 수 없음!\n");
            free(parameters[*param_count]->type);
            free(parameters[*param_count]);
            continue;
        }

        param_name_pos += strlen(param_name_keyword);
        while (*param_name_pos == ' ' || *param_name_pos == '\"') param_name_pos++;

        end_pos = strchr(param_name_pos, '\"');
        if (!end_pos) {
            printf("파라미터 이름 종료 문자를 찾을 수 없음!\n");
            free(parameters[*param_count]->type);
            free(parameters[*param_count]);
            continue;
        }

        len = end_pos - param_name_pos;
        parameters[*param_count]->name = (char*)malloc(len + 1);
        if (!parameters[*param_count]->name) {
            printf("파라미터 이름 메모리 할당 실패!\n");
            free(parameters[*param_count]->type);
            free(parameters[*param_count]);
            continue;
        }

        strncpy(parameters[*param_count]->name, param_name_pos, len);
        parameters[*param_count]->name[len] = '\0';

        (*param_count)++;
        params_pos = strchr(params_pos, '}');
        if (!params_pos) break;
        params_pos++;
    }

    return parameters;
}

int count_if_conditions(const char* func_start) {
    int count = 0;
    const char* if_keyword = "\"If\""; // JSON 형식에 맞게 수정
    const char* ptr = func_start;

    const char* func_end = strchr(ptr, '}');
    if (!func_end) return 0;

    while ((ptr = strstr(ptr, if_keyword)) != NULL && ptr < func_end) {
        count++;
        ptr++;
    }

    return count;
}

int main() {
    FILE* file = fopen("ast.json", "r");
    if (!file) {
        printf("파일을 열 수 없습니다.\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char* json_string = malloc(file_size + 1);
    fread(json_string, 1, file_size, file);
    json_string[file_size] = '\0';
    fclose(file);

    int total_functions = count_functions(json_string);
    printf("총 함수 개수: %d\n", total_functions);

    FunctionInfo* functions = malloc(total_functions * sizeof(FunctionInfo));

    const char* func_start = json_string;
    int func_index = 0;

    while ((func_start = strstr(func_start, "\"FuncDef\"")) != NULL) {
        printf("\n[DEBUG] 새로운 함수 발견!\n");
        functions[func_index].return_type = extract_function_return_type(func_start);
        functions[func_index].name = extract_function_name(func_start);
        functions[func_index].parameters = extract_function_parameters(func_start, &functions[func_index].parameter_count);
        functions[func_index].if_condition_count = count_if_conditions(func_start);

        func_index++;
        func_start++;
    }

    printf("\n함수 상세 정보:\n");
    for (int i = 0; i < total_functions; i++) {
        printf("\n함수 %d:\n", i+1);
        printf("- 이름: %s\n", functions[i].name);
        printf("- 리턴 타입: %s\n", functions[i].return_type);

        printf("- 파라미터:\n");
        for (int j = 0; j < functions[i].parameter_count; j++) {
            printf("  * 이름: %s, 타입: %s\n",
                   functions[i].parameters[j]->name,
                   functions[i].parameters[j]->type);
        }

        printf("- if 조건문 개수: %d\n", functions[i].if_condition_count);
    }

    for (int i = 0; i < total_functions; i++) {
        free_function_info(&functions[i]);
    }
    free(functions);
    free(json_string);

    return 0;
}
