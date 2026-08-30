#include "greatest.h"
#include "region.h"
#include "sync.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TEMPLATE =
    "dependencies {\n"
    "    // ferrule:begin\n"
    "    // ferrule:end\n"
    "    testImplementation \"junit\"\n"
    "}\n";

static void reset_build_file(void) {
    fr_error err;
    fr_file_write_text("test/fixtures/consumer/build.gradle", TEMPLATE, &err);
}

TEST sync_writes_the_resolved_lines(void) {
    reset_build_file();
    int changed = 0; fr_error err;
    ASSERT_EQ(FR_OK, fr_sync("test/fixtures/consumer/ferrule.json", 1, &changed, &err));
    ASSERT_EQ(1, changed);

    char *text = NULL;
    fr_file_read_text("test/fixtures/consumer/build.gradle", &text, &err);
    ASSERT(strstr(text, "    githubImplementation \"intisy-ai:basekit:5.0.0:contracts\"\n") != NULL);
    ASSERT(strstr(text, "    githubImplementation \"intisy-ai:basekit:5.0.0:ir\"\n") != NULL);
    ASSERT(strstr(text, "testImplementation \"junit\"") != NULL);
    free(text);
    reset_build_file();
    PASS();
}

TEST sync_is_idempotent(void) {
    reset_build_file();
    int changed = 0; fr_error err;
    fr_sync("test/fixtures/consumer/ferrule.json", 1, &changed, &err);
    ASSERT_EQ(FR_OK, fr_sync("test/fixtures/consumer/ferrule.json", 1, &changed, &err));
    ASSERT_EQ(0, changed);
    reset_build_file();
    PASS();
}

TEST check_reports_drift_without_writing(void) {
    reset_build_file();
    int changed = 0; fr_error err;
    ASSERT_EQ(FR_OK, fr_sync("test/fixtures/consumer/ferrule.json", 0, &changed, &err));
    ASSERT_EQ(1, changed);

    char *text = NULL;
    fr_file_read_text("test/fixtures/consumer/build.gradle", &text, &err);
    ASSERT_STR_EQ(TEMPLATE, text);
    free(text);
    reset_build_file();
    PASS();
}

TEST check_is_quiet_when_in_sync(void) {
    reset_build_file();
    int changed = 0; fr_error err;
    fr_sync("test/fixtures/consumer/ferrule.json", 1, &changed, &err);
    ASSERT_EQ(FR_OK, fr_sync("test/fixtures/consumer/ferrule.json", 0, &changed, &err));
    ASSERT_EQ(0, changed);
    reset_build_file();
    PASS();
}

TEST reports_a_build_file_without_markers(void) {
    fr_error err;
    fr_file_write_text("test/fixtures/consumer/build.gradle", "dependencies {\n}\n", &err);
    int changed = 0;
    ASSERT_EQ(FR_ERR, fr_sync("test/fixtures/consumer/ferrule.json", 1, &changed, &err));
    ASSERT(strstr(err.message, "build.gradle") != NULL);
    ASSERT(strstr(err.message, "ferrule:begin") != NULL);
    reset_build_file();
    PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(sync_writes_the_resolved_lines);
    RUN_TEST(sync_is_idempotent);
    RUN_TEST(check_reports_drift_without_writing);
    RUN_TEST(check_is_quiet_when_in_sync);
    RUN_TEST(reports_a_build_file_without_markers);
    GREATEST_MAIN_END();
}
