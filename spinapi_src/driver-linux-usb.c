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
 
#include <stdio.h>
#include <stdlib.h>
#include <usb.h>

#include "usb.h"
#include "util.h"

#define VENDOR_ID 0x0403
#define MAX_IO_WAIT_TIME 500

extern char* spinerr;

static usb_dev_handle** handles = 0;
static struct usb_device** devices = 0;

/**
 * Count SpinCore devices on the USB bus.
 * 
 */
int os_usb_count_devices(int vendor_id)
{
    int d = 0;

    struct usb_bus* bus;
    struct usb_bus* busses;
    struct usb_device* device;

    debug("os_usb_count_devices called\n");
    
    usb_init();
    usb_find_busses();
    usb_find_devices();

    busses = usb_get_busses();

    // Check all the USB busses.
    for (bus = busses; bus; bus = bus->next)
    {
        // And on each bus, check each device.
        for (device = bus->devices; device; device = device->next)
        {
            if (device->descriptor.idVendor == VENDOR_ID)
                d++;
        }
    }

	return d;
}

/**
 * Unique USB device identifier is idVendor << 16 | idProduct
 * 
 * \returns A negative number is returned on error and spinerr is set to a
 * product id of device on success
 * description of the error. 0 is returned on success
 */
int os_usb_init(int dev_num)
{
    struct usb_bus* bus;
    struct usb_bus* busses;
    struct usb_device* device;
    int number_of_devices = os_usb_count_devices(VENDOR_ID);
    int d = dev_num;
    int interface;

    debug("os_usb_init called\n");

    // initialize array of handles, if necessary.
    if (!handles)
        handles = calloc(number_of_devices, sizeof(usb_dev_handle*));
    if (!devices)
        devices = calloc(number_of_devices, sizeof(struct usb_device*));
    
    usb_init();
    usb_find_busses();
    usb_find_devices();

    busses = usb_get_busses();

    // find the device by counting through the devices.
    // this is inefficient, but most likely, only a small number of devices are involved.
    // Check all the USB busses.
    for (bus = busses; bus; bus = bus->next)
    {
        // And on each bus, check each device.
        for (device = bus->devices; device; device = device->next)
        {
            if (device->descriptor.idVendor == VENDOR_ID)
                d--;
            if (d == -1)
                break;
        }
        if (d == -1)
            break;
    }
    
    if (d != -1)
    {
    	debug("os_usb_init: device not found.\n");
        spinerr = "Device not found.";
        return -1;
    }

    devices[dev_num] = device;
    handles[dev_num] = usb_open(device);
    
    if (!handles[dev_num])
    {
        spinerr = "Handle failed.";
        debug("os_usb_init: handle not set.\n");
        return -1;
    }

    /* Only interface the boards provide. */
    interface = usb_claim_interface(handles[dev_num], 0);

    if (interface < 0)
    {
    	debug("os_usb_init: could not claim interface.\n");
        spinerr = "Could not claim interface.";
        return -1;
    }

    return device->descriptor.idProduct;
}

int os_usb_close()
{
    debug("os_usb_close called\n");
    int number_of_devices = os_usb_count_devices(VENDOR_ID);
    int i;

    for (i = 0; i < number_of_devices; i++)
        if (handles[i]) {
            debug("os_usb_close: closing device %d\n", i);
            if (usb_release_interface(handles[i], 0))
            	return -2;
            if (usb_close(handles[i]) < 0)
                return -1;
        }

    return 0;   
}

int os_usb_reset_pipes(int dev_num)
{
    debug("os_usb_reset_pipes called\n");
    if (usb_clear_halt(handles[dev_num], EP1OUT) < 0)
        return -1;
    
    if (usb_clear_halt(handles[dev_num], EP2OUT) < 0)
        return -1;

    if (usb_clear_halt(handles[dev_num], EP6IN) < 0)
        return -1;

    return 0;
}

/**
 * Write data to the USB device
 * \param pipe endpoint to write data too
 * \param data buffer holding data to be written
 * \param size Size in bytes of the buffer
 * \returns 0 on success, or a negative number on failure
 */
int os_usb_write(int dev_num, int pipe, void *data, int size)
{
    debug("os_usb_write(dev_num = %d, pipe = 0x%X, data, size = %d)\n", dev_num, pipe, size);

    int bytes_written = usb_bulk_write(handles[dev_num], pipe, data, size, MAX_IO_WAIT_TIME);
    if (bytes_written < 0)
    {
        spinerr = "write error.";
        return -1;
    }
    debug("number of bytes written: %d\n", bytes_written);
    
    return  0;
}

/**
 * Read data from the USB device.
 * \param pipe Endpoint to read data from
 * \param data Buffer to hold the data that will be read
 * \param size Size in bytes of data to read
 * \returns 0 on success, or a negative number on failure
 */
int os_usb_read(int dev_num, int pipe, void *data, int size)
{
    debug("os_usb_read(dev_num = %d, pipe = 0x%X, data, size = %d)\n", dev_num, pipe, size);

    int bytes_read = usb_bulk_read(handles[dev_num], pipe, data, size, MAX_IO_WAIT_TIME);
    if (bytes_read < 0)
    {
        spinerr = "Read error.";
        return -1;
    }
    debug("number of bytes read: %d\n", bytes_read);

    return  0;
}
