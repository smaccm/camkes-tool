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
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/types.h>

/*- set call_channel = c_symbol('call_channel') -*/
static channel_t * /*? call_channel ?*/;
/*- set return_channel = c_symbol('return_channel') -*/
static channel_t * /*? return_channel ?*/;
/*- set lock = c_symbol('lock') -*/
static pthread_mutex_t /*? lock ?*/;

/*- set BUFFER_SIZE = c_symbol('BUFFER_SIZE') -*/
#define /*? BUFFER_SIZE ?*/ 512 /* bytes */

void /*? me.from_interface.name ?*/___init(char *working_dir) {
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

int /*? me.from_interface.name ?*/__run(void) {
    if (pthread_mutex_init(&/*? lock ?*/, NULL) != 0) {
        return -1;
    }
    return 0;
}

/*- for i, m in enumerate(me.from_interface.type.methods) -*/
/*- if m.return_type -*/
    /*? show(m.return_type) ?*/
/*- else -*/
    void
/*- endif -*/
/*? me.from_interface.name ?*/_/*? m.name ?*/(
    /*? ', '.join(map(show, m.parameters)) ?*/
) {
    /*- set base = c_symbol('base') -*/
    char /*? base ?*/ [/*? BUFFER_SIZE ?*/];
    /*- set buffer = c_symbol('buffer') -*/
    void * /*? buffer ?*/ = (void*)/*? base ?*/;

    /* Marshal the method index */
    /*- set call = c_symbol('call') -*/
    int /*? call ?*/ = /*? i ?*/;
    /*? macros.marshal(buffer, call, 'sizeof(int)') ?*/

    /* Marshal all the parameters */
    /*- for p in m.parameters -*/
        /*- if p.direction.direction in ['inout', 'in'] -*/
            /*- if p.array -*/
                /*? macros.marshal_array(buffer, p.name, 'sizeof(%s)' % show(p.type), p.direction.direction == 'inout') ?*/
            /*- elif p.type.type == 'string' -*/
                /*? macros.marshal_string(buffer, p.name, p.direction.direction == 'inout') ?*/
            /*- else -*/
                /*? macros.marshal(buffer, p.name, 'sizeof(%s)' % show(p.type), p.direction.direction == 'inout') ?*/
            /*- endif -*/
        /*- endif -*/
    /*- endfor -*/

    int result = pthread_mutex_lock(&/*? lock ?*/);
    assert(result == 0);
    result = channel_send(/*? call_channel ?*/, /*? base ?*/,
        /*? buffer ?*/ - (void*)/*? base ?*/);
    assert(result == 0);
    result = channel_recv(/*? return_channel ?*/, /*? base ?*/, /*? BUFFER_SIZE ?*/);
    assert(result == 0);
    result = pthread_mutex_unlock(&/*? lock ?*/);
    assert(result == 0);

    /* Unmarshal the response */
    /*? buffer ?*/ = /*? base ?*/;
    /*- if m.return_type -*/
        /*- set ret = c_symbol('ret') -*/
        /*? show(m.return_type) ?*/ /*? ret ?*/;
        /*- if m.return_type.type == 'string' -*/
            /*? macros.unmarshal_string(buffer, ret, True) ?*/
        /*- else -*/
            /*? macros.unmarshal(buffer, ret, 'sizeof(%s)' % show(m.return_type)) ?*/
        /*- endif -*/
    /*- endif -*/

    /*- for p in m.parameters -*/
        /*- if p.direction.direction in ['inout', 'out'] -*/
            /*- if p.array -*/
    	        /*- if p.direction.direction == 'out' -*/
    	            /*? '*%s' % p.name ?*/ = NULL;
    		    /*? '*%s_sz' % p.name ?*/ = 0;
    	        /*- endif -*/
                /*? macros.unmarshal_array(buffer, p.name, 'sizeof(%s)' % show(p.type), True, show(p.type)) ?*/
            /*- elif p.type.type == 'string' -*/
                /*? macros.unmarshal_string(buffer, p.name, p.direction.direction == 'out', True) ?*/
            /*- else -*/
                /*? macros.unmarshal(buffer, p.name, 'sizeof(%s)' % show(p.type), True) ?*/
            /*- endif -*/
        /*- endif -*/
    /*- endfor -*/

    /*- if m.return_type -*/
        return /*? ret ?*/;
    /*- endif -*/
}
/*- endfor -*/
