/*
 * main.c -- Teeny entry point.
 *
 * All behaviour lives in the kernel; main only proves the boot path.
 */

#include "teeny/kernel.h"

int main(void)
{
    kernel_run();
    /* kernel_run never returns */
    return 0;
}