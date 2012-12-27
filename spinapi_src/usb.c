/* usb.c
 * This module implements the high-level usb communications protocol.
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

#ifdef WINDOWS
#include <Windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "driver-usb.h"
#include "util.h"
#include "usb.h"
#include "if.h"
#include "spinapi.h"

extern char *spinerr;
//extern int pid_list[128];

int setup_xfer (unsigned int addr, unsigned int packet_len);

int cur_dev = 0;


/**
 * \internal
 * This function sets the current USB device. If the device is not set by pb_select_board()
 * multiple board will not function properly.
 * 
 * \param board_num
 * \return
 */

void
usb_set_device (int board_num)
{
  debug ("usb_set_device: board num %d\n", board_num);
  cur_dev = board_num;
}

/**
 *
 * \param addr
 * \param data
 * \return 
 */
int
usb_write_address (int addr)
{
  char buf;
  char addr_buf[4];

  addr_buf[3] = (addr >> 24) & 0x0FF;
  addr_buf[2] = (addr >> 16) & 0x0FF;
  addr_buf[1] = (addr >> 8) & 0x0FF;
  addr_buf[0] = addr & 0x0FF;

  buf = ADDR_REG_EN | RST_L | 0x01;
  if (os_usb_write (cur_dev, EP1OUT, &buf, 1) < 0)
    return -1;

  if (os_usb_write (cur_dev, EP2OUT, addr_buf, 4) < 0)
    return -1;

  // now disable the address register (and thus enable
  // data registers)
  buf = RST_L | 0x01;

  if (os_usb_write (cur_dev, EP1OUT, &buf, 1) < 0)
    return -1;

    return 0;
}

int
usb_write_data (int *data, int nData)
{
  char *data_buf = (char *) data;

  int num_bytes;
  int num_xfers;
  int excess_xfer;
  int i = 0;

  num_bytes = nData * sizeof (int);
  num_xfers = (int) (num_bytes / 512);
  excess_xfer = num_bytes - 512 * num_xfers;

  for (i = 0; i < num_xfers; ++i)
    if (os_usb_write (cur_dev, EP2OUT, data_buf + 512 * i, 512) < 0)
      return -1;

  if (os_usb_write (cur_dev, EP2OUT, data_buf + 512 * i, excess_xfer) < 0)
    return -1;

  return 0;
}

int
usb_write_reg (unsigned int addr, unsigned int data)
{
  int ret;

  ret = setup_xfer (addr, 4);
  if (ret < 0)
    {
      spinerr = "Error setting up transfer";
      debug ("usb_write_reg: %s\n", spinerr);
      return ret;
    }

  ret = os_usb_write (cur_dev, EP2OUT, &data, 4);
  if (ret < 0)
    {
      spinerr = "Error doing write";
      debug ("usb_write_reg: %s\n", spinerr);
      return ret;
    }

  return 0;
}


int
usb_read_reg (unsigned int addr, unsigned int *data)
{
  int ret;
  int i;

  // read 3 times. the first two will bring back whatever
  // was in the buffers because the chip is double buffered
  // the third read will have the correct value

  for (i = 0; i < 3; i++)
    {
      ret = setup_xfer (addr, 4);
      if (ret < 0)
	{
	  spinerr = "Error setting up transfer";
	  debug ("usb_read_reg: %s (i=%d)\n", spinerr, i);
	  return ret;
	}

      ret = os_usb_read (cur_dev, EP6IN, data, 4);

      if (ret < 0)
	{
	  spinerr = "Error doing read";
	  debug ("usb_read_reg: %s (i=%d)\n", spinerr, i);
	  return ret;
	}
    }

  return 0;
}



/**
 * \internal 
 * This function actually does the ram reads. Before it starts reading, it starts the watchdog timer.
 * Then after every call to os_usb_read(), we check to see if the watchdog timer ran out. If it has,
 * that means that the transfer timed out, and we must restart the entire transfer. The watchdog
 * timer code will reset the endpoints automatically on a timeout. FYI, if the transfer times out
 * and we DONT have a way to automaitally reset it, the only way the use can recover is by cycling
 * the power on their board.
 * 
 * (note: I am not sure why these timeouts occur, and whether it is a problem with our firmware, with
 * spinapi, with the Cypress drivers, or something else)
 * 
 */
