/*#
 *# Copyright 2014, NICTA
 *#
 *# This software may be distributed and modified according to the terms of
 *# the BSD 2-Clause license. Note that NO WARRANTY is provided.
 *# See "LICENSE_BSD2.txt" for details.
 *#
 *# @TAG(NICTA_BSD)
 #*/

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <utils/util.h>
#include <camkes.h>

#ifndef CONFIG_HARDWARE_DEBUG_API
    #error
#endif
/*? macros.show_includes(me.to_instance.type.includes) ?*/
/*? macros.show_includes(me.to_interface.type.includes, '../static/components/' + me.to_instance.type.name + '/') ?*/

/*- set methods_len = len(me.to_interface.type.methods) -*/
/*- set instance = me.to_instance.name -*/
/*- set interface = me.to_interface.name -*/
/*- set size = 'seL4_MsgMaxLength * sizeof(seL4_Word)' -*/
/*- set allow_trailing_data = False -*/
/*- set ep = alloc(me.to_instance.name + "_ep_GDB_delegate", seL4_EndpointObject, read=True, write=True) -*/
/*- set cnode = alloc_cap('cnode', my_cnode, write=True) -*/
/*- set info = c_symbol('info') -*/
 /*- set reply_cap_slot = alloc_cap('reply_cap_slot', None) -*/
/*- include 'GDB_delegate.h' -*/

static void read_memory(void);
static void write_memory(void);
static void read_register(void);
static void read_registers(void);
static void write_registers(void);
static void write_register(void);
static void insert_break(void);
static void remove_break(void);
static int check_read_memory(seL4_Word addr);
static int check_write_memory(seL4_Word addr);
static void resume(void);
static void step(void);

static void breakpoint_init(void);
static int find_free_breakpoint(void);
static int get_breakpoint_num(seL4_Word vaddr, seL4_Word type, seL4_Word size, seL4_Word rw);
static void set_breakpoint_state(seL4_Word vaddr, seL4_Word type, seL4_Word size, seL4_Word rw, int bp);
static void clear_breakpoint_state(int bp);

#ifdef CONFIG_HARDWARE_DEBUG_API
breakpoint_state_t breakpoints[seL4_NumDualFunctionMonitors];
#endif

int /*? me.to_interface.name ?*/__run(void) {
    breakpoint_init();
    while (1) { 
        // Receive RPC and reply
        seL4_MessageInfo_t info = seL4_Recv(/*? ep ?*/, NULL);
        seL4_Word instruction = seL4_MessageInfo_get_label(info);
        switch (instruction) {
            case GDB_READ_MEM:
                read_memory();
                break;
            case GDB_WRITE_MEM:
                write_memory();
                break;
            case GDB_READ_REGS:
                read_registers();
                break;
            case GDB_WRITE_REGS:
                write_registers();
                break;
            case GDB_READ_REG:
                read_register();
                break;
            case GDB_WRITE_REG:
                write_register();
                break;
            case GDB_INSERT_BREAK:
                insert_break();
                break;
            case GDB_REMOVE_BREAK:
                remove_break();
                break;
            case GDB_RESUME:
                resume();
                break;
            case GDB_STEP:
                step();
                break;
            default:
                info = seL4_MessageInfo_new(0, 0, 0, 1);
                seL4_Send(/*? ep ?*/, info);
                continue;
        }
    }
    UNREACHABLE();
}

static void read_memory(void) {
    seL4_MessageInfo_t info;
    seL4_Word addr = seL4_GetMR(DELEGATE_ARG(0));
    seL4_Word length = seL4_GetMR(DELEGATE_ARG(1));
    // If invalid memory, return with error
    if (check_read_memory(addr)) {
        info = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, 1);
    } else if (length != 0) {
        seL4_Word message = 0;
        unsigned char byte = 0;
        // Pack data for messaging
        info = seL4_MessageInfo_new(0, 0, 0, CEIL_MR(length) + 1);
        seL4_SetMR(0, 0);
        for (int i = 0; i < length; i++) {
            byte = *((unsigned char*)(addr + i));
            message |= ((seL4_Word) byte) << ((FIRST_BYTE_BITSHIFT - (i % BYTES_IN_WORD) * BYTE_SHIFT));
            if ((i+1) % BYTES_IN_WORD == 0 || i == length-1) {
                seL4_SetMR(1 + (i / BYTES_IN_WORD), message);
                message = 0;
            }
        }
    }
    seL4_Send(/*? ep ?*/, info);
}

