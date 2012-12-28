/* driver-os.h
 * Definitions for the os abstraction layer. These functions are intended to be
 * used internally by the spinapi library. If you are an end-user, you should
 * not call these functions but instead use the pb_in* and pb_out* functions.
 *
 * $Date: 2008/04/28 18:29:51 $
 *
 * To get the latest version of this code, or to contact us for support, please
 * visit http://www.spincore.com
 */

/* Copyright (c) 2008 SpinCore Technologies, Inc.
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

#ifndef _DRIVER_OS_H
#define _DRIVER_OS_H

int os_count_boards (int vend_id);

int os_init (int card_num);
int os_close (int card_num);

int os_outp (int card_num, unsigned int address, char data);
char os_inp (int card_num, unsigned int address);

int os_outw (int card_num, unsigned int addresss, unsigned int data);
unsigned int os_inw (int card_num, unsigned int address);

#endif
