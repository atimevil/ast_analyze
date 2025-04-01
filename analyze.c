#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsmn.h"

#define MAX_FUNCTIONS 100
#define MAX_PARAMS 100
#define MAX_NAME_LENGTH 256
#define MAX_TOKENS 25565

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

    int token_count = jsmn_parse(&parser, json, strlen(json), NULL, 0);

    if (token_count < 0) {
    const char *err_msg[] = {
        [JSMN_ERROR_NOMEM] = "Not enough tokens",
        [JSMN_ERROR_INVAL] = "Invalid character",
        [JSMN_ERROR_PART] = "Truncated JSON"
    };
    fprintf(stderr, "JSON 파싱 실패 (%s)\n", err_msg[-token_count]);
}
    jsmntok_t *tokens = malloc(sizeof(jsmntok_t) * token_count);
    
    jsmn_init(&parser);
    token_count = jsmn_parse(&parser, json, strlen(json), tokens, token_count);

    parse_json(json, tokens, token_count);

    free(json);

    // Print results
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
                FunctionInfo func = {0};
                strcpy(func.name, "unknown");
                strcpy(func.return_type, "unknown");

                for (int j = i + 1; j < token_count && tokens[j].start < tokens[i].end; j++) {
                    if (tokens[j].type == JSMN_STRING && strncmp(json + tokens[j].start, "name", tokens[j].end - tokens[j].start) == 0) {
                        extract_string(json, &tokens[j + 1], func.name, MAX_NAME_LENGTH);
                    } else if (tokens[j].type == JSMN_STRING && strncmp(json + tokens[j].start, "return_type", tokens[j].end - tokens[j].start) == 0) {
                        extract_string(json, &tokens[j + 1], func.return_type, MAX_NAME_LENGTH);
                    } else if (tokens[j].type == JSMN_OBJECT && find_key(json, &tokens[j], token_count, "_nodetype") >= 0) {
                        int sub_nodetype_index = find_key(json, &tokens[j], token_count, "_nodetype");
                        char sub_nodetype[MAX_NAME_LENGTH] = {0};
                        extract_string(json, &tokens[sub_nodetype_index + 1], sub_nodetype, MAX_NAME_LENGTH);

                        if (strcmp(sub_nodetype, "ParamList") == 0) {
                            for (int k = j + 1; k < token_count && tokens[k].start < tokens[j].end; k++) {
                                if (tokens[k].type == JSMN_OBJECT && find_key(json, &tokens[k], token_count, "_nodetype") >= 0) {
                                    func.param_count++;
                                }
                            }
                        } else if (strcmp(sub_nodetype, "If") == 0) {
                            func.if_count++;
                        }
                    }
                }

                functions[function_count++] = func;
            }
        }
    }
}

void extract_string(const char *json, jsmntok_t *token, char *buffer, size_t max_len) {
    size_t len = token->end - token->start;
    if (len >= max_len) len = max_len - 1;
    
    strncpy(buffer, json + token->start, len);
    buffer[len] = '\0';
}

int find_key(const char *json, jsmntok_t *tokens, int token_count, const char *key) {
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == JSMN_STRING && strncmp(json + tokens[i].start, key,
                                                     tokens[i].end - tokens[i].start) == 0) {
            return i;
        }
    }
    
    return -1;
}
