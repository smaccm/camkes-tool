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
1. [Quick Start Guide](#quick)
2. [Usage](#usage)
3. [Developers](#developers)
4. [TODO](#todo)

##  Quick Start Guide <a name="quick"></a> ##
This is a guide to run the debugger on an example application.