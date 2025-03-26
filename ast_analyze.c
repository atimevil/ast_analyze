#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKEN_LENGTH 1000
#define MAX_CHILDREN 100

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

    // 여기서 메모리 해제 로직을 추가해야 합니다.

    return 0;
}
