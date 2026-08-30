#include "greatest.h"
#include "region.h"
#include "sync.h"

#include <stdlib.h>
#include <string.h>

static void reset_from_pristine(const char *pristine_path, const char *working_path) {
    fr_error err;
    char *text = NULL;
    fr_file_read_text(pristine_path, &text, &err);
    fr_file_write_text(working_path, text, &err);
    free(text);
}

static void reset_fixtures(void) {
    reset_from_pristine("test/fixtures/live/stub-translator/stub/build.gradle.pristine",
                        "test/fixtures/live/stub-translator/stub/build.gradle");
    reset_from_pristine("test/fixtures/live/stub-translator/teavm-stub/build.gradle.pristine",
                        "test/fixtures/live/stub-translator/teavm-stub/build.gradle");
}

TEST reproduces_both_real_consumers(void) {
    reset_fixtures();
    fr_sync_report report; fr_error err;
    ASSERT_EQ(FR_OK, fr_sync("test/fixtures/live/stub-translator/ferrule.json", 1, &report, &err));
    ASSERT_EQ(2, (int) report.count);
    fr_sync_report_free(&report);

    char *stub = NULL;
    fr_file_read_text("test/fixtures/live/stub-translator/stub/build.gradle", &stub, &err);
    ASSERT(strstr(stub, "githubImplementation \"intisy-ai:basekit:5.0.0:ir\"") != NULL);
    free(stub);

    char *teavm = NULL;
    fr_file_read_text("test/fixtures/live/stub-translator/teavm-stub/build.gradle", &teavm, &err);
    ASSERT(strstr(teavm, "githubImplementation \"intisy-ai:basekit:5.0.0:ir\"") != NULL);
    free(teavm);
    reset_fixtures();
    PASS();
}

TEST the_second_run_changes_nothing(void) {
    reset_fixtures();
    fr_sync_report report; fr_error err;
    fr_sync("test/fixtures/live/stub-translator/ferrule.json", 1, &report, &err);
    fr_sync_report_free(&report);
    ASSERT_EQ(FR_OK, fr_sync("test/fixtures/live/stub-translator/ferrule.json", 0, &report, &err));
    ASSERT_EQ(0, (int) report.count);
    fr_sync_report_free(&report);
    reset_fixtures();
    PASS();
}

TEST check_names_every_drifted_consumer(void) {
    reset_fixtures();
    fr_sync_report report; fr_error err;
    ASSERT_EQ(FR_OK, fr_sync("test/fixtures/live/stub-translator/ferrule.json", 0, &report, &err));
    ASSERT_EQ(2, (int) report.count);
    ASSERT(strstr(report.files[0], "/stub/build.gradle") != NULL);
    ASSERT(strstr(report.files[1], "teavm-stub/build.gradle") != NULL);
    fr_sync_report_free(&report);
    reset_fixtures();
    PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(reproduces_both_real_consumers);
    RUN_TEST(the_second_run_changes_nothing);
    RUN_TEST(check_names_every_drifted_consumer);
    GREATEST_MAIN_END();
}
