/*#
 *# Copyright 2014, NICTA
 *#
 *# This software may be distributed and modified according to the terms of
 *# the BSD 2-Clause license. Note that NO WARRANTY is provided.
 *# See "LICENSE_BSD2.txt" for details.
 *#
 *# @TAG(NICTA_BSD)
 #*/

#include <endian.h>

static int handle_gdb(void) {
    // Acknowledge packet
    gdb_printf(GDB_RESPONSE_START GDB_ACK GDB_RESPONSE_END "\n");
    // Get command and checksum
    int command_length = buf.checksum_index-1;
    char *command_ptr = &buf.data[COMMAND_START];
    char command[GETCHAR_BUFSIZ + 1] = {0};
    strncpy(command, command_ptr, command_length);
    char *checksum = &buf.data[buf.checksum_index + 1];
    // Calculate checksum of data
    debug_printf("command: %s\n", command);
    unsigned char computed_checksum = compute_checksum(command, 
                                                       command_length);
    unsigned char received_checksum = (unsigned char) strtol(checksum, 
                                                             NULL, 
                                                             HEX_STRING);
    if (computed_checksum != received_checksum) {
        debug_printf("Checksum error, computed %x,"
                     "received %x received_checksum\n",
                     computed_checksum, received_checksum);
    }
    // Parse the command
    handle_command(command);
    return 0;
}

// Compute a checksum for the GDB remote protocol
static unsigned char compute_checksum(char *data, int length) {
    unsigned char checksum = 0;
    for (int i = 0; i < length; i++) {
        checksum += (unsigned char) data[i];
    }
    return checksum;
}

// Send a message with the GDB remote protocol
static void send_message(char *message, int len) {
    if (len == 0) {
        len = strlen(message);
    }
    unsigned char checksum = compute_checksum(message, len);
    gdb_printf(GDB_RESPONSE_START "$%s#%02X\n", message, checksum);
    gdb_printf(GDB_RESPONSE_END);
}

static void string_to_word_data(char *string, seL4_Word *dest) {
    char buf[sizeof(int) * 2] = {0};
    strncpy(buf, string, sizeof(int) * 2);
    *dest = (seL4_Word) strtoul((char *) buf, NULL, HEX_STRING);
}

// GDB read memory command format:
// m[addr],[length]
static void GDB_read_memory(char *command) {
    int err;
    char *token_ptr;
    // Get args from command
    char *addr_string = strtok_r(command, "m,", &token_ptr);
    char *length_string = strtok_r(NULL, ",", &token_ptr);
    // Convert strings to values
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, HEX_STRING);
    seL4_Word length = (seL4_Word) strtol(length_string, NULL, 
                                          DEC_STRING);
    // Buffer for raw data
    unsigned char data[length];
    // Buffer for data formatted as hex string
    size_t buf_len = CHAR_HEX_SIZE * length + 1;
    char data_string[buf_len];
    debug_printf("length: %d\n", length);
    // Do a read call to the GDB delegate who will read from memory
    // on our behalf
    err = /*? me.from_instance.name ?*/_read_memory(addr, length, data);
    if (err) {
        send_message("E01", 0);
    } else {
        // Format the data
        for (int i = 0; i < length; i++) {
          sprintf(&data_string[CHAR_HEX_SIZE * i], "%02x", data[i]);
        }
        send_message(data_string, buf_len);
    }
}

// GDB write memory command format:
// M[addr],[length]:[data]
static void GDB_write_memory(char *command) {
    char *token_ptr;
    int err;
    // Get args from command
    char *addr_string = strtok_r(command, "M,", &token_ptr);
    char *length_string = strtok_r(NULL, ",:", &token_ptr);
    char *data_string = strtok_r(NULL, ":", &token_ptr);
     // Convert strings to values
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, HEX_STRING);
    seL4_Word length = (seL4_Word) strtol(length_string, NULL, 
                                          DEC_STRING);
    // Buffer for data to be written
    unsigned char data[length];
    memset(data, 0, length);
    // Parse data to be written as raw hex
    for (int i = 0; i < length; i++) {
        sscanf(data_string, "%2hhx", &data[i]);
        data_string += CHAR_HEX_SIZE;
    }
    // Do a write call to the GDB delegate who will write to memory 
    // on our behalf
    err = /*? me.from_instance.name ?*/_write_memory(addr, length, 
                                                     data);
    if (err) {
        send_message("E01", 0);
    } else {
        send_message("OK", 0);
    }
}

