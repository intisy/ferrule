#include "greatest.h"
#include "region.h"

#include <stdlib.h>
#include <string.h>

TEST replaces_between_the_markers(void) {
    const char *original =
        "dependencies {\n"
        "    // ferrule:begin\n"
        "    old line\n"
        "    // ferrule:end\n"
        "}\n";
    char *out = NULL; fr_error err;
    ASSERT_EQ(FR_OK, fr_region_replace(original, "// ferrule:begin", "// ferrule:end",
                                       "new line\nsecond line", &out, &err));
    ASSERT_STR_EQ(
        "dependencies {\n"
        "    // ferrule:begin\n"
        "    new line\n"
        "    second line\n"
        "    // ferrule:end\n"
        "}\n", out);
    free(out);
    PASS();
}

TEST empties_the_region_when_the_replacement_is_empty(void) {
    const char *original =
        "// ferrule:begin\n"
        "old\n"
        "// ferrule:end\n";
    char *out = NULL; fr_error err;
    ASSERT_EQ(FR_OK, fr_region_replace(original, "// ferrule:begin", "// ferrule:end", "", &out, &err));
    ASSERT_STR_EQ("// ferrule:begin\n// ferrule:end\n", out);
    free(out);
    PASS();
}

TEST leaves_text_outside_the_region_untouched(void) {
    const char *original =
        "before\n// ferrule:begin\nx\n// ferrule:end\nafter\n";
    char *out = NULL; fr_error err;
    fr_region_replace(original, "// ferrule:begin", "// ferrule:end", "y", &out, &err);
    ASSERT(strstr(out, "before\n") == out);
    ASSERT(strstr(out, "\nafter\n") != NULL);
    free(out);
    PASS();
}

TEST fails_when_the_begin_marker_is_absent(void) {
    char *out = NULL; fr_error err;
    ASSERT_EQ(FR_ERR, fr_region_replace("nothing here\n", "// ferrule:begin", "// ferrule:end", "y", &out, &err));
    ASSERT(strstr(err.message, "ferrule:begin") != NULL);
    PASS();
}

TEST fails_when_the_end_marker_is_absent(void) {
    char *out = NULL; fr_error err;
    ASSERT_EQ(FR_ERR, fr_region_replace("// ferrule:begin\nx\n", "// ferrule:begin", "// ferrule:end", "y", &out, &err));
    ASSERT(strstr(err.message, "ferrule:end") != NULL);
    PASS();
}

TEST fails_when_the_end_marker_precedes_the_begin_marker(void) {
    const char *original = "// ferrule:end\nx\n// ferrule:begin\n";
    char *out = NULL; fr_error err;
    ASSERT_EQ(FR_ERR, fr_region_replace(original, "// ferrule:begin", "// ferrule:end", "y", &out, &err));
    PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(replaces_between_the_markers);
    RUN_TEST(empties_the_region_when_the_replacement_is_empty);
    RUN_TEST(leaves_text_outside_the_region_untouched);
    RUN_TEST(fails_when_the_begin_marker_is_absent);
    RUN_TEST(fails_when_the_end_marker_is_absent);
    RUN_TEST(fails_when_the_end_marker_precedes_the_begin_marker);
    GREATEST_MAIN_END();
}
