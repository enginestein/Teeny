#include "teeny/mode.h"

system_mode_t g_system_mode = MODE_REMOTE;

void system_mode_set(system_mode_t mode)
{
    g_system_mode = mode;
}
