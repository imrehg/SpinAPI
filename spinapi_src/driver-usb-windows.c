/* driver-usb-windows.c
 * This file implements the low-level spinapi code to interface with the Cypress USB
 * driver.
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
#include <string.h>
#include <Windows.h>
#include <Setupapi.h>
#include "cyioctl.h"
#include "driver-usb.h"
#include "usb.h"
#include <spinapi.h>
#include <util.h>

#define MAX_USB 128
#define MAX_IO_WAIT_TIME 500	//Max time to wait for a IO transfer (read or write) to complete in milliseconds.
#define MAX_IO_ATTEMPTS  3	    //Number of times to try a single transfer before giving up.

static HANDLE *h_list[MAX_USB];
int pid_list[MAX_USB];
extern char *spinerr;

typedef struct
{
  HANDLE hDevice;
  DWORD dwIoControlCode;
  LPVOID lpInBuffer;
  DWORD nInBufferSize;
  LPVOID lpOutBuffer;
  DWORD nOutBufferSize;
  LPDWORD lpBytesReturned;
  LPOVERLAPPED lpOverlapped;
} DEVICEIOCONTROLARGS;

// Helper function internal to the usb code. These should not be accessed outside this file
static HANDLE *usb_get_handle ();
static int usb_get_device_name (HANDLE * h);
static int usb_get_friendly_name (HANDLE * h);
static int usb_get_num_endpoints (HANDLE * h);
static unsigned int usb_get_driver_version (HANDLE * h);
//static int usb_get_power_state(HANDLE *h);
//static int usb_get_xfer_size(HANDLE *h, int pipe);
//static int usb_set_xfer_size(HANDLE *h, int pipe, unsigned int xfer_size);
static int usb_reset_pipe (HANDLE * h, int pipe);
static int usb_abort_pipe (HANDLE * h, int pipe);

const GUID CYUSBDRV_GUID =
  { 0xae18aa60, 0x7f6a, 0x11d4, {0x97, 0xdd, 0x00, 0x01, 0x02, 0x29, 0xb9,
				 0x59}
};

/**
 * 
 * 
 */
