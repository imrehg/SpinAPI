Spinapi source code

SpinCore Technologies, Inc.
www.spincore.com

I. Introduction
===============

This directory contains the source code files used to build the spinapi control
library. This source code is provided for advanced users who need to port this
library to other operating systems, or who wish to understand how it works.

Users who simply need to USE the spinapi library can make use of the pre-compiled
version and can ignore the contents of this directory entirely. Instructions
for compiling to programs that make use of the spinapi library can be found in
the top-level spinapi directory.

Modified versions of spinapi cannot be officially supported, however we 
would be happy to integrate any useful modifications or enhancements into the
official source distribution.
 
Use of this source code is subject to the terms set out in the license below
in part IV.


II. Building spinapi from source:
=================================

The provided makefile will build a windows .dll for spinapi. It is probably
somewhat specific to our internal build system, so some modifications may
be necessary depending on the compiler being used.

Makefile.linux can be used on linux systems.

To build the library, the following files must be compiled and linked
together:
  caps.c
  if.c
  spinapi.c
  util.c

In addition to these four files, an os specific file called driver-xxx.c 
and driver-usb-xxx.c (where xxx is the name of an os) is needed to provide
os-specific functions.

On Windows, this is the precompiled library libdriver-windows.a. This uses
3rd party software from Jungo Ltd. to access the hardware. Unfortunately, the
source for libdriver-windows.a contains proprietary code so the source
cannot be released.


III. Porting spinapi to other Operating Systems:
================================================

The spinapi code itself should be fairly portable. To make spinapi usable on
a specific OS, a driver-xxx.c file must be created to provide the
os specific parts. driver-stub.c contains a template for this file with a 
description of what each function needs to do. To port spinapi to any given
os, simply implement this driver file for that os and link it with the four
main files listed in part II above.


IV. License
===========

/* Copyright (c) 2009 SpinCore Technologies, Inc.
 *
 * This software is provided 'as-is', without any express or implied warranty. 
 * In no event will the authors be held liable for any damages arising from the 
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose, 
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be appreciated
 * but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
