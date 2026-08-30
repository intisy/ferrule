#include "greatest.h"
#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define rmdir _rmdir
#define make_test_directory(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <unistd.h>
#define make_test_directory(path) mkdir(path, 0777)
#endif

/* Unique per process (not just per test) so a crash mid-test or a second CI job
   running this binary concurrently on the same machine can never share, and thus
   never contend over, another run's cache entries. */
static const char *test_cache_root(void) {
#ifdef _WIN32
    const char *base = getenv("TEMP");
    if (base == NULL) base = getenv("TMP");
    if (base == NULL) base = "C:/Windows/Temp";
    int pid = _getpid();
#else
    const char *base = getenv("TMPDIR");
    if (base == NULL) base = "/tmp";
    int pid = (int) getpid();
#endif
    static char root[512];
    snprintf(root, sizeof root, "%s/ferrule_test_cache_%d", base, pid);
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

static char *dup_or_null(const char *text) {
    if (text == NULL) return NULL;
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    if (copy != NULL) memcpy(copy, text, size);
    return copy;
}

/* project may itself contain one "/" (e.g. "intisy-ai/basekit"), so this
   removes every level fr_cache_path can have created for it: the file, the
   version directory, both project segments, and finally the root. */
static void remove_entry_tree(const char *root, const char *project, const char *version) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s/%s/ferrule.json", root, project, version);
    remove(path);
    snprintf(path, sizeof path, "%s/%s/%s", root, project, version);
    rmdir(path);
    snprintf(path, sizeof path, "%s/%s", root, project);
    rmdir(path);

    const char *slash = strchr(project, '/');
    if (slash != NULL) {
        char prefix[256];
        size_t len = (size_t) (slash - project);
        if (len < sizeof prefix) {
            memcpy(prefix, project, len);
            prefix[len] = '\0';
            snprintf(path, sizeof path, "%s/%s", root, prefix);
            rmdir(path);
        }
    }
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
    set_cache_dir(oversized);

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
    char temp_path[512];
    snprintf(temp_path, sizeof temp_path, "%s/intisy-ai/basekit/5.0.0/ferrule.json.tmp", root);
    FILE *leftover = fopen(temp_path, "rb");
    ASSERT(leftover == NULL);

    remove_entry_tree(root, "intisy-ai/basekit", "5.0.0");
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
    ASSERT_EQ(0, make_test_directory(blocker_path));

    fr_cache_write(project, version, "{\"corrupt");

    char *text = NULL;
    ASSERT_EQ(FR_OK, fr_cache_read(project, version, &text, &err));
    ASSERT(text != NULL);
    ASSERT_STR_EQ("{\"good\":true}", text);
    free(text);

    rmdir(blocker_path);
    free(final_path);
    remove_entry_tree(root, "intisy-ai/atomic-guard", "1.0.0");
    clear_cache_dir();
    PASS();
}

#ifdef _WIN32
TEST resolves_the_localappdata_fallback_when_no_override_is_set(void) {
    clear_cache_dir();
    char *saved = dup_or_null(getenv("LOCALAPPDATA"));

    _putenv_s("LOCALAPPDATA", test_cache_root());

    fr_error err;
    char *path = NULL;
    ASSERT_EQ(FR_OK, fr_cache_path("intisy-ai/basekit", "1.0.0", &path, &err));
    ASSERT(path != NULL);
    char expected_prefix[600];
    snprintf(expected_prefix, sizeof expected_prefix, "%s/ferrule/cache/", test_cache_root());
    ASSERT(strstr(path, expected_prefix) == path);
    free(path);

    if (saved != NULL) { _putenv_s("LOCALAPPDATA", saved); free(saved); }
    else { _putenv_s("LOCALAPPDATA", ""); }
    PASS();
}
#else
TEST resolves_the_xdg_cache_home_fallback_when_no_override_is_set(void) {
    clear_cache_dir();
    char *saved = dup_or_null(getenv("XDG_CACHE_HOME"));

    setenv("XDG_CACHE_HOME", test_cache_root(), 1);

    fr_error err;
    char *path = NULL;
    ASSERT_EQ(FR_OK, fr_cache_path("intisy-ai/basekit", "1.0.0", &path, &err));
    ASSERT(path != NULL);
    char expected_prefix[600];
    snprintf(expected_prefix, sizeof expected_prefix, "%s/ferrule/", test_cache_root());
    ASSERT(strstr(path, expected_prefix) == path);
    free(path);

    if (saved != NULL) { setenv("XDG_CACHE_HOME", saved, 1); free(saved); }
    else { unsetenv("XDG_CACHE_HOME"); }
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