int
os_usb_count_devices (int vendor_id)
{
  int i, j, ret;

  SP_DEVINFO_DATA devInfoData;
  devInfoData.cbSize = sizeof (SP_DEVINFO_DATA);

  SP_DEVICE_INTERFACE_DATA devInterfaceData;
  devInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

  PSP_INTERFACE_DEVICE_DETAIL_DATA functionClassDeviceData;

  ULONG requiredLength = 0;

  HDEVINFO hwDeviceInfo = SetupDiGetClassDevs ((LPGUID) & CYUSBDRV_GUID,
					       NULL,
					       NULL,
					       DIGCF_PRESENT |
					       DIGCF_INTERFACEDEVICE);


  if (hwDeviceInfo == INVALID_HANDLE_VALUE)
    {
      debug ("os_usb_count_devices: SetupDiGetClassDevs() failed with %d\n",
	     (int) GetLastError ());
      return -1;
    }

  debug ("os_usb_count_devices: Enumerating USB Devices...\n");

  for (i = 0, j = 0; i < MAX_USB; ++i)
    {

      if (!SetupDiEnumDeviceInterfaces
	  (hwDeviceInfo, 0, (LPGUID) & CYUSBDRV_GUID, i, &devInterfaceData))
	{
	  if (!((ret = GetLastError ()) == ERROR_NO_MORE_ITEMS))
	    {
	      debug
		("os_usb_count_devices: SetupDiEnumDeviceInterfaces failed with error code %d!\n",
		 (int) GetLastError ());
	      break;
	    }
	  else
	    {
	      debug
		("os_usb_count_devices: Enumeration Completed. Found %d Devices.\n",
		 i);
	      break;
	    }
	}
      SetupDiGetInterfaceDeviceDetail (hwDeviceInfo, &devInterfaceData, NULL,
				       0, &requiredLength, NULL);

      functionClassDeviceData =
	(PSP_INTERFACE_DEVICE_DETAIL_DATA) malloc (requiredLength);
      functionClassDeviceData->cbSize =
	sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

      if (!SetupDiGetInterfaceDeviceDetail (hwDeviceInfo, &devInterfaceData,
					    functionClassDeviceData,
					    requiredLength,
					    &requiredLength, &devInfoData))
	{

	  debug ("usb_get_handle: SetupDiGetClassDevs() failed with %d\n",
		 (int) GetLastError ());
	  break;

	}

      if (strncmp
	  (functionClassDeviceData->DevicePath + 8, "vid_0403&pid_c1a9",
	   17) == 0)
	{
	  pid_list[j] = 0xc1a9;
	  debug ("-SP7 Board Detected.\n");
	}
      else
	if (strncmp
	    (functionClassDeviceData->DevicePath + 8, "vid_0403&pid_c2a9",
	     17) == 0)
	{
	  pid_list[j] = 0xc2a9;
	  debug ("-SP15 Board Detected.\n");
	}
      else
	if (strncmp
	    (functionClassDeviceData->DevicePath + 8, "vid_0403&pid_c1aa",
	     17) == 0)
	{
	  pid_list[j] = 0xc1aa;
	  debug ("-SP9 Board Detected.\n");
	}
      else
    if (strncmp
	    (functionClassDeviceData->DevicePath + 8, "vid_0403&pid_c1ab",
	     17) == 0)
	{
	  pid_list[j] = 0xc1ab;
	  debug ("-SP9 Board Detected (pid_c1ab).\n");
	}
      else
	{
	  debug ("-Cypress USB Chip detected. VID/PID unknown.\n");
	}

      if (pid_list[j] != 0)
	{
	  h_list[j] = CreateFile (functionClassDeviceData->DevicePath,
				  GENERIC_WRITE | GENERIC_READ,
				  FILE_SHARE_WRITE | FILE_SHARE_READ,
				  NULL,
				  OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	  debug ("Device Handle=0x%x\n", (unsigned int) h_list[j]);
	  debug ("--System Path: %s\n", functionClassDeviceData->DevicePath);
	  j++;
	}

      free (functionClassDeviceData);
    }

  SetupDiDestroyDeviceInfoList (hwDeviceInfo);

  return ((ret == ERROR_NO_MORE_ITEMS) ? i : ret);
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
  HANDLE *h;

  int num_endpoints;

  // Get a handle to the device
  h = usb_get_handle (dev_num);

  if (h == INVALID_HANDLE_VALUE)
    {
      spinerr = "Unable to get handle for device";
      debug ("os_usb_init: %s\n", spinerr);
      return -1;
    }

  h_list[dev_num] = h;

  num_endpoints = usb_get_num_endpoints (h);

  if (num_endpoints != 3)
    {
      spinerr = "Internal error: device has wrong # of endpoints";
      debug ("os_usb_init(): %s (found: %d)\n", spinerr, num_endpoints);
      return -1;
    }

  usb_get_device_name (h);
  usb_get_friendly_name (h);
  usb_get_driver_version (h);
  //usb_get_power_state(h);

  return pid_list[dev_num];

}

int
os_usb_close ()
{
  int i;

  for (i = 0; i < MAX_USB; ++i)
    {
      if (h_list[i] == INVALID_HANDLE_VALUE)
	break;
      else
	CloseHandle (h_list[i]);
    }
  return 0;

}


/**
 * Get a handle to the usb device
 * \returns INVALID_HANDLE_VALUE on failure
 */
static HANDLE *
usb_get_handle (int dev_num)
{
  return h_list[dev_num];
}

/**
 * Reset the transfer pipes for the given device. This will, among other things,
 * unfreeze the transfer if it hangs somehow.
 * 
 */
int
os_usb_reset_pipes (int dev_num)
{
  usb_abort_pipe (h_list[dev_num], EP1OUT);
  usb_abort_pipe (h_list[dev_num], EP2OUT);
  usb_abort_pipe (h_list[dev_num], EP6IN);

  usb_reset_pipe (h_list[dev_num], EP1OUT);
  usb_reset_pipe (h_list[dev_num], EP2OUT);
  usb_reset_pipe (h_list[dev_num], EP6IN);

  return 0;
}

// The USB device can accept at most 512 bytes in a single transfer
#define MAX_XFER_SIZE 512

#ifndef false
#define false 0
#endif

DWORD WINAPI
DeviceIoControlThread (LPVOID args)
{
  int ret;

  DEVICEIOCONTROLARGS *myArgs = (DEVICEIOCONTROLARGS *) args;

  ret = DeviceIoControl (myArgs->hDevice, myArgs->dwIoControlCode,
			 myArgs->lpInBuffer, myArgs->nInBufferSize,
			 myArgs->lpOutBuffer, myArgs->nOutBufferSize,
			 myArgs->lpBytesReturned, myArgs->lpOverlapped);

  if (ret == 0)
    {
      spinerr = "DeviceIoControlThread: USB Internal transfer error.";
      debug ("DeviceIoControlThread: DeviceIOControl() last error was %d\n",
	     (int) GetLastError ());
      Sleep (MAX_IO_WAIT_TIME);
    }

  return 0;
}


int
os_usb_io (BOOL read, int dev_num, int pipe, void *data, int size)
{
  DWORD dwReturnBytes;
  HANDLE hThread;
  DEVICEIOCONTROLARGS args;
  int nAttempts = 0;
  DEVICEIOCONTROLARGS *myArgs =  &args;
  void *pArgs = (void*)&args;
  char *ptr;

  int iXmitBufSize = sizeof (SINGLE_TRANSFER) + MAX_XFER_SIZE;
  UCHAR pXmitBuf[MAX_XFER_SIZE + sizeof (SINGLE_TRANSFER)];
  ZeroMemory (pXmitBuf, iXmitBufSize);
  PSINGLE_TRANSFER pTransfer = (PSINGLE_TRANSFER) pXmitBuf;
  //pTransfer->WaitForever = false;
  pTransfer->ucEndpointAddress = (UCHAR) pipe;
  pTransfer->IsoPacketLength = 0;
  pTransfer->BufferOffset = sizeof (SINGLE_TRANSFER);
  pTransfer->BufferLength = (ULONG) size;

  if (!read)
    {
      ptr = (char *) pTransfer + pTransfer->BufferOffset;
      memcpy (ptr, data, size);
    }

  args.hDevice = h_list[dev_num];
  args.dwIoControlCode = IOCTL_ADAPT_SEND_NON_EP0_TRANSFER;
  args.lpInBuffer = pXmitBuf;
  args.nInBufferSize = iXmitBufSize;
  args.lpOutBuffer = pXmitBuf;
  args.nOutBufferSize = iXmitBufSize;
  args.lpBytesReturned = &dwReturnBytes;
  args.lpOverlapped = NULL;
			 
  hThread =
    (HANDLE) CreateThread (NULL, 0, DeviceIoControlThread, pArgs, 0, NULL);
  
   SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);

  while (WaitForSingleObject (hThread, MAX_IO_WAIT_TIME) == WAIT_TIMEOUT)
    {
      nAttempts++;

      if (nAttempts > MAX_IO_ATTEMPTS)
	{
	  debug
	    ("os_usb_io: DeviceIOControl Error. Transfer failed. Please check that your board is properly connected.\n");
	    debug("Killing IO thread.\n");
	    TerminateThread(hThread, -1);
	  CloseHandle (hThread);
	  return -1;
	}

      debug
	("os_usb_io: DeviceIOControl Timeout Detected. Attempting to restart transfer. Attempt %d of %d.\n",
	 nAttempts, MAX_IO_ATTEMPTS);

      usb_abort_pipe (h_list[dev_num], pipe);
      usb_reset_pipe (h_list[dev_num], pipe);

      debug("Killing IO thread.\n");
      TerminateThread(hThread, -1);
      CloseHandle (hThread);

      debug ("os_usb_io: Restarting Last DeviceIOControl.\n");
      hThread =
	(HANDLE) CreateThread (NULL, 0, DeviceIoControlThread, pArgs, 0,
			       NULL);

    }

  CloseHandle (hThread);
  
  if (read)
    {
      ptr = (char *) pTransfer + pTransfer->BufferOffset;
      memcpy (data, ptr, size);
    }

  return 0;
}

