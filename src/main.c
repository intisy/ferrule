#include "sync.h"

#include <stdio.h>
#include <string.h>

#define FERRULE_VERSION "0.1.0"

static int run(const char *manifest_path, int write, int use_cache) {
    fr_error err;
    fr_sync_report report;
    int status = 0;
    if (fr_sync(manifest_path, write, use_cache, &report, &err) != FR_OK) {
        fprintf(stderr, "ferrule: %s\n", err.message);
        if (write && report.count > 0) {
            fprintf(stderr, "ferrule: %zu file%s updated before the failure\n",
                    report.count, report.count == 1 ? "" : "s");
        }
        status = 1;
    } else if (write) {
        printf("ferrule: %s\n", report.count > 0 ? "updated" : "already in sync");
    } else if (report.count > 0) {
        for (size_t index = 0; index < report.count; index++) {
            fprintf(stderr, "ferrule: %s is out of date\n", report.files[index]);
        }
        fprintf(stderr, "ferrule: run \"ferrule sync\"\n");
        status = 1;
    } else {
        printf("ferrule: in sync\n");
    }
    fr_sync_report_free(&report);
    return status;
}

int main(int argc, char **argv) {
    char *positional[3];
    int positional_count = 0;
    int use_cache = 1;

    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--no-cache") == 0) {
            use_cache = 0;
        } else if (positional_count < (int) (sizeof positional / sizeof positional[0])) {
            positional[positional_count++] = argv[index];
        }
    }

    const char *manifest_path = positional_count >= 2 ? positional[1] : "ferrule.json";
    if (positional_count >= 1 && strcmp(positional[0], "--version") == 0) {
        printf("ferrule %s\n", FERRULE_VERSION);
        return 0;
    }
    if (positional_count >= 1 && strcmp(positional[0], "sync") == 0) return run(manifest_path, 1, use_cache);
    if (positional_count >= 1 && strcmp(positional[0], "check") == 0) return run(manifest_path, 0, use_cache);
    fprintf(stderr, "usage: ferrule [--version | sync [manifest] | check [manifest]] [--no-cache]\n");
    return 2;
}
