#include "semver.h"

#include "error.h"

#include <stdio.h>
#include <string.h>

int fr_version_parse(const char *text, fr_version *out, fr_error *err) {
    if (text == NULL) {
        fr_error_set(err, "version is missing");
        return FR_ERR;
    }
    int major = 0, minor = 0, patch = 0;
    char trailing = 0;
    if (sscanf(text, "%d.%d.%d%c", &major, &minor, &patch, &trailing) != 3) {
        fr_error_set(err, "version \"%s\" is not major.minor.patch", text);
        return FR_ERR;
    }
    if (major < 0 || minor < 0 || patch < 0) {
        fr_error_set(err, "version \"%s\" has a negative component", text);
        return FR_ERR;
    }
    out->major = major; out->minor = minor; out->patch = patch;
    return FR_OK;
}

int fr_range_parse(const char *text, fr_range *out, fr_error *err) {
    if (text == NULL) {
        fr_error_set(err, "range is missing");
        return FR_ERR;
    }
    fr_range_kind kind = FR_RANGE_EXACT;
    const char *rest = text;
    if (text[0] == '^') { kind = FR_RANGE_CARET; rest = text + 1; }
    else if (text[0] == '~') { kind = FR_RANGE_TILDE; rest = text + 1; }

    fr_version base;
    if (fr_version_parse(rest, &base, err) != FR_OK) {
        fr_error_set(err, "range \"%s\" is not a supported form", text);
        return FR_ERR;
    }
    out->kind = kind; out->base = base;
    return FR_OK;
}

static int at_least(const fr_version *v, const fr_version *base) {
    if (v->major != base->major) return v->major > base->major;
    if (v->minor != base->minor) return v->minor > base->minor;
    return v->patch >= base->patch;
}

int fr_range_satisfies(const fr_range *range, const fr_version *version) {
    const fr_version *base = &range->base;
    switch (range->kind) {
        case FR_RANGE_EXACT:
            return version->major == base->major
                && version->minor == base->minor
                && version->patch == base->patch;
        case FR_RANGE_CARET:
            return version->major == base->major && at_least(version, base);
        case FR_RANGE_TILDE:
            return version->major == base->major
                && version->minor == base->minor
                && at_least(version, base);
    }
    return 0;
}
