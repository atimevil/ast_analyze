#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 기본 용량
#define INITIAL_CAPACITY_FUNCTIONS 10
#define INITIAL_CAPACITY_CHILDREN 10
#define MAX_TOKEN_LENGTH 5000

typedef struct {
    char name[MAX_TOKEN_LENGTH];
    char return_type[MAX_TOKEN_LENGTH];
    char params[MAX_TOKEN_LENGTH];
} FunctionInfo;

typedef struct {
    int total_functions;
    int capacity_functions;
    FunctionInfo* functions;
    int total_if_statements;
} AnalysisResult;

typedef struct Node {
    char type[MAX_TOKEN_LENGTH];
    char name[MAX_TOKEN_LENGTH];
    char value[MAX_TOKEN_LENGTH];
    struct Node** children;
    int child_count;
    int capacity_children;
} Node;

// Node 구조체 초기화
Node* create_node() {
    Node* node = (Node*)malloc(sizeof(Node));
    node->child_count = 0;
    node->capacity_children = INITIAL_CAPACITY_CHILDREN;
    node->children = (Node**)malloc(node->capacity_children * sizeof(Node*));
    return node;
}

// Node에 자식 노드 추가
void add_child(Node* node, Node* child) {
    if (node->child_count >= node->capacity_children) {
        node->capacity_children *= 2;
        node->children = realloc(node->children, node->capacity_children * sizeof(Node*));
    }
    node->children[node->child_count++] = child;
}

// AnalysisResult 구조체 초기화
void init_analysis_result(AnalysisResult* result) {
    result->total_functions = 0;
    result->capacity_functions = INITIAL_CAPACITY_FUNCTIONS;
    result->functions = (FunctionInfo*)malloc(result->capacity_functions * sizeof(FunctionInfo));
    result->total_if_statements = 0;
}

// AnalysisResult에 함수 정보 추가
void add_function(AnalysisResult* result, const char* name, const char* return_type, const char* params) {
    if (result->total_functions >= result->capacity_functions) {
        result->capacity_functions *= 2;
        result->functions = realloc(result->functions, result->capacity_functions * sizeof(FunctionInfo));
    }
    
    strcpy(result->functions[result->total_functions].name, name);
    strcpy(result->functions[result->total_functions].return_type, return_type);
    strcpy(result->functions[result->total_functions].params, params);
    result->total_functions++;
}

// 노드에서 값을 추출하는 함수
void extract_value(char* start, char* dest) {
    char* colon = strchr(start, ':');
    if (!colon) return;
    
    char* quote_open = strchr(colon, '"');
    if (!quote_open) return;
    
    char* quote_close = strchr(quote_open + 1, '"');
    if (!quote_close) return;

    strncpy(dest, quote_open + 1, quote_close - quote_open - 1);
    dest[quote_close - quote_open - 1] = '\0';
}

// JSON 파일에서 노드 정보를 읽어오는 함수
void parse_node(FILE* file, Node* node) {
    char line[MAX_TOKEN_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char* key_pos;
        if ((key_pos = strstr(line, "\"_nodetype\""))) {
            extract_value(key_pos, node->type);
        } else if ((key_pos = strstr(line, "\"name\""))) {
            extract_value(key_pos, node->name);
        } else if ((key_pos = strstr(line, "\"value\""))) {
            extract_value(key_pos, node->value);
        } else if (strstr(line, "{")) {
            Node* child = create_node();
            parse_node(file, child);
            add_child(node, child);
        } else if (strstr(line, "}")) {
            return;
        }
    }
}

// AST를 분석하는 함수
void analyze_ast(Node* node, AnalysisResult* result, int in_function) {
    if (strcmp(node->type, "FuncDecl") == 0) {
        char name[MAX_TOKEN_LENGTH] = "";
        char return_type[MAX_TOKEN_LENGTH] = "";
        char params[MAX_TOKEN_LENGTH] = "";
        
        strcpy(name, node->name);
        
        for (int i = 0; i < node->child_count; i++) {
            Node* child = node->children[i];
            if (strcmp(child->type, "Type") == 0) {
                strcpy(return_type, child->name);
                break;
            }
        }
        
        for (int i = 0; i < node->child_count; i++) {
            Node* child = node->children[i];
            if (strcmp(child->type, "Param") == 0) {
                char type[MAX_TOKEN_LENGTH] = "";
                for (int j = 0; j < child->child_count; j++) {
                    if (strcmp(child->children[j]->type, "Type") == 0) {
                        strcpy(type, child->children[j]->name);
                        break;
                    }
                }
                strcat(params, type);
                strcat(params, " ");
                strcat(params, child->name);
                strcat(params, ", ");
            }
        }
        
        if (strlen(params) > 2) {
            params[strlen(params) - 2] = '\0'; // 마지막 ", " 제거
        }
        
        add_function(result, name, return_type, params);
        in_function = 1;
    }
    
    if (strcmp(node->type, "If") == 0) {
        result->total_if_statements++;
    }
    
    for (int i = 0; i < node->child_count; i++) {
        analyze_ast(node->children[i], result, in_function);
    }
}

// 분석 결과를 출력하는 함수
void print_analysis(AnalysisResult* result) {
    printf("\n=== AST 분석 결과 ===\n");
    printf("• 전체 함수 개수: %d\n", result->total_functions);
    
    for (int i = 0; i < result->total_functions; i++) {
        printf("\n[함수 %d]\n", i+1);
        printf("  이름: %s\n", result->functions[i].name);
        printf("  반환 타입: %s\n", result->functions[i].return_type);
        printf("  파라미터 명: %s\n", result->functions[i].params);
    }
    
    printf("\n• 전체 IF문 개수: %d\n", result->total_if_statements);
}

// 노드를 출력하는 함수
void print_node(Node* node, int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
    printf("Type: %s", node->type);
    if (strlen(node->name) > 0) printf(", Name: %s", node->name);
    if (strlen(node->value) > 0) printf(", Value: %s", node->value);
    printf("\n");

    for (int i = 0; i < node->child_count; i++) {
        print_node(node->children[i], depth + 1);
    }
}

// 메모리 해제 함수
void free_node(Node* node) {
    for (int i = 0; i < node->child_count; i++) {
        free_node(node->children[i]);
    }
    free(node->children);
    free(node);
}

int main() {
    FILE* file = fopen("ast.json", "rb");
    if (!file) {
        printf("Failed to open file\n");
        return 1;
    }

    Node* root = create_node();
    parse_node(file, root);
    fclose(file);

    print_node(root, 0);
    
    AnalysisResult result;
    init_analysis_result(&result);
    analyze_ast(root, &result, 0);
    print_analysis(&result);
    
    free_node(root);
    free(result.functions);
    
    return 0;
}
