#include "greatest.h"
#include "http.h"

#include <stdlib.h>
#include <string.h>

static const char *LAST_URL = NULL;

static int stub_get(const char *url, const fr_http_header *headers, size_t header_count,
                    char **out_body, size_t *out_length, fr_error *err) {
    (void) headers; (void) header_count; (void) err;
    LAST_URL = url;
    const char *body = "{}";
    *out_length = strlen(body);
    *out_body = malloc(*out_length + 1);
    memcpy(*out_body, body, *out_length + 1);
    return FR_OK;
}

TEST routes_through_the_installed_backend(void) {
    fr_http_fn previous = fr_http_set_backend(stub_get);
    char *body = NULL;
    size_t length = 0;
    fr_error err;
    ASSERT_EQ(FR_OK, fr_http_get("https://example.invalid/x", NULL, 0, &body, &length, &err));
    ASSERT_STR_EQ("https://example.invalid/x", LAST_URL);
    ASSERT_STR_EQ("{}", body);
    ASSERT_EQ(2u, (unsigned) length);
    free(body);
    fr_http_set_backend(previous);
    PASS();
}

TEST restores_the_previous_backend(void) {
    fr_http_fn original = fr_http_set_backend(stub_get);
    fr_http_fn installed = fr_http_set_backend(original);
    ASSERT(installed == stub_get);
    PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(routes_through_the_installed_backend);
    RUN_TEST(restores_the_previous_backend);
    GREATEST_MAIN_END();
}
