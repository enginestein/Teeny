#ifndef TEENY_VERSION_H
#define TEENY_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#define TEENY_NAME          "Teeny"
#define TEENY_VERSION_MAJOR 0
#define TEENY_VERSION_MINOR 6
#define TEENY_VERSION_PATCH 0
#define TEENY_PHASE         "8"

#define TEENY_VERSION_STRING "0.6.0"

/* Preprocessor-safe numeric version for comparisons. */
#define TEENY_VERSION_NUM \
    ((TEENY_VERSION_MAJOR << 16) | (TEENY_VERSION_MINOR << 8) | TEENY_VERSION_PATCH)

#ifdef __cplusplus
}
#endif

#endif /* TEENY_VERSION_H */