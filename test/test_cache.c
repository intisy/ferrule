#include "greatest.h"
#include "cache.h"
#include "support.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Unique per process (not just per test) so a crash mid-test or a second CI job
   running this binary concurrently on the same machine can never share, and thus
   never contend over, another run's cache entries. */
static const char *test_cache_root(void) {
    static char root[512];
    snprintf(root, sizeof root, "%s/ferrule_test_cache_%d", fr_test_temp_base(), fr_test_process_id());
    return root;
}

/* Every test points the cache at a private temp root before touching
   fr_cache_* so none of them can reach a real user cache directory. */
static void isolate_cache_dir(void) {
    fr_test_set_env("FERRULE_CACHE_DIR", test_cache_root());
}

static void clear_cache_dir(void) {
    fr_test_set_env("FERRULE_CACHE_DIR", NULL);
}

static char *dup_or_null(const char *text) {
    if (text == NULL) return NULL;
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    if (copy != NULL) memcpy(copy, text, size);
    return copy;
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

TEST rejects_a_component_with_a_trailing_dot(void) {
    isolate_cache_dir();

    fr_error err;
    char *path = NULL;
    ASSERT_EQ(FR_ERR, fr_cache_path("intisy-ai/basekit.", "1.0.0", &path, &err));
    ASSERT(path == NULL);

    path = NULL;
    ASSERT_EQ(FR_ERR, fr_cache_path("intisy-ai/basekit", "1.0.0.", &path, &err));
    ASSERT(path == NULL);

    clear_cache_dir();
    PASS();
}

TEST rejects_an_oversized_cache_root_rather_than_truncating_it(void) {
    char oversized[2000];
    memset(oversized, 'x', sizeof oversized - 1);
    oversized[sizeof oversized - 1] = '\0';
    fr_test_set_env("FERRULE_CACHE_DIR", oversized);

    fr_error err;
    char *path = NULL;
    ASSERT_EQ(FR_ERR, fr_cache_path("intisy-ai/basekit", "1.0.0", &path, &err));
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
    const char *root = test_cache_root();

    fr_cache_write("intisy-ai/basekit", "5.0.0", "{\"schema\":1}");
    fr_error err;
    char *text = NULL;
    ASSERT_EQ(FR_OK, fr_cache_read("intisy-ai/basekit", "5.0.0", &text, &err));
    ASSERT(text != NULL);
    ASSERT_STR_EQ("{\"schema\":1}", text);
    free(text);

    /* Proves the successful path leaves no ".tmp" sibling behind: it must
       have been renamed into place, not merely written and abandoned. */
    ASSERT_EQ(0, fr_test_count_files(root, "ferrule.json.tmp"));
    ASSERT_EQ(1, fr_test_count_files(root, "ferrule.json"));

    fr_test_remove_tree(root);
    clear_cache_dir();
    PASS();
}

/* Direct proof of the atomicity fix: pre-populate the cache entry with known
   good content, then force the next write's temp file to be un-openable (by
   pre-creating a directory at the exact ".tmp" path fr_cache_write uses) so
   the write cannot complete. The write must fail silently and the original
   content must still be there afterwards, untouched and not truncated. */
TEST preserves_the_existing_manifest_when_a_write_cannot_complete(void) {
    isolate_cache_dir();
    const char *root = test_cache_root();
    const char *project = "intisy-ai/atomic-guard";
    const char *version = "1.0.0";

    fr_cache_write(project, version, "{\"good\":true}");

    fr_error err;
    char *final_path = NULL;
    ASSERT_EQ(FR_OK, fr_cache_path(project, version, &final_path, &err));

    char blocker_path[512];
    snprintf(blocker_path, sizeof blocker_path, "%s.tmp", final_path);
    ASSERT_EQ(0, fr_test_make_directory(blocker_path));

    fr_cache_write(project, version, "{\"corrupt");

    char *text = NULL;
    ASSERT_EQ(FR_OK, fr_cache_read(project, version, &text, &err));
    ASSERT(text != NULL);
    ASSERT_STR_EQ("{\"good\":true}", text);
    free(text);

    free(final_path);
    fr_test_remove_tree(root);
    clear_cache_dir();
    PASS();
}

#ifdef _WIN32
TEST resolves_the_localappdata_fallback_when_no_override_is_set(void) {
    clear_cache_dir();
    char *saved = dup_or_null(getenv("LOCALAPPDATA"));

    fr_test_set_env("LOCALAPPDATA", test_cache_root());

    fr_error err;
    char *path = NULL;
    ASSERT_EQ(FR_OK, fr_cache_path("intisy-ai/basekit", "1.0.0", &path, &err));
    ASSERT(path != NULL);
    char expected_prefix[600];
    snprintf(expected_prefix, sizeof expected_prefix, "%s/ferrule/cache/", test_cache_root());
    ASSERT(strstr(path, expected_prefix) == path);
    free(path);

    fr_test_set_env("LOCALAPPDATA", saved);
    free(saved);
    PASS();
}
#else
TEST resolves_the_xdg_cache_home_fallback_when_no_override_is_set(void) {
    clear_cache_dir();
    char *saved = dup_or_null(getenv("XDG_CACHE_HOME"));

    fr_test_set_env("XDG_CACHE_HOME", test_cache_root());

    fr_error err;
    char *path = NULL;
    ASSERT_EQ(FR_OK, fr_cache_path("intisy-ai/basekit", "1.0.0", &path, &err));
    ASSERT(path != NULL);
    char expected_prefix[600];
    snprintf(expected_prefix, sizeof expected_prefix, "%s/ferrule/", test_cache_root());
    ASSERT(strstr(path, expected_prefix) == path);
    free(path);

    fr_test_set_env("XDG_CACHE_HOME", saved);
    free(saved);
    PASS();
}
#endif

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(rejects_a_project_that_would_escape_the_cache_root);
    RUN_TEST(rejects_a_component_with_a_trailing_dot);
    RUN_TEST(rejects_an_oversized_cache_root_rather_than_truncating_it);
    RUN_TEST(reports_a_miss_without_an_error);
    RUN_TEST(round_trips_a_written_manifest);
    RUN_TEST(preserves_the_existing_manifest_when_a_write_cannot_complete);
#ifdef _WIN32
    RUN_TEST(resolves_the_localappdata_fallback_when_no_override_is_set);
#else
    RUN_TEST(resolves_the_xdg_cache_home_fallback_when_no_override_is_set);
#endif
    GREATEST_MAIN_END();
}
