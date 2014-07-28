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
#include <camkes/channel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

/* Match the seL4 IPC buffer size for ease of use. */
#define CHANNEL_SIZE 512

struct channel {
    char *key_file;
    int queue_id;
};

typedef struct {
    long type;
    char data[CHANNEL_SIZE];
} msg_t;

channel_t *channel_init(const char *namespace, const char *name) {
    channel_t *c = (channel_t*)malloc(sizeof(channel_t));
    if (c == NULL) {
        goto fail;
    }

    if (namespace == NULL) {
        c->key_file = strdup(name);
        if (c->key_file == NULL) {
            goto fail;
        }
    } else {
        c->key_file = (char*)malloc(strlen(namespace) + 1 + strlen(name) + 1);
        if (c->key_file == NULL) {
            goto fail;
        }
        sprintf(c->key_file, "%s/%s", namespace, name);
    }

    FILE *f = fopen(c->key_file, "w");
    if (f != NULL) {
        fclose(f);
    }
    /* Ignore failure because it just means our partner has already created the
     * file.
     */

    key_t key = ftok(c->key_file, 0x42);
    if (key == -1) {
        goto fail;
    }

    c->queue_id = msgget(key, IPC_CREAT|0666);
    if (c->queue_id == -1) {
        goto fail;
    }

    return c;

fail:
    if (c != NULL) {
        if (c->key_file != NULL) {
            free(c->key_file);
        }
        free(c);
    }
    return NULL;
}

int channel_send(channel_t *ch, void *data, size_t size) {
    assert(ch != NULL);
    assert(data != NULL || size == 0);

    if (size > CHANNEL_SIZE) {
        return -1;
    }

    msg_t m = {
        .type = 1 /* ignored */,
    };
    memcpy(m.data, data, size);

    return msgsnd(ch->queue_id, &m, size, 0 /* blocking send */);
}

int channel_recv(channel_t *ch, void *data, size_t size) {
    assert(ch != NULL);
    assert(data != NULL || size == 0);

    if (size > CHANNEL_SIZE) {
        return -1;
    }

    msg_t m;
    ssize_t res = msgrcv(ch->queue_id, (void*)&m, size,
        0 /* receive any message */, 0 /* blocking receive */);
    if (res == -1) {
        return -1;
    }
    memcpy(data, m.data, res);
    return 0;
}

void channel_destroy(channel_t *ch) {
    assert(ch != NULL);
    unlink(ch->key_file);
    free(ch->key_file);
    free(ch);
}
