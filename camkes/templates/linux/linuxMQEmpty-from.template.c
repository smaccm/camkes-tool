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

/*- set channel = c_symbol() -*/
static channel_t * /*? channel ?*/;
/*- set mutex = c_symbol() -*/
static pthread_mutex_t /*? mutex ?*/;
/*- set cond = c_symbol() -*/
static pthread_cond_t /*? cond ?*/;

void /*? me.from_interface.name ?*/___init(char *working_dir) {
    if (pthread_mutex_init(&/*? mutex ?*/, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(-1);
    }
    if (pthread_cond_init(&/*? cond ?*/, NULL) != 0) {
        perror("pthread_cond_init");
        exit(-1);
    }
    /*? channel ?*/ = channel_init(working_dir, "/*? me.name ?*/.channel");
    if (/*? channel ?*/ == NULL) {
        perror("channel_init");
        exit(-1);
    }
}

int /*? me.from_interface.name ?*/__run(void) {
    int result = pthread_mutex_lock(&/*? mutex ?*/);
    assert(result == 0);
    while (1) {
        result = pthread_cond_wait(&/*? cond ?*/, &/*? mutex ?*/);
        assert(result == 0);

        /* At this point the user could signal us while we're not blocked on
         * the condition variable and this notification could be lost. It's
         * safe to ignore this because this is indistinguishable (by the user)
         * from losing a notification on the receiver's side, which is allowed
         * by the API.
         */

        result = channel_send(/*? channel ?*/, NULL, 0);
        assert(result == 0);
    }
}

void /*? me.from_interface.name ?*/_emit_underlying(void) {
    int result = pthread_mutex_lock(&/*? mutex ?*/);
    assert(result == 0);
    result = pthread_cond_signal(&/*? cond ?*/);
    assert(result == 0);
    result = pthread_mutex_unlock(&/*? mutex ?*/);
    assert(result == 0);
}