// GDB write binary memory command format:
// X[addr],[length]:[data]
static void GDB_write_memory_binary(char *command) {
    char *token_ptr;
    // Get args from command
    char *addr_string = strtok_r(command, "X,", &token_ptr);
    char *length_string = strtok_r(NULL, ",:", &token_ptr);
    // Convert strings to values
    seL4_Word addr = strtol(addr_string, NULL, HEX_STRING);
    seL4_Word length = strtol(length_string, NULL, DEC_STRING);
    unsigned char *data = NULL;
    if (length != 0) {
        data = (unsigned char *) strtok_r(NULL, ":", &token_ptr);
        // Copy the raw data to the expected location
    }
    // Do a write call to the GDB delegate who will write to memory 
    // on our behalf
    int err = /*? me.from_instance.name ?*/_write_memory(addr, 
                                                         length,
                                                         data);
    if (err) {
        send_message("E01", 0);
    } else {
        send_message("OK", 0);
    }
}

// GDB query command format:
// q[query]...
static void GDB_query(char *command) {
    char *token_ptr;
    char *query_type = strtok_r(command, "q:", &token_ptr);
    if (strcmp("Supported", query_type) == 0) {// Setup argument storage
        send_message("PacketSize=100", 0);
    // Most of these query messages can be ignored for basic functionality
    } else if (!strcmp("TStatus", query_type)) {
        send_message("", 0);
    } else if (!strcmp("TfV", query_type)) {
        send_message("", 0);
    } else if (!strcmp("C", query_type)) {
        send_message("QC1", 0);
    } else if (!strcmp("Attached", query_type)) {
        send_message("", 0);
    } else if (!strcmp("fThreadInfo", query_type)) {
        send_message("m01", 0);
    } else if (!strcmp("sThreadInfo", query_type)) {
        send_message("l", 0);
    } else if (!strcmp("Symbol", query_type)) {
        send_message("", 0);
    } else if (!strcmp("Offsets", query_type)) {
        send_message("", 0);
    } else {
        debug_printf("Unrecognised query command\n");
        send_message("E01", 0);
    }
}

// Currently ignored
static void GDB_set_thread(char *command) {
    send_message("OK", 0);
}   

// Respond with the reason the thread being debuged stopped
static void GDB_stop_reason(char *command) {
    switch(stop_reason) {
        case stop_hw_break:
            send_message("T05thread:01;hwbreak:;", 0);
            break;
        case stop_sw_break:
            send_message("T05thread:01;swbreak:;", 0);
            break;
        default:
            send_message("T05thread:01;", 0);
    }
}

static void GDB_read_general_registers(char* command) {
    seL4_Word registers[x86_MAX_REGISTERS] = {0};
    /*? me.from_instance.name ?*/_read_registers(tcb_num, registers);
    int buf_len = x86_MAX_REGISTERS * sizeof(int) * CHAR_HEX_SIZE + 1;
    char data[buf_len];
    memset(data, 0, buf_len);
    // Read the register data from the buffer and marshall into a string
    // to send back to GDB, making sure the byte order is correct
    for (int i = 0; i < x86_MAX_REGISTERS; i++) {
        sprintf(data + sizeof(seL4_Word) * CHAR_HEX_SIZE * i, 
                "%08x", __builtin_bswap32(registers[i]));
    }
    send_message(data, buf_len);
}

