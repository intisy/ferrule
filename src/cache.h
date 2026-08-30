#ifndef FERRULE_CACHE_H
#define FERRULE_CACHE_H

#include "types.h"

int fr_cache_path(const char *project, const char *version, char **out_path, fr_error *err);
int fr_cache_read(const char *project, const char *version, char **out_text, fr_error *err);
void fr_cache_write(const char *project, const char *version, const char *text);

#endif
