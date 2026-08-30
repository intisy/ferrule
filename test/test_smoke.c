#include "greatest.h"

TEST harness_runs(void) {
    ASSERT_EQ(4, 2 + 2);
    PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(harness_runs);
    GREATEST_MAIN_END();
}
