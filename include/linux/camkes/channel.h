/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* This API abstracts communication of chunks of data via a channel mechanism.
 *
 * Note, this functionality is not meant for high performance applications. It
 * does resource allocation on critical paths and various other simplifications
 * that significantly degrade throughput.
 */
#ifndef _CAMKES_CHANNEL_H_
#define _CAMKES_CHANNEL_H_

#include <stddef.h>

/* Opaque type for the user. */
typedef struct channel channel_t;

/* Create a new channel.
 *
 * namespace - A prefix for the channel. This can be NULL if you want no
 *   prefix. The channel is globally accessible as 'namespace/name' if you give
 *   a prefix, or 'name' if you do not. Note that namespace must be an existing
 *   directory in which to create metadata.
 * name - Name of the channel. A file of this name will be constructing on disk
 *   for the purposes of making this channel globally available.
 *
 * Returns an opaque pointer on success, NULL on failure.
 */
channel_t *channel_init(const char *namespace, const char *name);

/* Send a message on a channel.
 *
 * ch - Channel to send on. The result of channel_init.
 * data - Pointer to data to send.
 * size - Size in bytes of the data.
 *
 * Returns 0 on success, -1 on failure.
 */
int channel_send(channel_t *ch, void *data, size_t size);

/* Receive a message on a channel.
 *
 * ch - Channel to receive on.
 * data - Pointer to buffer to receive into.
 * size - Size in bytes of the buffer. If the incoming message is too large to
 *   fit in the buffer, it will be truncated. The implication of this is that
 *   the caller should know the size of the data they are expecting to receive
 *   in advance.
 *
 * Returns 0 on success, -1 on failure.
 */
int channel_recv(channel_t *ch, void *data, size_t size);

/* Close and delete a channel.
 *
 * ch - Channel to destroy.
 */
void channel_destroy(channel_t *ch);

#endif
