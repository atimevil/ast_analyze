#include <stdio.h>
#include <stdlib.h>

void error() {
    exit(1);
}

void takechar() {
    nextc = getchar();
    if (token_size <= i + 1) {
        int x = (i + 1 + 10) * 2;
        token = my_realloc(token, token_size, x);
        token_size = x;
    }
    token[i] = nextc;
    i++;
}

void get_token() {
    int w = 1;
    while (w) {
        w = 0;
        while (nextc == ' ' || nextc == '\t' || nextc == '\n') {
            nextc = getchar();
            i = 0;
        }
        if (('a' <= nextc && nextc <= 'z') || 
            ('0' <= nextc && nextc <= '9') || 
            nextc == '_') {
            takechar();
        }
        else if (nextc == '/') {
            takechar();
            if (nextc == '*') {
              // ??
            }
        }
    }
}

char* my_realloc(char* old, int oldlen, int newlen) {
    char* new = malloc(newlen);
    int i = 0;
    while (i <= oldlen - 1) {
        new[i] = old[i];
        i = i + 1;
    }
    return new;
}

void emit(int n, char* s) {
    if (code_size <= codepos + n) {
        int x = (codepos + n + 10) << 1;
        code = my_realloc(code, code_size, x);
        code_size = x;
    }
    int i = 0;
    while (i <= n - 1) {
        code[codepos] = s[i];
        codepos = codepos + 1;
        i = i + 1;
    }
}

int peek(char *s) {
    int i = 0;
    while (s[i] == token[i] && s[i] != 0) {
        i++;
    }
    return s[i] == token[i];
}



string equality_expr()

string shift_expr()

int emit()
