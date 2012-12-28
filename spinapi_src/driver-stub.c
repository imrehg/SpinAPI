/* driver-stub.c
 * This file provides a skeleton for the necessary os-specific functions
 * needed by spinapi. To port spinapi to a given OS simply implement these
 * functions for the OS and compile it with the other spinapi source files.
 *
 * If multiple boards are present in a system, the driver implementor is
 * free to enumerate them in any order. The important thing is to keep
 * the order consistant as long as the order in which the boards
 * are plugged into the system is not changed.
 *
 * If an error occurs during any of these functions, spinerr should be
 * set to an appropriate value.
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


#include "driver-os.h"

extern char *spinerr;


/**
 * This function returns the number of boards with a given vendor id.
 *
 *\param vend_id The vendor ID user for SpinCore boards will be passed
 * as a paramter.
 *\return number of boards present, or -1 on error.
 */

int
os_count_boards (int vend_id)
{
  return 0;
}

/**
 * Initialize the OS so that it can access a given board. Nothing needs to be
 * written to the registers of the board itself. Rather this function should
 * perform some functions such as gaining access to the IO space used by the
 * board, etc. Basically this needs to do whatever must be done to ensure
 * that the os_outx, and os_inx functions will work properly.
 *
 * Any initialization that may be needed on the board itself is done at a higher
 * level after this function exits (by using os_outp, etc.) and the implementor
 * of this file need not be concerned with this procedure.
 *
 *\return -1 on error
 */
int
os_init (int card_num)
{
  return 0;
}

/**
 * End access with the board. This should do the opposite of whatever was
 * done is os_init()
 *\return -1 on error
 */
int
os_close (int card_num)
{
  return 0;
}

// The following functions read and write to the IO address space of the card.
//
// For all of these functions, address is given as an offset of the base address.
// For example if the card has the IO addresses 0x0400-0x040F, 
// os_outp(0, 4, 42)
// should write the value 42 to the absolute IO address 0x0404 of the first
// card present in the system.
//


/**
 * Write a byte of data to the given card, at given the address.
 * \return -1 on error
 */

int
os_outp (int card_num, unsigned int address, char data)
{
  return 0;
}

/**
 * Read a byte of data from the given card, at the given address
 * \return value from IO address
 */
char
os_inp (int card_num, unsigned int address)
{
  return 0;
}

/**
 * Write a 32 bit word to the given card, at the given address
 *\return -1 on error
 */

int
os_outw (int card_num, unsigned int addresss, unsigned int data)
{
  return 0;
}

/**
 * Read a 32 bit word from the given card, at the given address
 *\return value form IO address
 */

unsigned int
os_inw (int card_num, unsigned int address)
{
  return 0;
}
