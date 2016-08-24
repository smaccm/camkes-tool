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
#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <utils/util.h>
#include <camkes.h>
#include <stdarg.h>

/*? macros.show_includes(me.to_instance.type.includes) ?*/
/*? macros.show_includes(me.to_interface.type.includes, '../static/components/' + me.to_instance.type.name + '/') ?*/


/*- set methods_len = len(me.to_interface.type.methods) -*/
/*- set instance = me.to_instance.name -*/
/*- set interface = me.to_interface.name -*/
/*- set size = 'seL4_MsgMaxLength * sizeof(seL4_Word)' -*/
/*- set allow_trailing_data = False -*/
/*- set ep = alloc(me.from_instance.name + "_ep_fault", seL4_EndpointObject, read=True, write=True, grant=True) -*/
/*- set cnode = alloc_cap('cnode', my_cnode, write=True) -*/
/*- set reply_cap_slot = alloc_cap('reply_cap_slot', None) -*/
/*- set info = c_symbol('info') -*/

/*- include 'serial.h' -*/
/*- include 'gdb.h' -*/

static int handle_command(char* command);
static void find_stop_reason(seL4_Word exception_num, seL4_Word fault_type);

static seL4_Word reg_pc;
static seL4_Word tcb_num;

static bool step_mode = false;
static stop_reason_t stop_reason = stop_none;

/*- include 'serial.c' -*/
/*- include 'gdb.c' -*/

void /*? me.to_interface.name ?*/__init(void) {
    debug_serial_init();
}

int /*? me.to_interface.name ?*/__run(void) {
    seL4_Word fault_type;
    seL4_Word fault_addr;
    seL4_Word exception_num;
    seL4_Word length;
    seL4_MessageInfo_t info;
    while (1) {
        info = seL4_Recv(/*? ep ?*/, &tcb_num);
        debug_printf("Received fault for tcb %u\n", tcb_num);
        fault_type = seL4_MessageInfo_get_label(info);
        length = seL4_MessageInfo_get_length(info);
        // Get the PC relevant registers
        reg_pc = seL4_GetMR(0);
        exception_num = seL4_GetMR(3);
        debug_printf("Stopped at %08x\n", reg_pc);
        debug_printf("Length: %lu\n", length);
        // Save the reply cap
    #ifdef CONFIG_KERNEL_RT
        seL4_CNode_SwapCaller(/*? cnode ?*/, /*? reply_cap_slot ?*/, 32);
    #else
        seL4_CNode_SaveCaller(/*? cnode ?*/, /*? reply_cap_slot ?*/, 32);
    #endif
        find_stop_reason(exception_num, fault_type);
        // Start accepting GDB input
        stream_read = true;
        serial_irq_reg_callback(serial_irq_rcv, 0);
    }
    UNREACHABLE();
}

static void find_stop_reason(seL4_Word exception_num, seL4_Word fault_type) {
    if (fault_type == seL4_UserException) {
        // Read from memory to make sure it's a user added software breakpoint
        unsigned char byte_check;
        /*? me.from_instance.name ?*/_read_memory(reg_pc, 1, &byte_check);
        if (byte_check == x86_SW_BREAK) {
            // Increment PC
            reg_pc += 1;
            /*? me.from_instance.name ?*/_write_register(tcb_num, reg_pc, 0);
            send_message("T05thread:01;swbreak:;", 0);
            stop_reason = stop_sw_break;
            debug_printf("Software breakpoint\n");
        } else {
            debug_printf("Got byte %02x\n", byte_check);
            send_message("T05thread:01;", 0);
            stop_reason = stop_none; 
            debug_printf("Unknown stop reason, GP fault\n");
        }
    } else if (fault_type == seL4_DebugException) {
        seL4_Word bp_num = seL4_GetMR(1);
        debug_printf("Breakpoint number %lu\n", bp_num);
        if (step_mode) {
            send_message("T05thread:01;", 0);
            stop_reason = stop_step;
            debug_printf("Did step\n");
        } else {
            send_message("T05thread:01;", 0);
            stop_reason = stop_watch;
            debug_printf("Hit watchpoint / breakpoint\n");
        }
    } else {
        send_message("T05thread:01;\n", 0);
        stop_reason = stop_none;
        debug_printf("Unknown stop reason\n");
    }
    step_mode = false;
}

