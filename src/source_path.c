#include "source_path.h"

#include "error.h"
#include "manifest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const fr_source *find_source(const fr_manifest *manifest, const char *project) {
    for (size_t index = 0; index < manifest->source_count; index++) {
        if (strcmp(manifest->sources[index].project, project) == 0) return &manifest->sources[index];
    }
    return NULL;
}

static char *join_manifest_path(const char *base_dir, const char *relative_path) {
    size_t length = strlen(base_dir) + 1 + strlen(relative_path) + 1 + strlen("ferrule.json") + 1;
    char *joined = malloc(length);
    if (joined != NULL) snprintf(joined, length, "%s/%s/ferrule.json", base_dir, relative_path);
    return joined;
}

static int source_path_load(void *state, const char *project, const char *base_dir,
                            fr_project *out, fr_error *err) {
    memset(out, 0, sizeof *out);
    const fr_manifest *manifest = (const fr_manifest *) state;
    const fr_source *source = find_source(manifest, project);
    if (source == NULL) {
        fr_error_set(err, "sources has no entry for \"%s\"", project);
        return FR_ERR;
    }

    char *file_path = join_manifest_path(base_dir, source->path);
    if (file_path == NULL) {
        fr_error_set(err, "out of memory joining path for \"%s\"", project);
        return FR_ERR;
    }
    int result = fr_project_read(file_path, out, err);
    free(file_path);
    return result;
}

const fr_source_plugin FR_SOURCE_PATH = { "ferrule.source/path", source_path_load, NULL };
