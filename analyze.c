#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKEN_LENGTH 1000
#define MAX_CHILDREN 100
#define MAX_FUNCTIONS 50

typedef struct {
    int total_functions;
    int total_if_statements;
    struct {
        char name[MAX_TOKEN_LENGTH];
        char return_type[MAX_TOKEN_LENGTH];
        char params[MAX_TOKEN_LENGTH];
    } functions[MAX_FUNCTIONS];
} AnalysisResult;

typedef struct Node {
    char type[MAX_TOKEN_LENGTH];
    char name[MAX_TOKEN_LENGTH];
    char value[MAX_TOKEN_LENGTH];
    struct Node* children[MAX_CHILDREN];
    int child_count;
} Node;

Node* create_node() {
    Node* node = (Node*)malloc(sizeof(Node));
    node->child_count = 0;
    return node;
}

void analyze_ast(Node* node, AnalysisResult* result, int in_function) {
    if (strcmp(node->type, "FuncDecl") == 0) {
        int idx = result->total_functions++;
        strcpy(result->functions[idx].name, node->name);
        
        for (int i = 0; i < node->child_count; i++) {
            Node* child = node->children[i];
            if (strcmp(child->type, "Type") == 0) {
                strcpy(result->functions[idx].return_type, child->name);
                break;
            }
        }
        in_function = 1; 
    }
    
    if (in_function && strcmp(node->type, "Param") == 0) {
        char type[MAX_TOKEN_LENGTH] = "";
        for (int i = 0; i < node->child_count; i++) {
            if (strcmp(node->children[i]->type, "Type") == 0) {
                strcpy(type, node->children[i]->name);
                break;
            }
        }
        strcat(result->functions[result->total_functions-1].params, type);
        strcat(result->functions[result->total_functions-1].params, " ");
        strcat(result->functions[result->total_functions-1].params, node->name);
        strcat(result->functions[result->total_functions-1].params, ", ");
    }
    
    if (strcmp(node->type, "If") == 0) {
        result->total_if_statements++;
    }
    
    for (int i = 0; i < node->child_count; i++) {
        analyze_ast(node->children[i], result, in_function);
    }
}

void print_analysis(AnalysisResult* result) {
    printf("\n=== AST 분석 결과 ===\n");
    printf("• 전체 함수 개수: %d\n", result->total_functions);
    
    for (int i = 0; i < result->total_functions; i++) {
        printf("\n[함수 %d]\n", i+1);
        printf("  이름: %s\n", result->functions[i].name);
        printf("  반환 타입: %s\n", result->functions[i].return_type);
        
        if (strlen(result->functions[i].params) > 0) {
            result->functions[i].params[strlen(result->functions[i].params)-2] = '\0';
        }
        printf("  파라미터: %s\n", result->functions[i].params);
    }
    
    printf("\n• 전체 IF문 개수: %d\n", result->total_if_statements);
}

void parse_node(FILE* file, Node* node) {
    char line[MAX_TOKEN_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "_nodetype")) {
            sscanf(line, "\"_nodetype\": \"%[^\"]\"", node->type);
        } else if (strstr(line, "\"name\":")) {
            sscanf(line, "\"name\": \"%[^\"]\"", node->name);
        } else if (strstr(line, "\"value\":")) {
            sscanf(line, "\"value\": \"%[^\"]\"", node->value);
        } else if (strstr(line, "{")) {
            Node* child = create_node();
            parse_node(file, child);
            node->children[node->child_count++] = child;
        } else if (strstr(line, "}")) {
            return;
        }
    }
}

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

int main() {
    FILE* file = fopen("ast.json", "r");
    if (!file) {
        printf("Failed to open file\n");
        return 1;
    }

    Node* root = create_node();
    parse_node(file, root);
    fclose(file);

    print_node(root, 0);

    AnalysisResult result = {0};
    analyze_ast(root, &result, 0);
    print_analysis(&result);

    return 0;
}