int
os_usb_write (int dev_num, int pipe, void *data, int size)
{
  if (h_list[dev_num] == INVALID_HANDLE_VALUE)
    {
      spinerr = "Device not initialized\n";
      debug ("os_usb_write: %s\n", spinerr);
      return -1;
    }

  if (size > MAX_XFER_SIZE)
    {
      spinerr = "Transfer size is too big";
      debug ("os_usb_write: %s\n", spinerr);
      return -1;
    }


  return os_usb_io (0, dev_num, pipe, data, size);

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
  if (h_list[dev_num] == INVALID_HANDLE_VALUE)
    {
      spinerr = "Device not initialized\n";
      debug ("os_usb_write: %s\n", spinerr);
      return -1;
    }

  if (size > MAX_XFER_SIZE)
    {
      spinerr = "Transfer size is too big";
      debug ("os_usb_write: %s\n", spinerr);
      return -1;
    }

  return os_usb_io (1, dev_num, pipe, data, size);

}

///
/// The following functions a wrappers for each of the IOCTL functions that the Cypress
/// driver provides
///

#define BUF_SIZE 256

static int
usb_get_device_name (HANDLE * h)
{
  int ret;
  DWORD dwBytes = 0;
  ULONG len = BUF_SIZE;
  UCHAR buf[BUF_SIZE];

  ret = DeviceIoControl (h, IOCTL_ADAPT_GET_DEVICE_NAME,
			 buf, len, buf, len, &dwBytes, NULL);

  if (ret == 0)
    {
      debug ("usb_get_device_name: DeviceIOControl() error was %d\n",
	     (int) GetLastError ());
      return -1;
    }

  debug ("usb_get_device_name: device name is: \"%s\"\n", buf);
  return 0;
}