// GDB read register command format:
// p[reg_num]
static void GDB_read_register(char* command) {
    seL4_Word reg;
    char *token_ptr;
    // Get which register we want to read
    char *reg_string = strtok_r(&command[1], "", &token_ptr);
    if (reg_string == NULL) {
        send_message("E00", 0);
        return;
    }
    seL4_Word reg_num = strtol(reg_string, NULL, HEX_STRING);
    if (reg_num >= x86_INVALID_REGISTER) {
        send_message("E00", 0);
        return;
    }
    // Convert to the register order we have
    seL4_Word seL4_reg_num = x86_GDB_Register_Map[reg_num];
    /*? me.from_instance.name ?*/_read_register(tcb_num, &reg, 
                                                seL4_reg_num);
    int buf_len = sizeof(seL4_Word) * CHAR_HEX_SIZE + 1;
    char data[buf_len];
    // Send the register contents as a string, making sure
    // the byte order is correct
    sprintf(data, "%02x", __builtin_bswap32(reg));
    send_message(data, buf_len);
}

static void GDB_write_general_registers(char *command) {
    char *token_ptr;
    // Get args from command
    char *data_string = strtok_r(&command[1], "", &token_ptr);
    // Truncate data to register length
    int num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    int num_regs_data = (strlen(data_string)) / (sizeof(int) * 2);
    if (num_regs_data > num_regs) {
        num_regs_data = num_regs;
    }
    // Marshall data
    seL4_Word data[num_regs_data];
    for (int i = 0; i < num_regs_data; i++) {
        string_to_word_data(&data_string[2*i*sizeof(int)], &data[i]);
        data[i] = __builtin_bswap32(data[i]);
    }
    /*? me.from_instance.name ?*/_write_registers(tcb_num, data, 
                                                  num_regs_data);
    reg_pc = data[GDBRegister_eip];
    send_message("OK", 0);
}

// GDB write register command format:
// P[reg_num]=[data]
static void GDB_write_register(char *command) {
    char *token_ptr;
    // Parse arguments
    char *reg_string = strtok_r(&command[1], "=", &token_ptr);
    char *data_string = strtok_r(NULL, "", &token_ptr);
    // If valid register, do something, otherwise reply OK
    seL4_Word reg_num = strtol(reg_string, NULL, HEX_STRING);
    if (reg_num < x86_GDB_REGISTERS) {
        // Convert arguments
        seL4_Word data;
        string_to_word_data(data_string, &data);
        data = __builtin_bswap32(data);
        // Convert to register order we have
        seL4_Word seL4_reg_num = x86_GDB_Register_Map[reg_num];
        /*? me.from_instance.name ?*/_write_register(tcb_num, data, 
                                                     seL4_reg_num);
        if (reg_num == GDBRegister_eip) {
            reg_pc = data;
        }
    }   
    send_message("OK", 0);
}

static void GDB_vcont(char *command) {
    if (!strncmp(&command[7],"c", 1)) {
        GDB_continue(command);
    } else if (!strncmp(&command[7],"s", 1)) {
        GDB_step(command);
    } else {
        send_message("", 0);
    }
}

static void GDB_continue(char *command) {
    int err = 0;
    if (step_mode) {
        err = /*? me.from_instance.name ?*/_resume(tcb_num);
        step_mode = false;
    }
    stream_read = false;
    seL4_MessageInfo_t info;
    if (err) {
        send_message("E01", 0);
    } else {
        // Reply to the fault ep to restart the thread
        if (stop_reason >= stop_hw_break) {
            // If this was a Debug Exception, then we respond with
            // a bp_num and the number of instruction to step
            // Since we're going to continue, we set MR1 to 0
            info = seL4_MessageInfo_new(0, 0, 0, 2);
            seL4_SetMR(0, 0);
            seL4_SetMR(1, 0);
            seL4_Send(/*? reply_cap_slot ?*/, info);
        } else {
            // If this was a fault, set the instruction pointer to
            // what we expect it to be
            info = seL4_MessageInfo_new(0, 0, 0, 1);
            seL4_SetMR(0, reg_pc);
            seL4_Send(/*? reply_cap_slot ?*/, info);
        }
    }
}

