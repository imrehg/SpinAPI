/* driver-usb-stub.c
 * 
 * This file implements dummy versions of the low-level usb interface functions. This
 * can be used on operating systems where USB is not (yet) supported. It can also
 * be used as a template to create drivers for new operating systems.
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

/**
 * 
 * 
 */
int
os_usb_count_devices (int vendor_id)
{
  return 0;
}

/**
 * 
 * 
 * \returns A negative number is returned on error and spinerr is set to a
 * description of the error. 0 is returned on success
 */
int
os_usb_init (int dev_num)
{
  return 0;
}

int
os_usb_close ()
{
  return 0;
}

int
os_usb_reset_pipes (int dev_num)
{
  return 0;
}

/**
 * Write data to the USB device
 * \param pipe endpoint to write data too
 * \param data buffer holding data to be written
 * \param size Size in bytes of the buffer
 * \returns 0 on success, or a negative number on failure
 */
int
os_usb_write (int dev_num, int pipe, void *data, int size)
{
  return 0;
}

/**
 * Read data from the USB device.
 * \param pipe Endpoint to read data from
 * \param data Buffer to hold the data that will be read
 * \param size Size in bytes of data to read
 * \returns 0 on success, or a negative number on failure
 */
int
os_usb_read (int dev_num, int pipe, void *data, int size)
{
  return 0;
}
