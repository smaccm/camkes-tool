#!/bin/bash
#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

# If we were passed --debug, echo commands when running.
if [[ "$1" == "--debug" || "$1" == "-d" ]]; then
    DEBUG=1
    set -x
else
    DEBUG=0
fi

# Create a temporary working directory for the application.
TMP_DIR=$(mktemp -d)

/*- for i in me.composition.instances -*/
    .//*? i.name ?*/ --working "${TMP_DIR}" "$@"
    /*? i.name ?*/_PID=$!
/*- endfor -*/

# Wait for everyone to finish.
/*- for i in me.composition.instances -*/
    wait ${/*? i.name ?*/_PID}
/*- endfor -*/

# If we're not debugging, clean up.
if [ ${DEBUG} -eq 0 ]; then
    rm -rf "${TMP_DIR}"
fi
