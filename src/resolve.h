#ifndef FERRULE_RESOLVE_H
#define FERRULE_RESOLVE_H

#include "types.h"
#include "registry.h"

#include <stddef.h>

struct fr_resolved {
    char *project;
    char *module;
    char *gradle_coordinate;
};

int fr_resolve_consumer(const fr_consumer *consumer, const char *manifest_dir,
                        const fr_registry *registry,
                        fr_resolved **out, size_t *count, fr_error *err);
void fr_resolved_free(fr_resolved *items, size_t count);

#endif
