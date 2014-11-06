/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <sel4/sel4.h>

/*? macros.show_includes(me.to_instance.type.includes) ?*/

/*- set attr = "%s_attributes" % me.from_interface.name -*/
/*- set irq= [] -*/
/*- set aep_obj = alloc_obj('aep', seL4_AsyncEndpointObject) -*/
/*- set aep = alloc_cap('aep', aep_obj, read=True) -*/
/*- for i in configuration.settings -*/
    /*- if attr == i.attribute and i.instance == me.from_instance.name -*/
        /*- set attr_irq, attr_level, attr_trig = i.value.strip('"').split(',') -*/
        /*- set irq_handler = alloc('irq', seL4_IRQControl, number=int(attr_irq), aep=aep_obj) -*/
        /*- do irq.append((irq_handler, int(attr_level), int(attr_trig))) -*/
        /*- break -*/
    /*- endif -*/
/*- endfor -*/
/*- set lock = alloc('lock', seL4_AsyncEndpointObject, read=True, write=True) -*/

#define MAX_CALLBACKS 10

static void (*volatile callbacks[MAX_CALLBACKS])(void*);
static void *callback_args[MAX_CALLBACKS];
static volatile int event_pending;
static volatile int sleepers;

#define CAS __sync_val_compare_and_swap
#define ATOMIC_INCREMENT(ptr) __sync_fetch_and_add((ptr), 1)
#define ATOMIC_DECREMENT(ptr) __sync_fetch_and_sub((ptr), 1)

#define SLEEP() \
    do { \
        ATOMIC_INCREMENT(&sleepers); \
        assert(sleepers > 0); \
        (void)seL4_Wait(/*? lock ?*/, NULL); \
        assert(sleepers > 0); \
        ATOMIC_DECREMENT(&sleepers); \
    } while (0)

#define WAKE() seL4_Notify(/*? lock ?*/, 0 /* ignored */)

int /*? me.to_interface.name ?*/__run(void) {
    /* Set trigger mode */
    seL4_IRQHandler_SetMode(/*? irq[0][0] ?*/, /*? irq[0][1] ?*/, /*? irq[0][2] ?*/);
    while (1) {
        int handled = 0;

        (void)seL4_Wait(/*? aep ?*/, NULL);

        /* First preference: callbacks. */
        if (!handled) {
            for (int i = 0; i < MAX_CALLBACKS; ++i) {
                void (*callback)(void*) = callbacks[i];
                if (callback != NULL) {
                    callbacks[i] = NULL; /* No need for CAS. */
                    callback(callback_args[i]);
                    handled = 1;
                }
            }
        }

        /* There may in fact already be a pending event, but we don't care. */
        event_pending = 1;

        /* Second preference: waiters. */
        if (!handled) {
            if (sleepers > 0) { /* No lock required. */
                WAKE();
                /* Assume one of them will grab it. */
                handled = 1;
            }
        }

        /* Else, leave it for polling. */
    }

    assert(!"Unreachable");
}

int /*? me.to_interface.name ?*/_poll(void) {
    return CAS(&event_pending, 1, 0);
}

void /*? me.to_interface.name ?*/_wait(void) {
    while (!/*? me.to_interface.name ?*/_poll()) {
        SLEEP();
    }
}

int /*? me.to_interface.name ?*/_reg_callback(void (*callback)(void*), void *arg) {
    int error;
    for (int i = 0; i < MAX_CALLBACKS; ++i) {
        if (CAS(&callbacks[i], NULL, callback) == NULL) {
            callback_args[i] = arg;
	    error = seL4_IRQHandler_Ack(/*? irq[0][0] ?*/);
	    assert(!error);
            return 0;
        }
    }
    /* We didn't find an empty slot. */
    return -1;
}