static int
usb_get_friendly_name (HANDLE * h)
{
  int ret;

  DWORD dwBytes = 0;
  UCHAR FriendlyName[BUF_SIZE];

  ret = DeviceIoControl (h, IOCTL_ADAPT_GET_FRIENDLY_NAME,
			 FriendlyName, BUF_SIZE,
			 FriendlyName, BUF_SIZE, &dwBytes, NULL);

  if (ret == 0)
    {
      debug ("usb_get_friendly_name: DeviceIOControl() error was %d\n",
	     (int) GetLastError ());
      return -1;
    }

  debug ("usb_get_friendly_name: friendly name is \"%s\"\n", FriendlyName);
  return 0;
}

static int
usb_get_num_endpoints (HANDLE * h)
{
  int ret;
  DWORD dwBytes = 0;
  UCHAR endPts;

  ret = DeviceIoControl (h, IOCTL_ADAPT_GET_NUMBER_ENDPOINTS,
			 NULL, 0, &endPts, sizeof (endPts), &dwBytes, NULL);

  if (ret == 0)
    {
      debug ("usb_get_num_endpoints: DeviceIOControl() error was %d\n",
	     (int) GetLastError ());
      return -1;
    }

  debug ("usb_get_num_endpoints: # of endpoints: %d\n", endPts);

  return (int) endPts;
}


static unsigned int
usb_get_driver_version (HANDLE * h)
{
  int ret;

  DWORD dwBytes = 0;
  ULONG ver;

  ret = DeviceIoControl (h, IOCTL_ADAPT_GET_DRIVER_VERSION,
			 &ver, sizeof (ver),
			 &ver, sizeof (ver), &dwBytes, NULL);

  if (ret == 0)
    {
      debug ("usb_get_driver_version: DeviceIOControl() error was %d\n",
	     (int) GetLastError ());
      return -1;
    }

  debug ("usb_get_driver_version: Driver version is 0x%x (%d decimal)\n",
	 (unsigned int) ver, (unsigned int) ver);

  return (unsigned int) ver;
}

