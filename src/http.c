#include "http.h"

static fr_http_fn ACTIVE_BACKEND = fr_http_backend_get;

fr_http_fn fr_http_set_backend(fr_http_fn backend) {
    fr_http_fn previous = ACTIVE_BACKEND;
    ACTIVE_BACKEND = backend == NULL ? fr_http_backend_get : backend;
    return previous;
}

int fr_http_get(const char *url, const fr_http_header *headers, size_t header_count,
                char **out_body, size_t *out_length, fr_error *err) {
    *out_body = NULL;
    *out_length = 0;
    return ACTIVE_BACKEND(url, headers, header_count, out_body, out_length, err);
}
