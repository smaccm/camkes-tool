/*#
 *# Copyright 2014, NICTA
 *#
 *# This software may be distributed and modified according to the terms of
 *# the BSD 2-Clause license. Note that NO WARRANTY is provided.
 *# See "LICENSE_BSD2.txt" for details.
 *#
 *# @TAG(NICTA_BSD)
 #*/

/*? macros.show_includes(me.to_instance.type.includes) ?*/
/*? macros.show_includes(me.to_interface.type.includes, '../static/components/' + me.to_instance.type.name + '/') ?*/


/*- set mem_ep = alloc(me.from_instance.name + "_mem_fault", seL4_EndpointObject, read=True, write=True, grant=True) -*/

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <utils/util.h>
#include <camkes.h>
#include <stdarg.h>

int /*? me.to_interface.name ?*/__run(void) {
    // Make connection to gdb
 	seL4_Word delegate_tcb;
 	seL4_UserContext regs;
    while (1) {
        seL4_Recv(/*? mem_ep ?*/, &delegate_tcb);
        printf("Mem fault received at PC %08x\n", seL4_GetMR(0));
        printf("Fault on %08x\n", seL4_GetMR(1));
        seL4_TCB_ReadRegisters(delegate_tcb, false, 0, 
        	                   sizeof(seL4_UserContext) / sizeof(seL4_Word),
        	                   &regs);
        // Check eax is 0 so that we know they were checking memory
        // TODO Change this to check they were in the right function
        if (regs.eax == 0) {
        	// Signal to the delegate the memory is invalid
        	regs.eax = 1;
	       	// Increment past the faulting instruction
	        regs.eip += 2;
	        // Write registers back
	        seL4_TCB_WriteRegisters(delegate_tcb, false, 0, 
	        	                    sizeof(seL4_UserContext) / sizeof(seL4_Word), 
	        	                    &regs);
	        // Resume the caller
	        printf("PC: %08x\n", regs.eip);
	        seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    		seL4_SetMR(0, regs.eip);
	        seL4_Reply(info);
        }
    }
}
