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
#include <camkes/marshal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*- set call_channel = c_symbol('call_channel') -*/
static channel_t * /*? call_channel ?*/;
/*- set return_channel = c_symbol('return_channel') -*/
static channel_t * /*? return_channel ?*/;

/*- set BUFFER_SIZE = c_symbol('BUFFER_SIZE') -*/
#define /*? BUFFER_SIZE ?*/ 512 /* bytes */

void /*? me.to_interface.name ?*/___init(char *working_dir) {
    /*? call_channel ?*/ = channel_init(working_dir, "/*? me.name ?*/.call_channel");
    if (/*? call_channel ?*/ == NULL) {
        perror("channel_init");
        exit(-1);
    }
    /*? return_channel ?*/ = channel_init(working_dir, "/*? me.name ?*/.return_channel");
    if (/*? return_channel ?*/ == NULL) {
        perror("channel_init");
        exit(-1);
    }
}

/*- for m in me.to_interface.type.methods -*/
    extern
    /*- if m.return_type -*/
        /*? show(m.return_type) ?*/
    /*- else -*/
        void
    /*- endif -*/
    /*? me.to_interface.name ?*/_/*? m.name ?*/(
        /*? ', '.join(map(show, m.parameters)) ?*/
    );
/*- endfor -*/

int /*? me.to_interface.name ?*/__run(void) {
    /*- set base = c_symbol('base') -*/
    char /*? base ?*/ [/*? BUFFER_SIZE ?*/];

    while (1) {
        /*- set buffer = c_symbol('buffer') -*/
        void * /*? buffer ?*/ = (void*)/*? base ?*/;

        if (channel_recv(/*? call_channel ?*/, (void*)/*? base ?*/,
                /*? BUFFER_SIZE ?*/) != 0) {
            perror("channel_recv");
            exit(-1);
        }

        /*- set call = c_symbol('call') -*/
        int /*? call ?*/;
        /*? macros.unmarshal(buffer, call, 'sizeof(int)') ?*/

        switch (/*? call ?*/) {
            /*- for i, m in enumerate(me.to_interface.type.methods) -*/
                case /*? i ?*/: { /*? '/' + '* ' + m.name + ' *' + '/' ?*/
                    /* Unmarshal parameters */
                    /*- for p in m.parameters -*/

                        /*# Declare parameters. #*/
                        /*- if p.array -*/
                            size_t /*? p.name ?*/_sz = 0;
                        /*- endif -*/
                        /*? show(p.type) ?*/ /*- if p.array -*/ * /*- endif -*/ /*? p.name ?*/;

                        /*- if p.direction.direction in ['inout', 'in'] -*/
                            /*- if p.array -*/
                                /*? macros.unmarshal_array(buffer, p.name, 'sizeof(%s)' % show(p.type), False, show(p.type)) ?*/
                            /*- else -*/
                                /*- if p.type.type == 'string' -*/
                                    /*? macros.unmarshal_string(buffer, p.name, True) ?*/
                                /*- else -*/
                                    /*? macros.unmarshal(buffer, p.name, 'sizeof(%s)' % show(p.type)) ?*/
                                /*- endif -*/
                            /*- endif -*/
                        /*- endif -*/
                    /*- endfor -*/

                    /* Call the implementation */
                    /*- if m.return_type -*/
                        /*- set ret = c_symbol('ret') -*/
                        /*? show(m.return_type) ?*/ /*? ret ?*/ =
                    /*- endif -*/
                    /*? me.to_interface.name ?*/_/*? m.name ?*/(
                        /*- for p in m.parameters -*/
                            /*- if p.array -*/
                                /*- if p.direction.direction in ['inout', 'out'] -*/
                                    &
                                /*- endif -*/
                                /*? p.name ?*/_sz,
                            /*- endif -*/
                            /*- if p.direction.direction in ['inout', 'out'] -*/
                                &
                            /*- endif -*/
                            /*? p.name ?*/
                            /*- if not loop.last -*/,/*- endif -*/
                        /*- endfor -*/
                    );

                    /* Marshal the response */
                    /*? buffer ?*/ = (void*)/*? base ?*/;
                    /*- if m.return_type -*/
                        /*- if m.return_type.type == 'string' -*/
                            /*? macros.marshal_string(buffer, ret) ?*/
                        /*- else -*/
                            /*? macros.marshal(buffer, ret, 'sizeof(%s)' % show(m.return_type)) ?*/
                        /*- endif -*/
                    /*- endif -*/
                    /*- for p in m.parameters -*/
                        /*- if p.direction.direction in ['inout', 'out'] -*/
                            /*- if p.array -*/
                                /*? macros.marshal_array(buffer, p.name, 'sizeof(%s)' % show(p.type)) ?*/
                            /*- elif p.type.type == 'string' -*/
                                /*? macros.marshal_string(buffer, p.name) ?*/
                            /*- else -*/
                                /*? macros.marshal(buffer, p.name, 'sizeof(%s)' % show(p.type)) ?*/
                            /*- endif -*/
                        /*- endif -*/
                    /*- endfor -*/

                    /* Send the response */
                    if (channel_send(/*? return_channel ?*/,
                            (void*)/*? base ?*/, /*? buffer ?*/ -
                            (void*)/*? base ?*/) != 0) {
                        perror("channel_send");
                        exit(-1);
                    }

                    /* Free any malloced variables */
                    /*- for p in m.parameters -*/
                        /*- if p.array or p.type.type == 'string' -*/
                            free(/*? p.name ?*/);
                        /*- endif -*/
                    /*- endfor -*/

                    break;
                }
            /*- endfor -*/
            default: {
                /*# TODO: Handle error #*/
                assert(0);
            }
        }
    }

    assert(!"Unreachable");
}
