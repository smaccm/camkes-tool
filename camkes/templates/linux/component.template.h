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
/*? macros.header_guard(re.sub('[^_a-zA-Z0-9]', '_', options.outfile.name)) ?*/
#include <camkes/dataport.h>

/* Entry point expected to be provided by the user. */
int run(void);

/*- set all_interfaces = me.type.provides + me.type.uses + me.type.emits + me.type.consumes + me.type.dataports -*/

/* Optional init functions provided by the user. */
void pre_init(void) __attribute__((weak));
void post_init(void) __attribute__((weak));
/*- for i in all_interfaces -*/
    void /*? i.name ?*/__init(void) __attribute__((weak));
/*- endfor -*/

/*- for u in me.type.uses -*/
    /*- for m in u.type.methods -*/
        /*- if m.return_type -*/
            /*? show(m.return_type) ?*/
        /*- else -*/
            void
        /*- endif -*/
        /*? u.name ?*/_/*? m.name ?*/(
            /*? ', '.join(map(show, m.parameters)) ?*/)
            /*- if u.optional -*/ __attribute__((weak)) /*- endif -*/;
    /*- endfor -*/
/*- endfor -*/

/*- for c in me.type.consumes -*/
    void /*? c.name ?*/_wait(void)
        /*- if c.optional -*/ __attribute__((weak)) /*- endif -*/;
    int /*? c.name ?*/_poll(void)
        /*- if c.optional -*/ __attribute__((weak)) /*- endif -*/;
    int /*? c.name ?*/_reg_callback(void (*callback)(void*), void *arg)
        /*- if c.optional -*/ __attribute__((weak)) /*- endif -*/;
/*- endfor -*/

/*- for e in me.type.emits -*/
    void /*? e.name ?*/_emit(void);
/*- endfor -*/

/*- for d in me.type.dataports -*/
    extern volatile /*? show(d.type) ?*/ * /*? d.name ?*/;
/*- endfor -*/

#endif
