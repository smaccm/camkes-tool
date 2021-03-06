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
An object representation of an integer. This is frustratingly necessary for
expressing things like a counter in a Jinja template that can be modified
within a loop.
'''

class Counter(object):
    def __init__(self):
        self.value = 0

    def set(self, value):
        self.value = value

    def __repr__(self):
        return str(self.value)

    def increment(self, offset=1):
        self.value += offset

    def decrement(self, offset=1):
        self.value -= offset