static void write_memory(void) {
    seL4_MessageInfo_t info;
    char *addr = (char *) seL4_GetMR(DELEGATE_ARG(0));
    seL4_Word length = seL4_GetMR(DELEGATE_ARG(1));
    seL4_Word message = 0;
    unsigned char byte = 0;
    char data[length];
    // If invalid memory, return with error
    if (check_write_memory((seL4_Word) addr)) {
        info = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, 1);
    } else if (length != 0) {
        // Unpack data from message
        for (int i = 0; i < length; i++) {
            if (i % BYTES_IN_WORD == 0) {
              message = seL4_GetMR(DELEGATE_WRITE_NUM_ARGS + (i / BYTES_IN_WORD));
            }
            byte = message & LAST_BYTE_MASK;
            message >>= BYTE_SHIFT;   
            data[i] = byte; 
        }
        memcpy(addr, data, length);
        info = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, 0);  
    }
    seL4_Send(/*? ep ?*/, info);
}

static void read_registers(void) {
    seL4_UserContext regs = {0};
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 10);
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    seL4_TCB_ReadRegisters(tcb_cap, false, 0, num_regs, &regs);
    // Send registers in the order GDB expects them
    seL4_SetMR(0, regs.eax);
    seL4_SetMR(1, regs.ecx);
    seL4_SetMR(2, regs.edx);
    seL4_SetMR(3, regs.ebx);
    seL4_SetMR(4, regs.esp);
    seL4_SetMR(5, regs.ebp);
    seL4_SetMR(6, regs.esi);
    seL4_SetMR(7, regs.edi);
    seL4_SetMR(8, regs.eip);
    seL4_SetMR(9, regs.eflags);
    seL4_Send(/*? ep ?*/, info);
}

static void read_register(void) {
    seL4_UserContext regs = {0};
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    seL4_Word reg_num = seL4_GetMR(DELEGATE_ARG(1));
    seL4_TCB_ReadRegisters(tcb_cap, false, 0, num_regs, &regs);
    seL4_Word *reg_word = (seL4_Word *) (& regs);
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, reg_word[reg_num]);
    seL4_Send(/*? ep ?*/, info);
}

static void write_registers(void) {
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    // Get register values from IPC
    seL4_UserContext regs;
    seL4_Word *reg_word = (seL4_Word *) (&regs);
    int err;
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    for (int i = 0; i < num_regs; i++) {
        reg_word[i] = seL4_GetMR(DELEGATE_ARG(i+1));
    }
    // Write registers
    err = seL4_TCB_WriteRegisters(tcb_cap, false, 0, num_regs, &regs);
    // Send IPC
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    if (err) {
        seL4_SetMR(0, 1);
    } else {
        seL4_SetMR(0, 0);
    }
    seL4_Send(/*? ep ?*/, info);
}

static void write_register(void) {
    int err;
    // Get arguments
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    seL4_Word data = seL4_GetMR(DELEGATE_ARG(1));
    seL4_Word reg_num = seL4_GetMR(DELEGATE_ARG(2));
    // Get registers
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    seL4_UserContext regs = {0};
    seL4_Word *reg_word = (seL4_Word *) (&regs);
    seL4_TCB_ReadRegisters(tcb_cap, false, 0, num_regs, &regs);
    // Change relevant register
    reg_word[reg_num] = data;
    err = seL4_TCB_WriteRegisters(tcb_cap, false, 0, num_regs, &regs);
    // Send IPC
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    if (err) {
        seL4_SetMR(0, 1);
    } else {
        seL4_SetMR(0, 0);
    }
    seL4_SetMR(0, 0);
    seL4_Send(/*? ep ?*/, info);
}

static void insert_break(void) {
    int err = 0;
#ifdef CONFIG_HARDWARE_DEBUG_API
    // Get arguments
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    seL4_Word type = seL4_GetMR(DELEGATE_ARG(1));
    seL4_Word addr = seL4_GetMR(DELEGATE_ARG(2));
    seL4_Word size = seL4_GetMR(DELEGATE_ARG(3));
    seL4_Word rw = seL4_GetMR(DELEGATE_ARG(4));
    // Insert the breakpoint
    int bp_num = find_free_breakpoint();
    if (bp_num == -1) {
        err = 1;
    } else {
        // Set the breakpoint
        err = seL4_TCB_SetBreakpoint(tcb_cap, (seL4_Uint16) bp_num, addr,
                                     type, size, rw);
        if (!err) {
            set_breakpoint_state(addr, type, size, rw, bp_num);
        }
        seL4_TCB_GetBreakpoint_t bp = seL4_TCB_GetBreakpoint(tcb_cap, bp_num);
    }
#else
    err = -1;
#endif
    // Send a reply
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    if (err) {
        seL4_SetMR(0, 1);
    } else {
        seL4_SetMR(0, 0);
    }
    seL4_Send(/*? ep ?*/, info);
}

