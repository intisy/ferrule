#include "greatest.h"
#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define rmdir _rmdir
#else
#include <unistd.h>
#endif

static const char *test_cache_root(void) {
#ifdef _WIN32
    const char *base = getenv("TEMP");
    if (base == NULL) base = getenv("TMP");
    if (base == NULL) base = "C:/Windows/Temp";
#else
    const char *base = getenv("TMPDIR");
    if (base == NULL) base = "/tmp";
#endif
    static char root[512];
    snprintf(root, sizeof root, "%s/ferrule_test_cache", base);
    return root;
}

static void set_cache_dir(const char *path) {
#ifdef _WIN32
    _putenv_s("FERRULE_CACHE_DIR", path);
#else
    setenv("FERRULE_CACHE_DIR", path, 1);
#endif
}

static void clear_cache_dir(void) {
#ifdef _WIN32
    _putenv_s("FERRULE_CACHE_DIR", "");
#else
    unsetenv("FERRULE_CACHE_DIR");
#endif
}

/* Every test points the cache at a private temp root before touching
   fr_cache_* so none of them can reach a real user cache directory. */
static void isolate_cache_dir(void) {
    set_cache_dir(test_cache_root());
}

static void remove_test_cache_tree(void) {
    const char *root = test_cache_root();
    char path[512];
    snprintf(path, sizeof path, "%s/intisy-ai/basekit/5.0.0/ferrule.json", root);
    remove(path);
    snprintf(path, sizeof path, "%s/intisy-ai/basekit/5.0.0", root);
    rmdir(path);
    snprintf(path, sizeof path, "%s/intisy-ai/basekit", root);
    rmdir(path);
    snprintf(path, sizeof path, "%s/intisy-ai", root);
    rmdir(path);
    rmdir(root);
}

TEST rejects_a_project_that_would_escape_the_cache_root(void) {
    isolate_cache_dir();

    fr_error err;
    char *path = NULL;
    ASSERT_EQ(FR_ERR, fr_cache_path("../evil", "1.0.0", &path, &err));
    ASSERT(strstr(err.message, "..") != NULL);
    ASSERT(path == NULL);

    clear_cache_dir();
    PASS();
}

TEST reports_a_miss_without_an_error(void) {
    isolate_cache_dir();

    fr_error err;
    char *text = NULL;
    ASSERT_EQ(FR_OK, fr_cache_read("intisy-ai/absent", "9.9.9", &text, &err));
    ASSERT(text == NULL);

    clear_cache_dir();
    PASS();
}

TEST round_trips_a_written_manifest(void) {
    isolate_cache_dir();

    fr_cache_write("intisy-ai/basekit", "5.0.0", "{\"schema\":1}");
    fr_error err;
    char *text = NULL;
    ASSERT_EQ(FR_OK, fr_cache_read("intisy-ai/basekit", "5.0.0", &text, &err));
    ASSERT(text != NULL);
    ASSERT_STR_EQ("{\"schema\":1}", text);
    free(text);

    remove_test_cache_tree();
    clear_cache_dir();
    PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(rejects_a_project_that_would_escape_the_cache_root);
    RUN_TEST(reports_a_miss_without_an_error);
    RUN_TEST(round_trips_a_written_manifest);
    GREATEST_MAIN_END();
}
