#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsmn.h"

#define MAX_FUNCTIONS 100
#define MAX_PARAMS 100
#define MAX_NAME_LENGTH 256
#define MAX_TOKENS 4096

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

void parse_json(const char *json, jsmntok_t *tokens, int token_count);
void extract_functions(const char *json, jsmntok_t *tokens, int start, int end);
void extract_string(const char *json, jsmntok_t *token, char *buffer, size_t max_len);
int find_key(const char *json, jsmntok_t *tokens, int token_count, const char *key);

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

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *json = malloc(length + 1);
    if (!json) {
        fprintf(stderr, "Memory allocation error\n");
        fclose(file);
        return 1;
    }

    fread(json, 1, length, file);
    json[length] = '\0';
    fclose(file);

    jsmn_parser parser;
    jsmn_init(&parser);

    jsmntok_t tokens[MAX_TOKENS];
    int token_count = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);

    if (token_count < 0) {
        fprintf(stderr, "Failed to parse JSON: %d\n", token_count);
        free(json);
        return 1;
    }

    parse_json(json, tokens, token_count);

    free(json);

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

    return 0;
}

void parse_json(const char *json, jsmntok_t *tokens, int token_count) {
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == JSMN_OBJECT && find_key(json, &tokens[i], token_count, "_nodetype") >= 0) {
            int nodetype_index = find_key(json, &tokens[i], token_count, "_nodetype");
            char nodetype[MAX_NAME_LENGTH] = {0};
            extract_string(json, &tokens[nodetype_index + 1], nodetype, MAX_NAME_LENGTH);

            if (strcmp(nodetype, "FuncDef") == 0) {
                extract_functions(json, tokens, i + 1, tokens[i].end);
            }
        }
    }
}

void extract_functions(const char *json, jsmntok_t *tokens, int start, int end) {
    FunctionInfo func = {0};
    strcpy(func.name, "unknown");
    strcpy(func.return_type, "unknown");

    for (int i = start; i < end; i++) {
        if (tokens[i].type == JSMN_STRING && strncmp(json + tokens[i].start, "name", tokens[i].end - tokens[i].start) == 0) {
            extract_string(json, &tokens[i + 1], func.name, MAX_NAME_LENGTH);
        } else if (tokens[i].type == JSMN_STRING && strncmp(json + tokens[i].start, "return_type", tokens[i].end - tokens[i].start) == 0) {
            extract_string(json, &tokens[i + 1], func.return_type, MAX_NAME_LENGTH);
        } else if (tokens[i].type == JSMN_OBJECT && find_key(json, &tokens[i], end - start + 1, "_nodetype") >= 0) {
            int sub_nodetype_index = find_key(json, &tokens[i], end - start + 1, "_nodetype");
            char sub_nodetype[MAX_NAME_LENGTH] = {0};
            extract_string(json, &tokens[sub_nodetype_index + 1], sub_nodetype, MAX_NAME_LENGTH);

            if (strcmp(sub_nodetype, "If") == 0) {
                func.if_count++;
            }
        }
    }

    functions[function_count++] = func;
}

void extract_string(const char *json, jsmntok_t *token, char *buffer, size_t max_len) {
    size_t len = token->end - token->start;
    if (len >= max_len) len = max_len - 1;

    strncpy(buffer, json + token->start, len);
    buffer[len] = '\0';
}

/* JSON 파서의 핵심 로직 (완전한 구현부) */
int find_key(const char *json, jsmntok_t *tokens, int token_count, const char *key, int start) {
    for (int i = start; i < token_count; i++) {
        if (tokens[i].type == JSMN_STRING && 
            strncmp(json + tokens[i].start, key, tokens[i].end - tokens[i].start) == 0) {
            return i;
        }
    }
    return -1;
}

void extract_functions(const char *json, jsmntok_t *tokens, int token_count) {
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == JSMN_OBJECT) {
            int nodetype_idx = find_key(json, tokens, token_count, "_nodetype", i);
            if (nodetype_idx == -1) continue;

            char nodetype[32];
            extract_string(json, &tokens[nodetype_idx + 1], nodetype, sizeof(nodetype));

            if (strcmp(nodetype, "FuncDef") == 0) {
                FunctionInfo func = {0};
                
                /* 함수 이름 추출 */
                int decl_idx = find_key(json, tokens, token_count, "decl", i);
                if (decl_idx != -1) {
                    int name_idx = find_key(json, tokens, token_count, "name", decl_idx);
                    if (name_idx != -1) {
                        extract_string(json, &tokens[name_idx + 1], func.name, MAX_NAME_LENGTH);
                    }
                }

                /* 반환 타입 추출 */
                int func_decl_idx = find_key(json, tokens, token_count, "FuncDecl", i);
                if (func_decl_idx != -1) {
                    int type_idx = find_key(json, tokens, token_count, "type", func_decl_idx);
                    if (type_idx != -1) {
                        extract_string(json, &tokens[type_idx + 1], func.return_type, MAX_NAME_LENGTH);
                    }
                }

                /* 매개변수 분석 */
                int param_list_idx = find_key(json, tokens, token_count, "ParamList", i);
                if (param_list_idx != -1) {
                    int params_idx = find_key(json, tokens, token_count, "params", param_list_idx);
                    if (params_idx != -1 && tokens[params_idx].type == JSMN_ARRAY) {
                        for (int j = params_idx + 1; j < token_count; j++) {
                            if (tokens[j].type == JSMN_OBJECT) {
                                int param_type_idx = find_key(json, tokens, token_count, "type", j);
                                int param_name_idx = find_key(json, tokens, token_count, "name", j);
                                
                                if (param_type_idx != -1) {
                                    extract_string(json, &tokens[param_type_idx + 1], 
                                                 func.param_types[func.param_count], MAX_NAME_LENGTH);
                                }
                                
                                if (param_name_idx != -1) {
                                    extract_string(json, &tokens[param_name_idx + 1], 
                                                 func.param_names[func.param_count], MAX_NAME_LENGTH);
                                }
                                
                                func.param_count++;
                            }
                            if (tokens[j].parent == param_list_idx) break;
                        }
                    }
                }

                /* if 문 카운팅 */
                for (int j = i; j < token_count; j++) {
                    if (tokens[j].parent == i && 
                        find_key(json, tokens, token_count, "If", j) != -1) {
                        func.if_count++;
                    }
                }

                functions[function_count++] = func;
            }
        }
    }
}