static void remove_break(void) {
    int err = 0;
#ifdef CONFIG_HARDWARE_DEBUG_API
    // Get arguments
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    seL4_Word type = seL4_GetMR(DELEGATE_ARG(1));
    seL4_Word addr = seL4_GetMR(DELEGATE_ARG(2));
    seL4_Word size = seL4_GetMR(DELEGATE_ARG(3));
    seL4_Word rw = seL4_GetMR(DELEGATE_ARG(4));
    // Find the breakpoint
    int bp_num = get_breakpoint_num(addr, type, size, rw);
    if (bp_num == -1) {
        err = 1;
    }
    // Unset the breakpoint
    err = seL4_TCB_UnsetBreakpoint(tcb_cap, (seL4_Uint16) bp_num);
    if (!err) {
        clear_breakpoint_state(bp_num);
    }
#else
    err = -1;
#endif
    // Send a reply
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    if (err) {
        seL4_SetMR(0, 1);
    } else {
        seL4_SetMR(0, 0);
    }
    seL4_SetMR(0, 0);
    seL4_Send(/*? ep ?*/, info);
}

static int check_read_memory(seL4_Word addr) {
    uint32_t result = 0;
    asm volatile (
        "movl $0, %%eax;"
        "movl %[addr], %%ebx;"
        "movl (%%ebx), %%ecx;"
        "movl %%eax, %0;"
        : "=r" (result)
        : [addr] "r" (addr)
        : "eax", "ebx", "ecx"
    );
    return result;
}

static int check_write_memory(seL4_Word addr) {
    uint32_t result = 0;
    asm volatile (
        "movl $0, %%eax;"
        "movl %[addr], %%ebx;"
        "movl (%%ebx), %%ecx;"
        "movl %%ecx, (%%ebx);"
        "movl %%eax, %0;"
        : "=r" (result)
        : [addr] "r" (addr)
        : "eax", "ebx", "ecx"
    );
    return result;
}

static void resume(void) {
#ifdef CONFIG_HARDWARE_DEBUG_API
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    seL4_TCB_ConfigureSingleStepping_t result = 
        seL4_TCB_ConfigureSingleStepping(tcb_cap, 0, 0);
    // Send a reply
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    if (result.error) {
        seL4_SetMR(0, 1);
    } else {
        seL4_SetMR(0, 0);
    }
#else
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, 0);
#endif
    seL4_Send(/*? ep ?*/, info);
}

static void step(void) {
#ifdef CONFIG_HARDWARE_DEBUG_API
    seL4_Word tcb_cap = seL4_GetMR(DELEGATE_ARG(0));
    seL4_TCB_ConfigureSingleStepping_t result = 
        seL4_TCB_ConfigureSingleStepping(tcb_cap, 0, 1);
    // Send a reply
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    if (result.error) {
        seL4_SetMR(0, 1);
    } else {
        seL4_SetMR(0, 0);
    }
#else
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, 1);
#endif
    seL4_Send(/*? ep ?*/, info);
}

static void breakpoint_init(void) {
#ifdef CONFIG_HARDWARE_DEBUG_API
    for (int i = 0; i < seL4_NumDualFunctionMonitors; i++) {
        breakpoints[i].vaddr = 0;
        breakpoints[i].type = 0;
        breakpoints[i].size = 0;
        breakpoints[i].rw = 0;
        breakpoints[i].is_enabled = false;
    }
#endif
}

static int find_free_breakpoint(void) {
#ifdef CONFIG_HARDWARE_DEBUG_API
    int bp = -1;
    for (int i = 0; i < seL4_NumDualFunctionMonitors; i++) {
        if (!breakpoints[i].is_enabled) {
            bp = i;
            break;
        }
    }
    return bp;
#else
    return -1;
#endif
}

static int get_breakpoint_num(seL4_Word vaddr, seL4_Word type,
                              seL4_Word size, seL4_Word rw) {
#ifdef CONFIG_HARDWARE_DEBUG_API
    int bp = -1;
    for (int i = 0; i < seL4_NumDualFunctionMonitors; i++) {
        if (breakpoints[i].is_enabled && breakpoints[i].vaddr == vaddr &&
            breakpoints[i].type == type && breakpoints[i].size == size &&
            breakpoints[i].rw == rw) {
           bp = i;
           break;
        }
    }
    return bp;
#else
    (void) vaddr;
    (void) type;
    (void) size;
    (void) rw;
    return -1;
#endif
}

static void set_breakpoint_state(seL4_Word vaddr, seL4_Word type,
                                 seL4_Word size, seL4_Word rw, int bp) {
#ifdef CONFIG_HARDWARE_DEBUG_API
    breakpoints[bp].vaddr = vaddr;
    breakpoints[bp].type = type;
    breakpoints[bp].size = size;
    breakpoints[bp].rw = rw;
    breakpoints[bp].is_enabled = true;
#else
    (void) vaddr;
    (void) type;
    (void) size;
    (void) rw;
    (void) bp;
#endif
}

static void clear_breakpoint_state(int bp) {
#ifdef CONFIG_HARDWARE_DEBUG_API
    breakpoints[bp].vaddr = 0;
    breakpoints[bp].type = 0;
    breakpoints[bp].size = 0;
    breakpoints[bp].rw = 0;
    breakpoints[bp].is_enabled = false;
#else
    (void) bp;
#endif
}