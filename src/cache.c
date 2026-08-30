#include "cache.h"

#include "error.h"
#include "region.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

static int CACHE_ENABLED = 1;

static int is_safe_component(const char *text, int allow_one_slash) {
    if (text == NULL || text[0] == '\0') return 0;
    if (text[0] == '/' || text[0] == '\\') return 0;
    if (strstr(text, "..") != NULL) return 0;
    if (strchr(text, '\\') != NULL) return 0;
    if (strchr(text, ':') != NULL) return 0;

    const char *slash = strchr(text, '/');
    if (slash == NULL) return 1;
    if (!allow_one_slash) return 0;
    return strchr(slash + 1, '/') == NULL && slash[1] != '\0';
}

static int cache_root_dir(char *out, size_t out_size, fr_error *err) {
    const char *override = getenv("FERRULE_CACHE_DIR");
    if (override != NULL && override[0] != '\0') {
        snprintf(out, out_size, "%s", override);
        return FR_OK;
    }

#ifdef _WIN32
    const char *local_appdata = getenv("LOCALAPPDATA");
    if (local_appdata != NULL && local_appdata[0] != '\0') {
        snprintf(out, out_size, "%s/ferrule/cache", local_appdata);
        return FR_OK;
    }
#else
    const char *xdg_cache_home = getenv("XDG_CACHE_HOME");
    if (xdg_cache_home != NULL && xdg_cache_home[0] != '\0') {
        snprintf(out, out_size, "%s/ferrule", xdg_cache_home);
        return FR_OK;
    }
    const char *home = getenv("HOME");
    if (home != NULL && home[0] != '\0') {
        snprintf(out, out_size, "%s/.cache/ferrule", home);
        return FR_OK;
    }
#endif

    fr_error_set(err, "no cache directory available: set FERRULE_CACHE_DIR");
    return FR_ERR;
}

int fr_cache_path(const char *project, const char *version, char **out_path, fr_error *err) {
    *out_path = NULL;
    if (!is_safe_component(project, 1)) {
        fr_error_set(err, "project \"%s\" is not a safe cache path component", project);
        return FR_ERR;
    }
    if (!is_safe_component(version, 0)) {
        fr_error_set(err, "version \"%s\" is not a safe cache path component", version);
        return FR_ERR;
    }

    char root[1024];
    if (cache_root_dir(root, sizeof root, err) != FR_OK) return FR_ERR;

    size_t length = strlen(root) + 1 + strlen(project) + 1 + strlen(version) + 1 + strlen("ferrule.json") + 1;
    char *path = malloc(length);
    if (path == NULL) {
        fr_error_set(err, "out of memory building cache path");
        return FR_ERR;
    }
    snprintf(path, length, "%s/%s/%s/ferrule.json", root, project, version);

    *out_path = path;
    return FR_OK;
}

int fr_cache_read(const char *project, const char *version, char **out_text, fr_error *err) {
    *out_text = NULL;
    if (!CACHE_ENABLED) return FR_OK;

    char *path = NULL;
    if (fr_cache_path(project, version, &path, err) != FR_OK) return FR_ERR;

    FILE *probe = fopen(path, "rb");
    if (probe == NULL) { free(path); return FR_OK; }
    fclose(probe);

    int result = fr_file_read_text(path, out_text, err);
    free(path);
    return result;
}

static void make_directory(const char *path) {
#ifdef _WIN32
    if (_mkdir(path) == 0 || errno == EEXIST) return;
#else
    if (mkdir(path, 0777) == 0 || errno == EEXIST) return;
#endif
}

/* Walks every '/' in the full cache-entry path, creating each ancestor
   directory in turn (root included). Failures are ignored here: a directory
   that could not be created simply makes the later fopen fail too, and that
   failure is swallowed by the caller, never the caller's caller. */
static void make_parent_directories(char *path) {
    for (char *slash = strchr(path, '/'); slash != NULL; slash = strchr(slash + 1, '/')) {
        *slash = '\0';
        make_directory(path);
        *slash = '/';
    }
}

void fr_cache_write(const char *project, const char *version, const char *text) {
    if (!CACHE_ENABLED) return;

    fr_error err;
    char *path = NULL;
    if (fr_cache_path(project, version, &path, &err) != FR_OK) return;

    make_parent_directories(path);

    FILE *file = fopen(path, "wb");
    free(path);
    if (file == NULL) return;

    size_t length = strlen(text);
    (void) fwrite(text, 1, length, file);
    fclose(file);
}
