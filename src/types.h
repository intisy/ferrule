#ifndef FERRULE_TYPES_H
#define FERRULE_TYPES_H

#define FR_OK 0
#define FR_ERR 1

typedef struct { char message[512]; } fr_error;

typedef struct { int major, minor, patch; } fr_version;

typedef enum { FR_RANGE_EXACT, FR_RANGE_CARET, FR_RANGE_TILDE } fr_range_kind;

typedef struct { fr_range_kind kind; fr_version base; } fr_range;

#endif
