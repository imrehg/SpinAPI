/* usb.h
 * 
 * This header file defines the API for the functions which implement the high-level
 * usb protocol.
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

#ifndef USB_H_
#define USB_H_

int usb_write_reg (unsigned int addr, unsigned int data);
int usb_read_reg (unsigned int addr, unsigned int *data);
int usb_read_ram (unsigned int bank, unsigned int start_addr,
		  unsigned int len, char *data);
int usb_write_ram (unsigned int bank, unsigned int start_addr,
		   unsigned int len, char *data);
int usb_write_address (int addr);
int usb_write_data (int *data, int nData);

void usb_set_device (int board_num);

int usb_do_outp (unsigned int address, char data);

int usb_reset_gpif (int dev_num);

// RAM banks for usb_{read,write}_ram
#define BANK_DATARAM 0x1000
#define BANK_DDSRAM 0x2000

// Definitions for low-level transfer stuff
#define ADDR_REG_EN 0x80
#define RST_L 0x40
#define DO_RENUM 0x20
#define DO_GPIF_RESET 0x10

#define DO_LITE 0x02
#define DO_HEAVY 0x01

#define EP1OUT 0x01
#define EP2OUT 0x02
#define EP6IN  0x86

#endif /*USB_H_ */
