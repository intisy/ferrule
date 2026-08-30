#include <stdio.h>
#include <string.h>

#define FERRULE_VERSION "0.1.0"

int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "--version") == 0) {
        printf("ferrule %s\n", FERRULE_VERSION);
        return 0;
    }
    fprintf(stderr, "usage: ferrule --version\n");
    return 2;
}
