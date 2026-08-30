#include "sync.h"

#include "error.h"
#include "lang_gradle.h"
#include "manifest.h"
#include "region.h"
#include "registry.h"
#include "resolve.h"
#include "source_path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FR_REGION_BEGIN "// ferrule:begin"
#define FR_REGION_END "// ferrule:end"

static char *manifest_directory(const char *manifest_path) {
    const char *last_slash = strrchr(manifest_path, '/');
    const char *last_backslash = strrchr(manifest_path, '\\');
    const char *last = last_slash;
    if (last_backslash != NULL && (last == NULL || last_backslash > last)) last = last_backslash;

    size_t length = last != NULL ? (size_t) (last - manifest_path) : 0;
    char *dir = malloc(length + 1);
    if (dir != NULL) {
        memcpy(dir, manifest_path, length);
        dir[length] = '\0';
    }
    return dir;
}

static char *join_path(const char *dir, const char *file) {
    const char *base = (dir == NULL || dir[0] == '\0') ? "." : dir;
    size_t length = strlen(base) + 1 + strlen(file) + 1;
    char *joined = malloc(length);
    if (joined != NULL) snprintf(joined, length, "%s/%s", base, file);
    return joined;
}

static void wrap_error_with_file(fr_error *err, const char *target_path) {
    char original[sizeof err->message];
    memcpy(original, err->message, sizeof original);
    fr_error_set(err, "%s: %s", target_path, original);
}

static int build_registry(fr_registry **out, fr_error *err) {
    fr_registry *registry = fr_registry_create();
    if (registry == NULL) {
        fr_error_set(err, "out of memory creating the plugin registry");
        return FR_ERR;
    }
    if (fr_registry_add_source(registry, &FR_SOURCE_PATH, err) != FR_OK) {
        fr_registry_destroy(registry);
        return FR_ERR;
    }
    if (fr_registry_add_language(registry, &FR_LANGUAGE_GRADLE, err) != FR_OK) {
        fr_registry_destroy(registry);
        return FR_ERR;
    }
    *out = registry;
    return FR_OK;
}

static int sync_consumer(const fr_manifest *manifest, const fr_consumer *consumer,
                         const char *manifest_dir, const fr_registry *registry,
                         int write, int *changed, fr_error *err) {
    char *target_path = join_path(manifest_dir, consumer->file);
    if (target_path == NULL) {
        fr_error_set(err, "out of memory building the target path for consumer \"%s\"", consumer->id);
        return FR_ERR;
    }

    fr_resolved *resolved = NULL;
    size_t count = 0;
    if (fr_resolve_consumer(manifest, consumer, manifest_dir, registry, &resolved, &count, err) != FR_OK) {
        wrap_error_with_file(err, target_path);
        free(target_path);
        return FR_ERR;
    }

    char capability[256];
    snprintf(capability, sizeof capability, "ferrule.language/%s", consumer->language);
    const fr_language_plugin *language = fr_registry_language(registry, capability);
    if (language == NULL) {
        fr_error_set(err, "%s: no language plugin registered for \"%s\"", target_path, consumer->language);
        fr_resolved_free(resolved, count);
        free(target_path);
        return FR_ERR;
    }

    char *rendered = NULL;
    if (language->render(language->state, consumer, resolved, count, &rendered, err) != FR_OK) {
        wrap_error_with_file(err, target_path);
        fr_resolved_free(resolved, count);
        free(target_path);
        return FR_ERR;
    }
    fr_resolved_free(resolved, count);

    char *original_text = NULL;
    if (fr_file_read_text(target_path, &original_text, err) != FR_OK) {
        wrap_error_with_file(err, target_path);
        free(rendered);
        free(target_path);
        return FR_ERR;
    }

    char *replaced = NULL;
    if (fr_region_replace(original_text, FR_REGION_BEGIN, FR_REGION_END, rendered, &replaced, err) != FR_OK) {
        wrap_error_with_file(err, target_path);
        free(original_text);
        free(rendered);
        free(target_path);
        return FR_ERR;
    }
    free(rendered);

    if (strcmp(original_text, replaced) != 0) {
        *changed = 1;
        if (write) {
            if (fr_file_write_text(target_path, replaced, err) != FR_OK) {
                wrap_error_with_file(err, target_path);
                free(original_text);
                free(replaced);
                free(target_path);
                return FR_ERR;
            }
        }
    }

    free(original_text);
    free(replaced);
    free(target_path);
    return FR_OK;
}

int fr_sync(const char *manifest_path, int write, int *changed, fr_error *err) {
    *changed = 0;

    fr_manifest manifest;
    if (fr_manifest_read(manifest_path, &manifest, err) != FR_OK) return FR_ERR;

    char *manifest_dir = manifest_directory(manifest_path);
    if (manifest_dir == NULL) {
        fr_error_set(err, "out of memory deriving the manifest directory");
        fr_manifest_free(&manifest);
        return FR_ERR;
    }

    fr_registry *registry = NULL;
    if (build_registry(&registry, err) != FR_OK) {
        free(manifest_dir);
        fr_manifest_free(&manifest);
        return FR_ERR;
    }

    int result = FR_OK;
    for (size_t index = 0; index < manifest.consumer_count; index++) {
        if (sync_consumer(&manifest, &manifest.consumers[index], manifest_dir, registry,
                          write, changed, err) != FR_OK) {
            result = FR_ERR;
            break;
        }
    }

    fr_registry_destroy(registry);
    free(manifest_dir);
    fr_manifest_free(&manifest);
    return result;
}