static int handle_command(char* command) {
    switch (command[0]) {
        case '!':
            // Enable extended mode
            debug_printf("Not implemented: enable extended mode\n");
            break;
        case '?':
            // Halt reason
            GDB_stop_reason(command);
            break;
        case 'A':
            // Argv
            debug_printf("Not implemented: argv\n");
            break;
        case 'b':
            if (command[1] == 'c') {
                // Backward continue
                debug_printf("Not implemented: backward continue\n");
                break;
            } else if (command[1] == 's') {
                // Backward step
                debug_printf("Not implemented: backward step\n");
                break;
            } else {
               // Set baud rate
                debug_printf("Not implemented: Set baud rate\n"); 
                break;
            } 
        case 'c':
            // Continue
            debug_printf("Continuing\n");
            GDB_continue(command);
            break;
        case 'C':
            // Continue with signal
            debug_printf("Not implemented: continue with signal\n");
            break;
        case 'd':
            debug_printf("Not implemented: toggle debug\n");
            break;
        case 'D':
            debug_printf("Not implemented: detach\n");
            break;
        case 'F':
            debug_printf("Not implemented: file IO\n");
            break;
        case 'g':
            debug_printf("Reading general registers\n");
            GDB_read_general_registers(command);
            break;
        case 'G':
            debug_printf("Write general registers\n");
            GDB_write_general_registers(command);
            break;
        case 'H':
            debug_printf("Set thread ignored\n");
            GDB_set_thread(command);
            break;
        case 'i':
            debug_printf("Not implemented: cycle step\n");
            break;
        case 'I':
            debug_printf("Not implemented: signal + cycle step\n");
            break;
        case 'k':
            debug_printf("Not implemented: kill\n");
            break;
        case 'm':
            debug_printf("Reading memory\n");
            GDB_read_memory(command);
            break;
        case 'M':
            debug_printf("Writing memory\n");
            GDB_write_memory(command);
            break;
        case 'p':
            debug_printf("Read register\n");
            GDB_read_register(command);
            break;
        case 'P':
            debug_printf("Write register\n");
            GDB_write_register(command);
            break;
        case 'q':
            debug_printf("Query\n");
            GDB_query(command);
            break;
        case 'Q':
            debug_printf("Not implemented: set\n");
            break;
        case 'r':
            debug_printf("Not implemented: reset\n");
            break;
        case 'R':
            debug_printf("Not implemented: restart\n");
            break;
        case 's':
            debug_printf("Stepping\n");
            GDB_step(command);
            break;
        case 'S':
            debug_printf("Not implemented: step + signal\n");
            break;
        case 't':
            debug_printf("Not implemented: search\n");
            break;
        case 'T':
            debug_printf("Not implemented: check thread\n");
            break;
        case 'v':
            if (!strncmp(&command[1], "Cont?", 5)) {
                send_message("vCont;c;s", 0);
            } else if (!strncmp(&command[1], "Cont", 4)) {
                GDB_vcont(command);
            } else {
                debug_printf("Command not supported\n");
            }
            break;
        case 'X':
            debug_printf("Writing memory, binary\n");
            GDB_write_memory_binary(command);
            break;
        case 'z':
            debug_printf("Removing breakpoint\n");
            GDB_breakpoint(command, false);
            break;
        case 'Z':
            debug_printf("Inserting breakpoint\n");
            GDB_breakpoint(command, true);
            break;
        default:
            debug_printf("Unknown command\n");
    }
    
    return 0;
}

