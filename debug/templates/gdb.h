/*#
 *# Copyright 2014, NICTA
 *#
 *# This software may be distributed and modified according to the terms of
 *# the BSD 2-Clause license. Note that NO WARRANTY is provided.
 *# See "LICENSE_BSD2.txt" for details.
 *#
 *# @TAG(NICTA_BSD)
 #*/

#define NO_BREAKPOINT -1
#define USER_BREAKPOINT 0
#define BREAKPOINT_INSTRUCTION 0xCC
#define DEBUG_PRINT true
#define MAX_ARGS 20   
#define COMMAND_START                   1
#define HEX_STRING						16
#define DEC_STRING						10
#define CHAR_HEX_SIZE					2
// Colour coding for response packets from GDB stub
#define GDB_RESPONSE_START      "\x1b[31m"
#define GDB_RESPONSE_END        "\x1b[0m"

// Ok packet for GDB
#define GDB_ACK                 "+"
#define x86_VALID_REGISTERS     10
#define x86_GDB_REGISTERS       13
#define x86_MAX_REGISTERS       16
#define x86_INVALID_REGISTER    10
#define x86_NUM_HW_BRK          4
#define x86_SW_BREAK            0xCC

#define HARDWARE_BREAKPOINT      0x1
#define GENERAL_PROTECTION_FAULT 0xD

typedef enum {
    stop_none,
    stop_sw_break,
    stop_hw_break,
    stop_step,
    stop_watch
} stop_reason_t;

typedef enum {
    gdb_SoftwareBreakpoint,
    gdb_HardwareBreakpoint,
    gdb_WriteWatchpoint,
    gdb_ReadWatchpoint,
    gdb_AccessWatchpoint
} gdb_BreakpointType;

// The ordering of registers as GDB expects them
typedef enum {
    GDBRegister_eax =    0,
    GDBRegister_ecx =    1,
    GDBRegister_edx =    2,
    GDBRegister_ebx =    3,
    GDBRegister_esp =    4,
    GDBRegister_ebp =    5,
    GDBRegister_esi =    6,
    GDBRegister_edi =    7,
    GDBRegister_eip =    8,
    GDBRegister_eflags = 9,
    GDBRegister_cs =     10,
    GDBRegister_ss =     11,
    GDBRegister_ds =     12
} x86_gdb_registers;

// The ordering for registers in seL4
// Only required since GDB refers to registers by intel ordering
#define SEL4_EFLAGS             2

#define IA32_CR4_DE             3


typedef struct gdb_buffer {
    uint32_t length;
    uint32_t checksum_count;
    uint32_t checksum_index;
    char data[GETCHAR_BUFSIZ];
} gdb_buffer_t;

gdb_buffer_t buf;


// Map registers from what GDB expects to seL4 order
static unsigned char x86_GDB_Register_Map[x86_GDB_REGISTERS] = {
    // eax
    3,
    // ecx
    5,
    // edx
    6,
    // ebx
    4,
    // esp
    1,
    // ebp
    9,
    // esi
    7,
    // edi
    8,
    // eip
    0,
    // eflags
    2,
    // cs - invalid
    x86_INVALID_REGISTER,
    // ss - invalid
    x86_INVALID_REGISTER,
    // ds - invalid
    x86_INVALID_REGISTER
};

static int handle_gdb(void);

static unsigned char compute_checksum(char *data, int length);
static int get_breakpoint_format(gdb_BreakpointType type,
                               seL4_Word *break_type,
                               seL4_Word *rw);
static void send_message(char *message, int len);
static void string_to_word_data(char *string, seL4_Word *dest);
static void GDB_read_memory(char *command);
static void GDB_write_memory(char *command);
static void GDB_write_memory_binary(char *command);
static void GDB_query(char *command);
static void GDB_set_thread(char *command);
static void GDB_stop_reason(char *command);
static void GDB_read_general_registers(char *command);
static void GDB_read_register(char *command);
static void GDB_vcont(char *command);
static void GDB_continue(char *command);
static void GDB_step(char *command);
static void GDB_breakpoint(char *command, bool insert);


extern int /*? me.from_instance.name ?*/_write_memory(seL4_Word addr, seL4_Word length, 
															unsigned char *data);
extern int /*? me.from_instance.name ?*/_read_memory(seL4_Word addr, seL4_Word length, 
																unsigned char *data);
extern void /*? me.from_instance.name ?*/_read_registers(seL4_Word tcb_cap, seL4_Word registers[]);
extern void /*? me.from_instance.name ?*/_read_register(seL4_Word tcb_cap, seL4_Word *reg, seL4_Word reg_num);
extern void /*? me.from_instance.name ?*/_write_registers(seL4_Word tcb_cap, seL4_Word registers[], int len);
extern void /*? me.from_instance.name ?*/_write_register(seL4_Word tcb_cap, seL4_Word data, seL4_Word reg_num);
extern int /*? me.from_instance.name ?*/_insert_break(seL4_Word tcb_cap, seL4_Word type, seL4_Word addr, seL4_Word size, seL4_Word rw);
extern int /*? me.from_instance.name ?*/_remove_break(seL4_Word tcb_cap, seL4_Word type, seL4_Word addr, seL4_Word size, seL4_Word rw);
extern int /*? me.from_instance.name ?*/_resume(seL4_Word tcb_cap);
extern int /*? me.from_instance.name ?*/_step(seL4_Word tcb_cap);
