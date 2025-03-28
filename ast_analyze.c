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

// 값 추출 헬퍼 함수
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

// 기존 sscanf 방식 대신 문자열 탐색 방식으로 변경
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
    FILE* file = fopen("ast.json", "rb");
    if (!file) {
        printf("Failed to open file\n");
        return 1;
    }

    Node* root = create_node();
    parse_node(file, root);
    fclose(file);

    print_node(root, 0);

    return 0;
}
