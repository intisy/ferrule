#ifndef FERRULE_SYNC_H
#define FERRULE_SYNC_H

#include "types.h"

int fr_sync(const char *manifest_path, int write, int *changed, fr_error *err);

#endif
