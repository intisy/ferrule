#include "greatest.h"
#include "region.h"
#include "sync.h"

#include <stdlib.h>
#include <string.h>

TEST reproduces_both_real_consumers(void) {
    int changed = 0; fr_error err;
    ASSERT_EQ(FR_OK, fr_sync("test/fixtures/live/stub-translator/ferrule.json", 1, &changed, &err));

    char *stub = NULL;
    fr_file_read_text("test/fixtures/live/stub-translator/stub/build.gradle", &stub, &err);
    ASSERT(strstr(stub, "githubImplementation \"intisy-ai:basekit:5.0.0:ir\"") != NULL);
    free(stub);

    char *teavm = NULL;
    fr_file_read_text("test/fixtures/live/stub-translator/teavm-stub/build.gradle", &teavm, &err);
    ASSERT(strstr(teavm, "githubImplementation \"intisy-ai:basekit:5.0.0:ir\"") != NULL);
    free(teavm);
    PASS();
}

TEST the_second_run_changes_nothing(void) {
    int changed = 1; fr_error err;
    fr_sync("test/fixtures/live/stub-translator/ferrule.json", 1, &changed, &err);
    ASSERT_EQ(FR_OK, fr_sync("test/fixtures/live/stub-translator/ferrule.json", 0, &changed, &err));
    ASSERT_EQ(0, changed);
    PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(reproduces_both_real_consumers);
    RUN_TEST(the_second_run_changes_nothing);
    GREATEST_MAIN_END();
}
