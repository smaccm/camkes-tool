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
#include </*? me.to_instance.type.name ?*/.h>

/*? macros.show_includes(me.to_instance.type.includes) ?*/

int /*? me.to_interface.name ?*/__run(void) {
    return 0;
}

int /*? me.to_interface.name ?*/_poll(void) {
    SignalSet sigset = SIGNAL_ID(/*? me.to_instance.name ?*/, /*? me.to_interface.name ?*/);
    SignalIdOption received = signal_poll_set(sigset);
    return received != SIGNAL_ID_NONE;
}

void /*? me.to_interface.name ?*/_wait(void) {
    SignalSet sigset = SIGNAL_ID(/*? me.to_instance.name ?*/, /*? me.to_interface.name ?*/);
    (void)signal_poll_wait(sigset);
}