static void GDB_step(char *command) {
    int err = 0;
    if (!step_mode) {
        err = /*? me.from_instance.name ?*/_step(tcb_num);
        step_mode = true;
    }
    stream_read = false;
    if (err) {
        send_message("E01", 0);
    } else {
        seL4_MessageInfo_t info;
        if (stop_reason >= stop_hw_break) {
            // If this was a Debug Exception, then we respond with
            // a bp_num and the number of instruction to step
            // Since we're going to step, we set MR1 to 1
            info = seL4_MessageInfo_new(0, 0, 0, 2);
            seL4_SetMR(0, 0);
            seL4_SetMR(1, 1);
            seL4_Send(/*? reply_cap_slot ?*/, info);
        } else {
            // If this was a fault, set the instruction pointer to
            // what we expect it to be
            info = seL4_MessageInfo_new(0, 0, 0, 1);
            seL4_SetMR(0, reg_pc);
            seL4_Send(/*? reply_cap_slot ?*/, info);
        }
    }
}

// GDB insert breakpoint command format:
// Z[type],[addr],[size]
static void GDB_breakpoint(char *command, bool insert) {
    char *token_ptr;
    seL4_Word break_type;
    seL4_Word rw;
    // Parse arguments
    char *type_string = strtok_r(&command[1], ",", &token_ptr);
    char *addr_string = strtok_r(NULL, ",", &token_ptr);
    char *size_string = strtok_r(NULL, ",", &token_ptr);
    // Convert strings to values
    seL4_Word type = (seL4_Word) strtol(type_string, NULL, HEX_STRING);
    seL4_Word addr = (seL4_Word) strtol(addr_string, NULL, HEX_STRING);
    seL4_Word size = (seL4_Word) strtol(size_string, NULL, HEX_STRING);
    // If this is a software breakpoint, then we will ignore
    // By ignoring this command, GDB will just use the read and write
    // memory commands to set a breakpoint itself. This can later be changed
    // if setting software breakpoints becomes supported by the kernel.
    if (type == gdb_SoftwareBreakpoint) {
        send_message("", 0);
    } else {
        int err;
        err = get_breakpoint_format(type, &break_type, &rw);
        if (!err) {
            // Hardware breakpoints can only be size 0
            if (type == gdb_HardwareBreakpoint) {
                size = 0;
            }
            if (insert) {
                err = /*? me.from_instance.name ?*/_insert_break(tcb_num, break_type,
                                                                 addr, size,
                                                                 rw);
            } else {
                err = /*? me.from_instance.name ?*/_remove_break(tcb_num, break_type, 
                                                                 addr, size,
                                                                 rw);
            }
        }
        if (err) {
            send_message("E01", 0);
        } else {
            send_message("OK", 0);
        }
    }
}

static int get_breakpoint_format(gdb_BreakpointType type, 
                               seL4_Word *break_type, seL4_Word *rw) {
    int err = 0;
    debug_printf("Breakpoint type %d\n", type);
    switch (type) {
        case gdb_HardwareBreakpoint:
            *break_type = seL4_InstructionBreakpoint;
            *rw = seL4_BreakOnRead;
            err = 0;
            break;
        case gdb_WriteWatchpoint:
            *break_type = seL4_DataBreakpoint;
            *rw = seL4_BreakOnWrite;
            err = 0;
            break;
        case gdb_ReadWatchpoint:
            *break_type = seL4_DataBreakpoint;
            *rw = seL4_BreakOnRead;
            err = 0;
            break;
        case gdb_AccessWatchpoint:
            *break_type = seL4_DataBreakpoint;
            *rw = seL4_BreakOnReadWrite;
            err = 0;
            break;
        default:
            // Unknown type
            err = 1;
    }
    return err;
}