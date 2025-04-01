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
    const char* return_type_keyword = "\"type\":";
    const char* return_type_pos = strstr(func_start, return_type_keyword);
    if (!return_type_pos) return strdup("Unknown");
    
    return_type_pos += strlen(return_type_keyword);
    while (*return_type_pos == ' ' || *return_type_pos == '\"') return_type_pos++;
    
    const char* end_pos = strchr(return_type_pos, '\"');
    int len = end_pos - return_type_pos;
    char* return_type = (char*)malloc(len + 1);
    strncpy(return_type, return_type_pos, len);
    return_type[len] = '\0';
    
    return return_type;
}

char* extract_function_name(const char* func_start) {
    const char* name_keyword = "\"name\":";
    const char* name_pos = strstr(func_start, name_keyword);
    if (!name_pos) return strdup("Unknown");
    
    name_pos += strlen(name_keyword);
    while (*name_pos == ' ' || *name_pos == '\"') name_pos++;
    
    const char* end_pos = strchr(name_pos, '\"');
    int len = end_pos - name_pos;
    char* name = (char*)malloc(len + 1);
    strncpy(name, name_pos, len);
    name[len] = '\0';
    
    return name;
}

Parameter** extract_function_parameters(const char* func_start, int* param_count) {
    const char* params_keyword = "\"params\":";
    const char* params_pos = strstr(func_start, params_keyword);
    if (!params_pos) return NULL;
    
    params_pos += strlen(params_keyword);
    while (*params_pos == ' ' || *params_pos == '[') params_pos++;
    
    Parameter** parameters = (Parameter**)malloc(10 * sizeof(Parameter*));  
    
    *param_count = 0;
    while (*params_pos != ']') {
        if (*param_count >= 10) {
            parameters = realloc(parameters, (*param_count + 10) * sizeof(Parameter*)); 
        }
        
        parameters[*param_count] = (Parameter*)malloc(sizeof(Parameter));
        
        const char* param_type_keyword = "\"type\":";
        const char* param_type_pos = strstr(params_pos, param_type_keyword);
        param_type_pos += strlen(param_type_keyword);
        while (*param_type_pos == ' ' || *param_type_pos == '\"') param_type_pos++;
        
        const char* end_pos = strchr(param_type_pos, '\"');
        int len = end_pos - param_type_pos;
        parameters[*param_count]->type = (char*)malloc(len + 1);
        strncpy(parameters[*param_count]->type, param_type_pos, len);
        parameters[*param_count]->type[len] = '\0';
        
        const char* param_name_keyword = "\"name\":";
        const char* param_name_pos = strstr(params_pos, param_name_keyword);
        param_name_pos += strlen(param_name_keyword);
        while (*param_name_pos == ' ' || *param_name_pos == '\"') param_name_pos++;
        
        end_pos = strchr(param_name_pos, '\"');
        len = end_pos - param_name_pos;
        parameters[*param_count]->name = (char*)malloc(len + 1);
        strncpy(parameters[*param_count]->name, param_name_pos, len);
        parameters[*param_count]->name[len] = '\0';
        
        (*param_count)++;
        
        params_pos = strchr(params_pos, '}');
        params_pos++;
    }
    
    return parameters;
}

int count_if_conditions(const char* json_string) {
    int count = 0;
    const char* if_keyword = "\"If\"";
    const char* ptr = json_string;
    
    while ((ptr = strstr(ptr, if_keyword)) != NULL) {
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
