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
#include <camkes/dataport.h>
#include <camkes/debug.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef UNUSED
    #define UNUSED __attribute__((unused))
#endif
#ifndef NORETURN
    #define NORETURN __attribute__((noreturn))
#endif

static char *working_dir;

typedef struct {
    int (*entry)(void);
    const char *name;
} thread_data_t;

/* Signals that we want to catch in the main thread and propagate to the
 * interface threads.
 */
static int signals_propagate[] = {
    SIGTERM,
    SIGINT,
};

/*- set all_interfaces = me.type.provides + me.type.uses + me.type.emits + me.type.consumes + me.type.dataports -*/

void pre_init(void) __attribute__((weak));
void post_init(void) __attribute__((weak));
/*- for i in all_interfaces -*/
    extern char * /*? i.name ?*/_arg;
    int /*? i.name ?*/__run(void);
    void /*? i.name ?*/__init(void) __attribute__((weak));
/*- endfor -*/
int run(void);

static void parse_arguments(int argc, char **argv) {
    struct option opts[] = {
        {"debug", no_argument, NULL, 'd'},
        {"working", required_argument, NULL, 'w'},
        {NULL, 0, NULL, 0},
    };

    while (1) {
        int c = getopt_long(argc, argv, "dw:", opts, NULL);

        if (c == -1) {
            /* Finished processing options. */
            break;
        }

        switch (c) {
            case 'd':
                debug = true;
                dprintf("Debug mode enabled\n");
                break;

            case 'w':
                dprintf("Working directory: %s\n", optarg);
                working_dir = strdup(optarg);
                if (working_dir == NULL) {
                    perror("strdup");
                    exit(-1);
                }
                break;

            default:
                /* getopt will have given the user an error. */
                exit(-1);
        }
    }

    if (working_dir == NULL) {
        fprintf(stderr, "Missing required argument, --working\n");
        exit(-1);
    }
}

/* Run wrapper for interfaces. */
static void * NORETURN run_wrapper(void *data) {
    thread_data_t *d = data;
    thread_name = d->name;

    /* Allow ourselves to be interrupted by our parent at any time. */
    int result = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    if (result != 0) {
        dprintf("warning: failed to set %s cancelable\n", d->name);
    }

    /* Invoke the generated run function. */
    result = d->entry();

    /* Exit with whatever run gave us. */
    pthread_exit((void*)result);

    assert(!"unreachable");
}

/*- for i in all_interfaces -*/
    static pthread_t /*? i.name ?*/_thread;
/*- endfor -*/

static void parent_handler(int signum) {
    /*- for i in all_interfaces -*/
        pthread_cancel(/*? i.name ?*/_thread);
        dprintf("Cancelling child /*? i.name ?*/ due to signal %d\n", signum);
    /*- endfor -*/
}

int main(int argc, char **argv) {
    /* Note, this gets overwritten in child threads. */
    thread_name = "/*? me.name ?*/";

    parse_arguments(argc, argv);

    /*- for i in all_interfaces -*/
        void /*? i.name ?*/___init(char*);
        /*? i.name ?*/___init(working_dir);
    /*- endfor -*/

    /* Call init functions if they exist. */
    if (pre_init) {
        dprintf("Calling pre_init()\n");
        pre_init();
    }
    /*- for i in all_interfaces -*/
        if (/*? i.name ?*/__init) {
            dprintf("Calling /*? i.name ?*/__init()\n");
            /*? i.name ?*/__init();
        }
    /*- endfor -*/
    if (post_init) {
        dprintf("Calling post_init()\n");
        post_init();
    }

    /* Set up our signal handler. */
    const struct sigaction a = {
        .sa_handler = &parent_handler,
    };
    for (int i = 0; i < sizeof(signals_propagate) / sizeof(int); i++) {
        sigaction(signals_propagate[i], &a, NULL);
    }

    /* Fork off a thread per interface. */
    /*- for i in all_interfaces -*/
        dprintf("Creating thread for interface /*? i.name ?*/\n");
        thread_data_t /*? i.name ?*/_data = {
            .entry = /*? i.name ?*/__run,
            .name = "/*? i.name ?*/",
        };
        pthread_create(&/*? i.name ?*/_thread, NULL, &run_wrapper,
            &/*? i.name ?*/_data);
    /*- endfor -*/

    int result = 0;
    /*- if me.type.control -*/
        /* Call the user's control entry point. */
        dprintf("Calling run()\n");
        result = run();
    /*- endif -*/

    void *child_result UNUSED;
    /* For each interface, wait for it to complete execution and propagate its
     * return value.
     */
    /*- for i in all_interfaces -*/
        dprintf("Waiting on child /*? i.name ?*/...\n");
        pthread_join(/*? i.name ?*/_thread, &child_result);
        result |= (int)child_result;
    /*- endfor -*/

    return result;
}

/*- for e in me.type.emits -*/
    void /*? e.name ?*/_emit_underlying(void) __attribute__((weak));
    void /*? e.name ?*/_emit(void) {
        /* If the interface is not connected, the 'underlying' function will
         * not exist.
         */
        if (/*? e.name ?*/_emit_underlying) {
            /*? e.name ?*/_emit_underlying();
        }
    }
/*- endfor -*/

/*# We need to emit dataports here, rather than in the relevant connection
 *# templates because the dataport may be unconnected, in which case the
 *# connection template will never be invoked.
 #*/
/* Use 4MB alignment, suits both x86 and ARM, being lazy. */
#define MMIO_ALIGN (1 << 12)
/*- for d in me.type.dataports -*/
    char /*? d.name ?*/_data[ROUND_UP(sizeof(/*? show(d.type) ?*/), PAGE_SIZE)]
        __attribute__((aligned(MMIO_ALIGN)))
        __attribute__((externally_visible));
    volatile /*? show(d.type) ?*/ * /*? d.name ?*/ = (volatile /*? show(d.type) ?*/ *) /*? d.name ?*/_data;
/*- endfor -*/

/* Prototypes for functions generated in per-interface files. */
/*- for d in me.type.dataports -*/
    int /*? d.name ?*/_wrap_ptr(dataport_ptr_t *p, void *ptr)
    /*- if d.optional -*/
        __attribute__((weak))
    /*- endif -*/
    ;
/*- endfor -*/
dataport_ptr_t dataport_wrap_ptr(void *ptr) {
    dataport_ptr_t p = { .id = -1 };
    /*- for d in me.type.dataports -*/
        if (
            /*- if d.optional -*/
                /*? d.name ?*/_wrap_ptr != NULL &&
            /*- endif -*/
            /*? d.name ?*/_wrap_ptr(&p, ptr) == 0) {
            return p;
        }
    /*- endfor -*/
    return p;
}

/* Prototypes for functions generated in per-interface files. */
/*- for d in me.type.dataports -*/
    void * /*? d.name ?*/_unwrap_ptr(dataport_ptr_t *p)
    /*- if d.optional -*/
        __attribute__((weak))
    /*- endif -*/
    ;
/*- endfor -*/
void *dataport_unwrap_ptr(dataport_ptr_t p) {
    void *ptr = NULL;
    /*- for d in me.type.dataports -*/
        /*- if d.optional -*/
            if (/*? d.name ?*/_unwrap_ptr != NULL) {
        /*- endif -*/
                ptr = /*? d.name ?*/_unwrap_ptr(&p);
                if (ptr != NULL) {
                    return ptr;
                }
        /*- if d.optional -*/
            }
        /*- endif -*/
    /*- endfor -*/
    return ptr;
}

