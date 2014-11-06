#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

'''
A mapping from abstract CAmkES generated files to implementations for a
specific platform. Callers are intended to invoke `lookup` with a path to the
template they need, using dots as path separators. E.g. call
`lookup('seL4.seL4RPC.from.source')` to get the template for the source file
for the from end of a seL4RPC connector on seL4.
'''

from Template import Templates, PLATFORMS
import macros
