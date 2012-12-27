Spinapi drivers for Linux
---------------

Table of Contents
-----------------
1. Overview
2. Driver Installation
3. Linux Support
4. Contact Info

This is spinapi version 2011-09-13

Overview:
=========

This archive contains a unified driver which will work for PB24, PBDDS-III, PulseBlasterESR, PulseBlasterESR-PRO and RadioProcessor boards. The included driver will function on all versions of Windows as well as x86 Linux. Example programs which use spinapi are included for each supported model.

The contents of each directory is explained below.

general        - example programs for all boards
PB24           - example programs for PulseBlaster24
PBDDS-III      - example programs for PulseBlasterDDS-III
PBESR          - example programs for PulseBlasterESR
PBESR-PRO      - example programs for PulseBlasterESR-PRO
RadioProcessor - example programs for the RadioProcessor
spinapi_source - contains the source code for the spinapi library


Linux support:
==============

Spinapi can be used with Linux on x86 processors. To get your programs running on Linux:

1. In the spinapi_source/ directory, type "make". You maybe required to provide permission as root. This will create a file called "libspinapi.a" which you can link your programs against to use SpinCore hardware. 
Please check if the file "spinapi.h" is copied under directory "/usr/include/".

2. Install libusb v0.1. The source package is available at:
        http://sourceforge.net/projects/libusb/files/libusb-0.1%20%28LEGACY%29/
   Under debian based systems, run:
        sudo apt-get install libusb-dev

3. To compile your program, you must link with the spinapi, math, libdl and usb libraries. For example, when compiling the "pb24_ex1.c" example program, you would use a command like (assuming both libspinapi.a and pb24_ex1.c are in the current directory):

    gcc -opb24_ex1 pb24_ex1.c -L. -lspinapi -lm -ldl -lusb 

This will create an executable called "pb24_ex1".

4. When running programs, you MUST have superuser priveledges (ie be running as root). This is because the spinapi library needs to be able to gain access to the low-level hardware resources.


Contact Info:
=============

Thank you for choosing a design from SpinCore Technologies, Inc. We appreciate your buisness! At SpinCore we try to fully support the needs of our customers. If you are in need of assistance, please contact us and we will strive to provide the necessary support. Please see our contact information below for any questions/comments you may have.

SpinCore Technologies, Inc.  
4623 NW 53rd Avenue, SUITE 5  
Gainesville, FL 32653  
USA  

Phone:    (USA) 352-271-7383  
Fax:      (USA) 352-371-8679
Internet: http://www.spincore.com
