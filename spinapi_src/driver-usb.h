/* driver-usb.h
 * 
 * This header file defines the API for the low-level USB routines
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

#ifndef DRIVER_USB_H_
#define DRIVER_USB_H_

int os_usb_count_devices (int vendor_id);
int os_usb_init (int dev_num);
int os_usb_close ();
int os_usb_write (int dev_num, int pipe, void *data, int size);
int os_usb_read (int dev_num, int pipe, void *data, int size);
int os_usb_reset_pipes (int dev_num);

#endif /*DRIVER_USB_H_ */
