#ifndef TEENY_MODE_H
#define TEENY_MODE_H

typedef enum {
    MODE_REMOTE   = 0,
    MODE_HARDWARE = 1,
} system_mode_t;

extern system_mode_t g_system_mode;

void system_mode_set(system_mode_t mode);

#endif /* TEENY_MODE_H */
