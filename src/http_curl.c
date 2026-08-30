#include "http.h"

#include "error.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char *data; size_t length; int overflowed; } fr_buffer;

static size_t append(void *chunk, size_t size, size_t count, void *user_data) {
    fr_buffer *buffer = user_data;
    size_t incoming = size * count;
    if (buffer->length + incoming > FR_HTTP_MAX_BODY) {
        buffer->overflowed = 1;
        return 0;
    }
    char *grown = realloc(buffer->data, buffer->length + incoming + 1);
    if (grown == NULL) return 0;
    buffer->data = grown;
    memcpy(buffer->data + buffer->length, chunk, incoming);
    buffer->length += incoming;
    buffer->data[buffer->length] = '\0';
    return incoming;
}

int fr_http_backend_get(const char *url, const fr_http_header *headers, size_t header_count,
                        char **out_body, size_t *out_length, fr_error *err) {
    static int initialised = 0;
    if (!initialised) { curl_global_init(CURL_GLOBAL_DEFAULT); initialised = 1; }

    CURL *handle = curl_easy_init();
    if (handle == NULL) {
        fr_error_set(err, "could not create an http client for %s", url);
        return FR_ERR;
    }

    struct curl_slist *list = NULL;
    for (size_t index = 0; index < header_count; index++) {
        char line[1024];
        snprintf(line, sizeof line, "%s: %s", headers[index].name, headers[index].value);
        list = curl_slist_append(list, line);
    }

    fr_buffer buffer;
    memset(&buffer, 0, sizeof buffer);

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, (long) FR_HTTP_MAX_REDIRECTS);
    /* CURLOPT_UNRESTRICTED_AUTH is deliberately left at its default (0), so
       libcurl drops Authorization (and other caller headers) on a redirect
       that changes host, rather than replaying them to the new host. */
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "ferrule");
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, append);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffer);
    if (list != NULL) curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);

    CURLcode result = curl_easy_perform(handle);
    long status = 0;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(list);
    curl_easy_cleanup(handle);

    if (buffer.overflowed) {
        free(buffer.data);
        fr_error_set(err, "%s returned a body larger than %u bytes", url, FR_HTTP_MAX_BODY);
        return FR_ERR;
    }
    if (result != CURLE_OK) {
        free(buffer.data);
        fr_error_set(err, "%s failed: %s", url, curl_easy_strerror(result));
        return FR_ERR;
    }
    if (status < 200 || status > 299) {
        free(buffer.data);
        fr_error_set(err, "%s returned status %ld", url, status);
        return FR_ERR;
    }

    *out_body = buffer.data;
    *out_length = buffer.length;
    return FR_OK;
}

/* curl_global_cleanup is deliberately never called: a library cannot know it
   is the last user of the process. */
