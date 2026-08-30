#include "sync.h"

#include <stdio.h>
#include <string.h>

#define FERRULE_VERSION "0.1.0"

static int run(const char *manifest_path, int write) {
    fr_error err;
    int changed = 0;
    if (fr_sync(manifest_path, write, &changed, &err) != FR_OK) {
        fprintf(stderr, "ferrule: %s\n", err.message);
        return 1;
    }
    if (write) {
        printf("ferrule: %s\n", changed ? "updated" : "already in sync");
        return 0;
    }
    if (changed) {
        fprintf(stderr, "ferrule: manifests are out of date, run \"ferrule sync\"\n");
        return 1;
    }
    printf("ferrule: in sync\n");
    return 0;
}

int main(int argc, char **argv) {
    const char *manifest_path = argc >= 3 ? argv[2] : "ferrule.json";
    if (argc >= 2 && strcmp(argv[1], "--version") == 0) {
        printf("ferrule %s\n", FERRULE_VERSION);
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "sync") == 0) return run(manifest_path, 1);
    if (argc >= 2 && strcmp(argv[1], "check") == 0) return run(manifest_path, 0);
    fprintf(stderr, "usage: ferrule [--version | sync [manifest] | check [manifest]]\n");
    return 2;
}
