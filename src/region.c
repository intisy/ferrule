#include "region.h"

#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *line_start(const char *base, const char *pos) {
    while (pos > base && pos[-1] != '\n') pos--;
    return pos;
}

/* Scans replacement into indented lines, prefixing each with indent_start[0..indent_len).
   Called once with dest == NULL to size the output, then once more to fill it, so the
   two passes can never disagree about how many bytes a line contributes. */
static size_t emit_region_body(char *dest, const char *replacement,
                               const char *indent_start, size_t indent_len) {
    if (replacement[0] == '\0') return 0;

    size_t written = 0;
    const char *cursor = replacement;
    for (;;) {
        const char *newline = strchr(cursor, '\n');
        size_t line_len = newline != NULL ? (size_t) (newline - cursor) : strlen(cursor);

        if (dest != NULL) memcpy(dest + written, indent_start, indent_len);
        written += indent_len;
        if (dest != NULL) memcpy(dest + written, cursor, line_len);
        written += line_len;
        if (dest != NULL) dest[written] = '\n';
        written += 1;

        if (newline == NULL) break;
        cursor = newline + 1;
    }
    return written;
}

int fr_region_replace(const char *original, const char *begin_marker, const char *end_marker,
                      const char *replacement, char **out, fr_error *err) {
    *out = NULL;

    const char *begin_pos = strstr(original, begin_marker);
    if (begin_pos == NULL) {
        fr_error_set(err, "region marker \"%s\" not found", begin_marker);
        return FR_ERR;
    }

    const char *begin_marker_end = begin_pos + strlen(begin_marker);
    const char *end_pos = strstr(begin_marker_end, end_marker);
    if (end_pos == NULL) {
        fr_error_set(err, "region marker \"%s\" not found after \"%s\"", end_marker, begin_marker);
        return FR_ERR;
    }

    const char *indent_start = line_start(original, begin_pos);
    size_t indent_len = (size_t) (begin_pos - indent_start);

    const char *begin_newline = strchr(begin_marker_end, '\n');
    const char *prefix_end = begin_newline != NULL ? begin_newline + 1 : begin_marker_end;
    size_t prefix_len = (size_t) (prefix_end - original);

    const char *suffix_start = line_start(original, end_pos);
    size_t suffix_len = strlen(suffix_start);

    size_t body_len = emit_region_body(NULL, replacement, indent_start, indent_len);

    char *buffer = malloc(prefix_len + body_len + suffix_len + 1);
    if (buffer == NULL) {
        fr_error_set(err, "out of memory replacing region");
        return FR_ERR;
    }

    memcpy(buffer, original, prefix_len);
    emit_region_body(buffer + prefix_len, replacement, indent_start, indent_len);
    memcpy(buffer + prefix_len + body_len, suffix_start, suffix_len);
    buffer[prefix_len + body_len + suffix_len] = '\0';

    *out = buffer;
    return FR_OK;
}

int fr_file_read_text(const char *path, char **out, fr_error *err) {
    *out = NULL;
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fr_error_set(err, "cannot open \"%s\"", path);
        return FR_ERR;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size < 0) {
        fclose(file);
        fr_error_set(err, "cannot size \"%s\"", path);
        return FR_ERR;
    }

    char *buffer = malloc((size_t) size + 1);
    if (buffer == NULL) {
        fclose(file);
        fr_error_set(err, "out of memory reading \"%s\"", path);
        return FR_ERR;
    }
    size_t read = fread(buffer, 1, (size_t) size, file);
    fclose(file);
    buffer[read] = '\0';

    *out = buffer;
    return FR_OK;
}

int fr_file_write_text(const char *path, const char *text, fr_error *err) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        fr_error_set(err, "cannot open \"%s\" for writing", path);
        return FR_ERR;
    }
    size_t length = strlen(text);
    size_t written = fwrite(text, 1, length, file);
    fclose(file);
    if (written != length) {
        fr_error_set(err, "cannot write \"%s\"", path);
        return FR_ERR;
    }
    return FR_OK;
}
