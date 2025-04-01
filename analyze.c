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

char* extract_value(const char* json, const char* key) {
    const char* key_pos = strstr(json, key);
    if (!key_pos) return NULL;

    const char* start = strchr(key_pos, ':');
    if (!start) return NULL;

    start++;
    while (*start == ' ' || *start == '\"') start++; 

    const char* end = strchr(start, '\"');
    if (!end) end = strchr(start, ','); 

    if (!end) return NULL;

    size_t len = end - start;
    char* value = malloc(len + 1);
    strncpy(value, start, len);
    value[len] = '\0';

    return value;
}

char* extract_function_name(const char* func_start) {
    return extract_value(func_start, "\"name\"");
}

char* extract_function_return_type(const char* func_start) {
    const char* type_key = "\"type\"";
    const char* type_pos = strstr(func_start, type_key);

    if (!type_pos) return strdup("Unknown");

    const char* names_key = "\"names\"";
    const char* names_pos = strstr(type_pos, names_key);

    if (!names_pos) return strdup("Unknown");

    return extract_value(names_pos, "");
}

Parameter** extract_function_parameters(const char* func_start, int* param_count) {
    const char* params_key = "\"params\"";
    const char* params_pos = strstr(func_start, params_key);

    if (!params_pos) {
        *param_count = 0;
        return NULL;
    }

    const char* start = strchr(params_pos, '[');
    if (!start) {
        *param_count = 0;
        return NULL;
    }

    Parameter** parameters = NULL;
    *param_count = 0;

    const char* current = start + 1; 
    while (*current && *current != ']') {
        parameters = realloc(parameters, (*param_count + 1) * sizeof(Parameter*));
        parameters[*param_count] = malloc(sizeof(Parameter));

        parameters[*param_count]->name = extract_value(current, "\"name\"");
        parameters[*param_count]->type = extract_value(current, "\"type\"");

        (*param_count)++;

        current = strchr(current, '}');
        if (!current) break;
        current++;
        while (*current == ',' || *current == ' ') current++;
    }

    return parameters;
}

int count_if_conditions(const char* func_start) {
    int count = 0;
    const char* current = func_start;

    while ((current = strstr(current, "\"If\"")) != NULL) {
        count++;
        current++;
    }

    return count;
}

int count_functions(const char* json_string) {
    int count = 0;
    const char* current = json_string;

    while ((current = strstr(current, "\"FuncDef\"")) != NULL) {
        count++;
        current++;
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
        functions[func_index].name = extract_function_name(func_start);
        functions[func_index].return_type = extract_function_return_type(func_start);
        functions[func_index].parameters = extract_function_parameters(func_start, &functions[func_index].parameter_count);
        functions[func_index].if_condition_count = count_if_conditions(func_start);

        func_index++;
        func_start++;
    }

    printf("\n함수 상세 정보:\n");
    for (int i = 0; i < total_functions; i++) {
        printf("\n함수 %d:\n", i + 1);
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
