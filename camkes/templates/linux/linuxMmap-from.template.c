/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <camkes/dataport.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

/*- set fd = c_symbol('fd') -*/
static int /*? fd ?*/;

extern volatile /*? show(me.from_interface.type) ?*/ * /*? me.from_interface.name ?*/;

void /*? me.from_interface.name ?*/___init(char *working_dir) {
    /*- set path = c_symbol() -*/
    char * /*? path ?*/ = (char*)malloc(strlen(working_dir) + strlen("//*? me.name ?*/") + 1);
    if (/*? path ?*/ == NULL) {
        perror("malloc");
        exit(-1);
    }
    sprintf(/*? path ?*/, "%s//*? me.name ?*/", working_dir);
    /*? fd ?*/ = open(/*? path ?*/, O_CREAT|O_RDWR, 0666);
    if (/*? fd ?*/ == -1) {
        perror("open");
        exit(-1);
    }

    /* Zero out enough of the file to cover the dataport. */
    /*- set zero = c_symbol() -*/
    /*? show(me.from_interface.type) ?*/ /*? zero ?*/;
    memset(&/*? zero ?*/, 0, sizeof(/*? zero ?*/));
    if (write(/*? fd ?*/, &/*? zero ?*/, sizeof(/*? zero ?*/)) != sizeof(/*? zero ?*/)) {
        perror("write");
        exit(-1);
    }
    lseek(/*? fd ?*/, 0, SEEK_SET);

    /*? me.from_interface.name ?*/ = (volatile /*? show(me.from_interface.type) ?*/ *)
        mmap(NULL, sizeof(/*? show(me.from_interface.type) ?*/),
        PROT_READ|PROT_WRITE, MAP_SHARED, /*? fd ?*/, 0);
    if (/*? me.from_interface.name ?*/ == NULL) {
        perror("mmap");
        exit(-1);
    }
}

int /*? me.from_interface.name ?*/__run(void) {
    /* Nothing required. */
    return 0;
}

/*# Clagged from the seL4 templates with minor changes: #*/
int /*? me.from_interface.name ?*/_wrap_ptr(dataport_ptr_t *p, void *ptr) {
    /*- set offset = c_symbol('offset') -*/
    off_t /*? offset ?*/ = (off_t)((uintptr_t)ptr -
        (uintptr_t)/*? me.from_interface.name ?*/);
    if (/*? offset ?*/ < sizeof(/*? show(me.from_interface.type) ?*/)) {
        p->id = /*? id ?*/;
        p->offset = /*? offset ?*/;
        return 0;
    } else {
        return -1;
    }
}

void * /*? me.from_interface.name ?*/_unwrap_ptr(dataport_ptr_t *p) {
    if (p->id == /*? id ?*/) {
        return (void*)((uintptr_t)/*? me.from_interface.name ?*/ +
            (uintptr_t)p->offset);
    } else {
        return NULL;
    }
}
