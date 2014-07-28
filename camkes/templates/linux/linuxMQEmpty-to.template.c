/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*- import 'macros.jinja' as macros -*/
#include <assert.h>
#include <camkes/channel.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

static int pending;
/*- set mutex = c_symbol() -*/
static pthread_mutex_t /*? mutex ?*/;
/*- set cond = c_symbol() -*/
static pthread_cond_t /*? cond ?*/;
/*- set channel = c_symbol() -*/
static channel_t * /*? channel ?*/;

void /*? me.to_interface.name ?*/___init(char *working_dir) {
    if (pthread_mutex_init(& /*? mutex ?*/, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(-1);
    }
    if (pthread_cond_init(& /*? cond ?*/, NULL) != 0) {
        perror("pthread_cond_init");
        exit(-1);
    }
    /*? channel ?*/ = channel_init(working_dir, "/*? me.name ?*/.channel");
    if (/*? channel ?*/ == NULL) {
        perror("channel_init");
        exit(-1);
    }
}

int /*? me.to_interface.name ?*/__run(void) {
    while (1) {
        int result = channel_recv(/*? channel ?*/, NULL, 0);
        assert(result == 0);
        result = pthread_mutex_lock(& /*? mutex ?*/);
        assert(result == 0);
        __sync_or_and_fetch(&pending, 1);
        result = pthread_cond_signal(& /*? cond ?*/);
        assert(result == 0);
        result = pthread_mutex_unlock(& /*? mutex ?*/);
        assert(result == 0);
    }
}

int /*? me.to_interface.name ?*/_poll(void) {
    return __sync_bool_compare_and_swap(&pending, 1, 0);
}

void /*? me.to_interface.name ?*/_wait(void) {
    int result = pthread_mutex_lock(&/*? mutex ?*/);
    assert(result == 0);
    while (!__sync_bool_compare_and_swap(&pending, 1, 0)) {
        result = pthread_cond_wait(&/*? cond ?*/, &/*? mutex ?*/);
        assert(result == 0);
    }
    result = pthread_mutex_unlock(&/*? mutex ?*/);
    assert(result == 0);
}

/* TODO: callbacks */
