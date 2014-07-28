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
#include </*? me.from_instance.type.name ?*/.h>

/*? macros.show_includes(me.from_instance.type.includes) ?*/

int /*? me.from_interface.name ?*/__run(void) {
    return 0;
}

void /*? me.from_interface.name ?*/_emit_underlying(void) {
    TaskId task = TASK_ID(/*? me.to_instance.name ?*/);
    SignalSet sigset = SIGNAL_ID(/*? me.to_instance.name ?*/, /*? me.to_interface.name ?*/);
    signal_send_set(task, sigset);
}
