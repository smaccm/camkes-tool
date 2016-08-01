<style>
div.warn {    
    background-color: #fcf2f2;
    border-color: #dFb5b4;
    border-left: 5px solid #fcf2f2;
    padding: 0.5em;
    }
</style>

<style>
div.attn {    
    background-color: #ffffb3;
    border-color: #dFb5b4;
    border-left: 5px solid #ffffb3;
    padding: 0.5em;
    }
</style>

# CAmkES Debug Manual


<!--
     Copyright 2014, NICTA

     This software may be distributed and modified according to the terms of
     the BSD 2-Clause license. Note that NO WARRANTY is provided.
     See "LICENSE_BSD2.txt" for details.

     @TAG(NICTA_BSD)
  -->

This document describes the structure and use of the CAmkES debug tool, which allows you to debug systems built on the CAmkES platform. The documentation is divided into sections for users and developers. The [Usage](#usage) section is for people wanting to debug a component that they or someone else has built on CAmkES, as well as the current limitations of the tool. The [Developers](#developers) will describe the internal implementation of the tool, for anyone who wishes to modify or extend the functionality of the tool itself.
This document assumes some familiary with [CAmkES](https://github.com/seL4/ camkes-tool/blob/master/docs/index.md) and the [seL4 microkernel](http://sel4.systems/). If you are not familiar with them then you should read their documentation first.

## Table of Contents
1. [Usage](#usage)
2. [Developers](#developers)
3. [TODO](#todo)

## Usage

This debug tool will provide an interface for you to debug components as you wish within CAmkES. Currently, the debug tool is only compatible with x86.

To select which components you wish to debug, specify the debug attribute in the camkes configuration.

### Example CAmkES file
```c
import <std_connector.camkes>;
import "components/Sender1/Sender1.camkes";
import "components/Receiver1/Receiver1.camkes";

assembly {
  composition {
    component Sender1 sender1;
    component Receiver1 receiver1;
    connection seL4RPC conn1(from sender1.out1, to receiver1.in1);
  }
}

configuration {
    sender1.debug = "True"
}
```

### Serial port
You can set the serial port to use for debugging in tools/camkes/debug/debug_config.py

### Running the tool
The debug tool is used by setting the relevant option in the build.
In the build config menu, enable CAmkES options -> Debugging -> Run GDB debug generator.

</br>
### Debugging

<br/>

After you have built the image, you should be able to connect to the serial port via GDB. The GDB connection will only be opened once there is a fault or breakpoint, so if you want to inspect on startup you should set a code breakpoint within the component you are debugging.

The same serial port also provides other information about the state of the debugger, simply connect to the port (eg. via minicom) to view any messages.

The following code will run the image in QEMU, and divert output to port 1234, which can then be connected to by GDB.
``` qemu-system-i386 -nographic -m 512 -kernel images/kernel-ia32-pc99   -initrd images/capdl-loader-experimental-image-ia32-pc99 \
-chardev socket,host=127.0.0.1,port=1234,id=gnc0,server,nowait \
-device isa-serial,chardev=gnc0 ```
<br> </br>

### Using GDB
Current functionality includes reading memory and registers, backtrace, seeing variables, and so on. Software and hardware breakpoints, as well as watchpoints, can be set.

## Developers

This section is targeted at those intending to modify the CAmkES debug tool implementation itself. The information below assumes you are familiar with the functionality of CAmkES.

### Architecture

When a camkes project is built with the debug tool, a GDB server is generated that allows manipulation of the target debug component. This is generated in the top level assembly, and contains the GDB remote code, and two endpoints to each debug component.

The first is a fault_ep that is connected from the debug component to the server. The second is a connection to a helper thread in the debug component, known as a "delegate", used to manipulate data on GDBs behalf (reading/writing to memory, manipulating the TCB etc.)

### Tool

1. Looks for debug components in the camkes project currently being built. These are specified by adding the following setting to the relevant configuration:
```<component instance>.debug = "True"```
2. Generates new definitions for the specified debug components that allow debug access. This consists of a fault_ep connection to the main GDB component, as well as to the delegate thread in the component.
3. Generates the main GDB server, including endpoints and the GDB remote code.
4. Creates a serial connection to talk to GDB with.

### Files

**debug.py** - The main script for the debug tool. Contains the code for parsing CAmkES and CapDL files.

**seL4GDB-to.template.c** - Template code for the GDB server fault handler.

**seL4GDB-from.template.c** - Template for the fault ep on the component side. This should just be generating a cap, since it is set manually later.

**seL4Debug-from.template.c** - server-side code for the delegate function calls. These functions are called by the GDB server implementation.

**seL4Debug-to.template.c** - Component-side code for handling delegate calls. This will do the actual reading and writing and then send relevant info back to the GDB server.

**gdb.c** - GDB remote stub implementation.

**gdb.h** - GDB remote stub header.
<br></br>

### Inspecting output

You can communicate directly with the debugger using GDB remote protocol commands. These are the commands that GDB sends over the serial port to communicate with the debugger. Currently supported commands are listed below, for more information on commands read the [GDB serial protocol](https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html).


| Command   | Command Description       | Notes                                                                             |
|---------  |------------------------   |--------------------------------------------------------------------------------   |
| q         | Query                     | Working for queries required for GDB initialisation.                              |
| g         | Read general registers    | Complete                                                                          |
| ?         | Halt reason               | Complete for current functionality |
| Hc        | Set thread                | Handled for GDB initialisation, but otherwise ignored.                            |
| m/M/X     | Memory read / write       | Complete     |
| Z0        | Set SW breakpoint         | Complete, but requires writable instruction memory |
| Z1/2/3/4        | Set HW breakpoints        | Complete    |
| s               | Step                      | Complete    |

## TODO
A list of features that need to be implemented, roughly in order of importance.

### Divert serial ports correctly
Currently, diverting serial connections on QEMU diverts both debug and program output if there is only one serial port. Should find a way to divert a single COM port as opposed to all serial.

### Multi debugging
Support multi-debugging either through multiple serial ports (or ethernet) or using a lock on the serial port and using GDB's thread switch functionality.

### Other commands to implement

| Command   | Command Description               |
|---------  |---------------------------------- |
| D         | Detach                            |
| k         | Kill the target                   |