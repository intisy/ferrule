#include "lang_gradle.h"

#include "error.h"

#include <stdio.h>
#include <stdlib.h>

static const char *GRADLE_LINE_FORMAT = "%s \"%s\"";

static int gradle_render(void *state, const fr_consumer *consumer, const fr_resolved *resolved,
                         size_t count, char **out_text, fr_error *err) {
    (void) state;
    *out_text = NULL;

    if (consumer->configuration == NULL || consumer->configuration[0] == '\0') {
        fr_error_set(err, "consumer has no \"configuration\" for the gradle language plugin");
        return FR_ERR;
    }

    size_t total_length = 1;
    for (size_t index = 0; index < count; index++) {
        int line_length = snprintf(NULL, 0, GRADLE_LINE_FORMAT,
                                   consumer->configuration, resolved[index].gradle_coordinate);
        if (line_length < 0) {
            fr_error_set(err, "failed formatting gradle dependency line %zu", index);
            return FR_ERR;
        }
        total_length += (size_t) line_length;
        if (index + 1 < count) total_length += 1;
    }

    char *text = malloc(total_length);
    if (text == NULL) {
        fr_error_set(err, "out of memory rendering %zu gradle dependency lines", count);
        return FR_ERR;
    }

    char *cursor = text;
    size_t remaining = total_length;
    for (size_t index = 0; index < count; index++) {
        int written = snprintf(cursor, remaining, GRADLE_LINE_FORMAT,
                               consumer->configuration, resolved[index].gradle_coordinate);
        if (written < 0) {
            free(text);
            fr_error_set(err, "failed formatting gradle dependency line %zu", index);
            return FR_ERR;
        }
        cursor += written;
        remaining -= (size_t) written;
        if (index + 1 < count) {
            *cursor++ = '\n';
            remaining -= 1;
        }
    }
    *cursor = '\0';

    *out_text = text;
    return FR_OK;
}

const fr_language_plugin FR_LANGUAGE_GRADLE = {
    "ferrule.language/gradle", "// ferrule:begin", "// ferrule:end", gradle_render, NULL
};