// Note: this always fails for some reason. check cypress documentation if you want to get it working,
// but we ignore it for now
/*static int usb_get_power_state(HANDLE *h)
{
    int ret;

    DWORD dwBytes = sizeof(UCHAR); 
    UCHAR pwrState; 

    ret = DeviceIoControl(h, IOCTL_ADAPT_GET_DEVICE_POWER_STATE, 
                    &pwrState,  sizeof (pwrState),      
                    &pwrState, sizeof (pwrState), 
                    &dwBytes, NULL);     

    if(ret == 0) {
        debug("usb_get_power_state: DeviceIOControl() error was %d\n", (int)GetLastError());        
        return -1;
    }
    
    debug("usb_get_power_state: Power state is: %d\n", (int)pwrState);
    
    return (int)pwrState;
}

// Transfer size is independant of the endpoints buffer size. Transfer size must be
// a multiple of the buffer size.
static int usb_get_xfer_size(HANDLE *h, int pipe)
{
    int ret;

    DWORD BytesXfered; 
    SET_TRANSFER_SIZE_INFO SetTransferInfo; 
    SetTransferInfo.EndpointAddress = (UCHAR)pipe; 

    ret = DeviceIoControl(h,  IOCTL_ADAPT_GET_TRANSFER_SIZE, 
                    &SetTransferInfo,  sizeof (SET_TRANSFER_SIZE_INFO), 
                    &SetTransferInfo,  sizeof (SET_TRANSFER_SIZE_INFO), 
                    &BytesXfered,  NULL); 

    if(ret == 0) {
        debug("usb_get_xfer_size: DeviceIOControl() error was %d\n", (int)GetLastError());        
        return -1;
    }

    debug("usb_get_xfer_size: Transfer size was (pipe 0x%x): %d\n", pipe, (int)SetTransferInfo.TransferSize); 

    return (int)SetTransferInfo.TransferSize; 
}

static int usb_set_xfer_size(HANDLE *h, int pipe, unsigned int xfer_size)
{
    int ret;

    DWORD BytesXfered; 
    SET_TRANSFER_SIZE_INFO  SetTransferInfo; 
    SetTransferInfo.EndpointAddress  = (UCHAR)pipe; 
    SetTransferInfo.TransferSize = (ULONG)xfer_size;

    ret = DeviceIoControl(h,  IOCTL_ADAPT_SET_TRANSFER_SIZE, 
                    &SetTransferInfo,  sizeof (SET_TRANSFER_SIZE_INFO), 
                    &SetTransferInfo,  sizeof (SET_TRANSFER_SIZE_INFO), 
                    &BytesXfered,  NULL); 

    if(ret == 0) {
        debug("usb_set_xfer_size: DeviceIOControl() error was %d\n", (int)GetLastError());
        return -1;
    }

    return 0;
}
*/

static int
usb_reset_pipe (HANDLE * h, int pipe)
{
  int ret;
  DWORD dwBytes;
  UCHAR Address = (UCHAR) pipe;

  ret = DeviceIoControl (h, IOCTL_ADAPT_RESET_PIPE,
			 &Address, sizeof (Address), NULL, 0, &dwBytes, NULL);

  if (ret == 0)
    {
      debug ("usb_reset_pipe: DeviceIOControl() error was %d\n",
	     (int) GetLastError ());
      return -1;
    }

  return 0;
}

static int
usb_abort_pipe (HANDLE * h, int pipe)
{
  int ret;
  DWORD dwBytes = 0;
  UCHAR Address = (UCHAR) pipe;

  ret = DeviceIoControl (h, IOCTL_ADAPT_ABORT_PIPE,
			 &Address, sizeof (UCHAR), NULL, 0, &dwBytes, NULL);

  if (ret == 0)
    {
      debug ("usb_abort_pipe: DeviceIOControl() error was %d\n",
	     (int) GetLastError ());
      return -1;
    }

  return 0;
}