int
usb_read_ram (unsigned int bank, unsigned int start_addr, unsigned int len,
		 char *data)
{
  char inbuf[512];
  int i;

  char *ptr;

  char buf[3];

  int num_xfers;
  int excess_xfer;
  int xfer_size;
  int line_size;

  int amount_xferred = 0;

  switch (bank)
    {
    case BANK_DATARAM:
      line_size = 8;
      xfer_size = 512;
      usb_write_reg (0x0012, start_addr);
      break;
    case BANK_DDSRAM:
      debug ("usb_read_ram: DDRSRAM is write only\n");
      return -1;
      break;
    default:
      debug ("usb_read_ram: invalid RAM bank\n");
      return -1;
      break;
    }

  num_xfers = (len + line_size) / xfer_size;
  excess_xfer = (len + line_size) - num_xfers * xfer_size;

  if (len % line_size != 0)
    {
      debug ("usb_read_ram: length is not multiple of line size\n");
      return -1;
    }

  buf[2] = (xfer_size >> 8) & 0x0FF;
  buf[1] = xfer_size & 0x0FF;
  buf[0] = RST_L | DO_LITE;

  ptr = data;

  // setup transfer to work on dataram
  setup_xfer (bank, xfer_size);

  for (i = 0; i < num_xfers + 2; i++)
    {

      if (os_usb_write (cur_dev, EP1OUT, buf, 1) < 0)	// no reset, address register enable is disabled
        return -1;

      if (os_usb_read (cur_dev, EP6IN, inbuf, xfer_size) < 0)
	{
	  debug ("usb_read_ram: read not successful (xfer %d)\n", i);
	  return -1;
	}

      if (i == 0 || i == 1)
	{
	  continue;
	}

      // on the first real transfer, skip the first line
      if (i == 2)
	{
	  memcpy (ptr, inbuf + line_size, xfer_size - line_size);
	  ptr += xfer_size - line_size;
	}
      else
	{
	  memcpy (ptr, inbuf, xfer_size);
	  ptr += xfer_size;
	}

    }

  // if there is excess left to read, read an entire xfer_size block, but only
  // copy the part we want
  if (excess_xfer != 0)
    {
      if (os_usb_write (cur_dev, EP1OUT, buf, 1) < 0)	// no reset, address register enable is disabled
        return -1;

      if (os_usb_read (cur_dev, EP6IN, inbuf, xfer_size) < 0)
	{
	  debug ("usb_read_ram: read not successful (excess)\n");
	  return -1;
	}

      memcpy (ptr, inbuf, excess_xfer);
    }

  // read two more times to clear out the buffer
  if (os_usb_read (cur_dev, EP6IN, inbuf, xfer_size) < 0)
    {
      debug ("usb_read_ram: read not successful (clear 1)\n");
      return -1;
    }

  if (os_usb_read (cur_dev, EP6IN, inbuf, xfer_size) < 0)
    {
      debug ("usb_read_ram: read not successful (clear 2)\n");
      return -1;
    }

  reg_read (REG_CONTROL);
  reg_read (REG_CONTROL);

  return amount_xferred;
}

/**
 * 
 * 
 */

