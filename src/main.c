#include "../includes/vm.h"
#include <stdio.h>

// TODO: 391

void read_lines();
void run_file(const char *path);

int main(int argc, const char *argv[]) {
    init_vm();
    if (argc == 1) {
        read_lines();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Error: no path specified\n");
        exit(64);
    }

    free_vm();
    return 0;
}

void run_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Error: invalid path \"%s\"\n", path);
        exit(74);
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *code = (char *)(malloc(size + 1));
    if (!code) {
        fprintf(stderr, "Error: not enough memory available to read file \"%s\"\n", path);
    }

    int end = fread(code, sizeof(char), size, fp);
    if (end < size) {
        fprintf(stderr, "Error: unsuccessful file read \"%s\"\n", path);
    }
    code[end] = '\0';

    fclose(fp);

    InterpretResult_t result = interpret(code);
    if (result == INTERPRET_COMPILE_ERROR) {
        exit(65);
    }
    if (result == INTERPRET_RUNTIME_ERROR) {
        exit(70);
    }
    free(code);
    code = NULL;
}

void read_lines() {
    char line[1024];
    while (true) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        interpret(line);
    }
}
