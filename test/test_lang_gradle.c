#include "greatest.h"
#include "lang_gradle.h"

#include <stdlib.h>
#include <string.h>

static fr_resolved two_modules[2];

static void setup(void) {
    two_modules[0].project = "intisy-ai/basekit";
    two_modules[0].module = "contracts";
    two_modules[0].gradle_coordinate = "intisy-ai:basekit:5.0.0:contracts";
    two_modules[1].project = "intisy-ai/basekit";
    two_modules[1].module = "ir";
    two_modules[1].gradle_coordinate = "intisy-ai:basekit:5.0.0:ir";
}

TEST renders_one_line_per_module_in_order(void) {
    setup();
    fr_consumer consumer = {0};
    consumer.configuration = "githubImplementation";
    char *text = NULL; fr_error err;
    ASSERT_EQ(FR_OK, FR_LANGUAGE_GRADLE.render(FR_LANGUAGE_GRADLE.state, &consumer,
                                               two_modules, 2, &text, &err));
    ASSERT_STR_EQ(
        "githubImplementation \"intisy-ai:basekit:5.0.0:contracts\"\n"
        "githubImplementation \"intisy-ai:basekit:5.0.0:ir\"", text);
    free(text);
    PASS();
}

TEST renders_empty_text_for_no_modules(void) {
    fr_consumer consumer = {0};
    consumer.configuration = "githubImplementation";
    char *text = NULL; fr_error err;
    ASSERT_EQ(FR_OK, FR_LANGUAGE_GRADLE.render(FR_LANGUAGE_GRADLE.state, &consumer, NULL, 0, &text, &err));
    ASSERT_STR_EQ("", text);
    free(text);
    PASS();
}

TEST fails_without_a_configuration(void) {
    fr_consumer consumer = {0};
    char *text = NULL; fr_error err;
    ASSERT_EQ(FR_ERR, FR_LANGUAGE_GRADLE.render(FR_LANGUAGE_GRADLE.state, &consumer, NULL, 0, &text, &err));
    ASSERT(strstr(err.message, "configuration") != NULL);
    PASS();
}

TEST carries_the_expected_capability(void) {
    ASSERT_STR_EQ("ferrule.language/gradle", FR_LANGUAGE_GRADLE.capability);
    PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(renders_one_line_per_module_in_order);
    RUN_TEST(renders_empty_text_for_no_modules);
    RUN_TEST(fails_without_a_configuration);
    RUN_TEST(carries_the_expected_capability);
    GREATEST_MAIN_END();
}