int
usb_write_ram (unsigned int bank, unsigned int start_addr, unsigned int len,
	       char *data)
{
  char *outbuf;
  int i;
  char *ptr;

  int num_xfers;
  int xfer_size;
  int line_size;
  int excess_xfer;

  switch (bank)
    {
    case BANK_DATARAM:
      line_size = 8;
      xfer_size = 512;
      usb_write_reg (0x0012, start_addr);
      break;

    case BANK_DDSRAM:
      line_size = 1;
      xfer_size = 512;
      break;

    default:
      debug ("usb_write_ram: invalid bank (0x%x)\n", bank);
      return -1;
    }


  num_xfers = len / xfer_size;
  excess_xfer = len - num_xfers * xfer_size;

  if (len % line_size != 0)
    {
      debug ("usb_write_ram: length is not multiple of line size\n");
      return -1;
    }

  outbuf = (char *) malloc (xfer_size);

  if (!outbuf)
    {
      debug ("usb_write_ram: couldnt allocate scratchpad memory\n");
      return -1;
    }

  ptr = data;

  // setup transfer to work on pbram
  setup_xfer (bank, xfer_size);

  for (i = 0; i < num_xfers; i++)
    {

      memcpy (outbuf, ptr, xfer_size);

      if (os_usb_write (cur_dev, EP2OUT, outbuf, xfer_size) < 0)
	{
	  debug ("write not succesfful (xfer %d)\n", i);
	  return -1;
	}

      ptr += xfer_size;
    }

  if (excess_xfer != 0)
    {
      memcpy (outbuf, ptr, excess_xfer);
      if (os_usb_write (cur_dev, EP2OUT, outbuf, excess_xfer) < 0)
	{
	  debug ("write not succesfful (excess xfer)\n");
	  return -1;
	}
    }


  free (outbuf);

  return 0;
}

/**
 * \internal
 * This initializes the GPIF functionality of the USB interface chip. This should always be called
 * immediatly after the board is initialized (or any transfers to/from the board will not work.
 * 
 */
int
usb_reset_gpif (int dev_num)
{
  char buf[3];

  buf[0] = RST_L | DO_GPIF_RESET | 0x01;
  if (os_usb_write (dev_num, EP1OUT, buf, 1) < 0)
    return -1;

  return 0;
}

/**
 * 
 * 
 * 
 */
int
setup_xfer (unsigned int addr, unsigned int packet_len)
{
  char buf[3];
  char addr_buf[4];

  addr_buf[3] = (addr >> 24) & 0x0FF;
  addr_buf[2] = (addr >> 16) & 0x0FF;
  addr_buf[1] = (addr >> 8) & 0x0FF;
  addr_buf[0] = addr & 0x0FF;

  buf[2] = (packet_len >> 8) & 0x0FF;
  buf[1] = packet_len & 0x0FF;

  // set the xfer counter (which is only used for IN xfers)
  buf[0] = ADDR_REG_EN | RST_L | 0x01;
  if (os_usb_write (cur_dev, EP1OUT, buf, 3) < 0)
    return -1;

  // write the actual address
//  if (pid_list[cur_dev] == 0xc2a9 || pid_list[cur_dev] == 0xc1ab)
//    {
      if (os_usb_write (cur_dev, EP2OUT, addr_buf, 4) < 0)
        return -1;
//    }
  else
    if (os_usb_write (cur_dev, EP2OUT, addr_buf, 2) < 0)
      return -1;
  // now disable the address register (and thus enable
  // data registers)
  buf[0] = RST_L | 0x01;

  if (os_usb_write (cur_dev, EP1OUT, buf, 3) < 0)
    return -1;

  return 0;
}


/**
 * Mimic the effect of doing an outp. This writes the data to the REG_PBCORE register, which is
 * in turn connected to the isa bridge component internally and will initate an ISA transfer
 * to the PulseBlaster core.
 * 
 * Also, this should only ever get called when the spinapi high-level code is talking to
 * the PulseBlaster Core. All other resources on usb devices can be accessed be read/writing
 * registers or RAM directly with the above functions.
 * 
 */
int
usb_do_outp (unsigned int address, char data)
{
  unsigned int word;

  word = 0x0FF & data;
  word = word | ((0x07 & address) << 8);
  unsigned int load_flag = 0x01 << 11;

  if (usb_write_reg (REG_PBCORE, word) < 0)
    {
      debug ("pb_outp_usb: error 1\n");
      return -1;
    }
  if (usb_write_reg (REG_PBCORE, word | load_flag) < 0)
    {
      debug ("pb_outp_usb: error 2\n");
      return -1;
    }

  return 0;
}
