/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _LINUX_CAMKES_DEBUG_H_
#define _LINUX_CAMKES_DEBUG_H_

#include <stdbool.h>
#include <stdio.h>

/* Name of the current thread. It is up to the caller to initialise this. */
extern __thread const char *thread_name;

/* Whether debugging is currently enabled or not. */
extern bool debug;

#define dprintf(args...) \
    do { \
        if (debug) { \
            fprintf(stderr, "%s: ", thread_name); \
            fprintf(stderr, args); \
        } \
    } while (0)

#endif
