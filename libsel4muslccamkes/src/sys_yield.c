/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sel4/sel4.h>
#include <errno.h>
#include <autoconf.h>

long sys_sched_yield(va_list ap)
{
#ifdef CONFIG_KERNEL_RT
    return -ENOSYS;
#else
    seL4_Yield();
    return 0;
#endif
}
