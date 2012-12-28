/* usbtest.c
 *
 * This file contains some tests for transfer to and from the RadioProcessor USB board.
 * It also can reset the firmware.
 * 
 * This code is used for our own internal debugging procedures. It is of no use to customers.
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

// Need this for the timing functions
#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>

#include "spinapi.h"
#include "if.h"
#include "usb.h"
#include "driver-usb.h"

extern int cur_dev;

extern char version[];

void regtest ();
int ramtest ();
void do_renum ();
void print_data (int size, char *data_buf);



int
main (int argc, char **argv)
{

  srand (time (0));

  printf ("This statically linked program is based off of spinapi %s\n",
	  version);
  printf
    ("(You have to recompile this to use a new version of spinapi. Try 'make usb'\n");

  os_usb_count_devices (0);

  if (os_usb_init (cur_dev) < 0)
    {
      printf ("###############################\n");
      printf ("####Error Intializing board####\n");
      system ("PAUSE");
      return -1;
    }
  usb_reset_gpif (cur_dev);
  cur_dev = 0;


  printf ("USB test, new and improved!!\n");
  printf ("\n-----------------------------\n\n");

  // do a hard reset
  char buf[3];
  buf[0] = 0;
  os_usb_write (cur_dev, EP1OUT, buf, 3);

  // enable the PCI_READ bit
  usb_write_reg (REG_CONTROL, 0x02);

  int arg;
  do
    {
      printf ("---------------------------\n");
      printf ("What would you like to do?\n");
      printf ("0: Quit program\n");
      printf ("1: Reset firmware\n");
      printf ("2: Run Register test\n");
      printf ("3: Run RAM test\n");
      printf ("---------------------------\n");
      printf ("Choice:");
      scanf ("%d", &arg);

      switch (arg)
	{
	case 0:
	  break;
	case 1:
	  do_renum ();
	  system ("PAUSE");
	  exit (0);
	  break;
	case 2:
	  regtest ();
	  break;
	case 3:
	  ramtest ();
	  system ("PAUSE");
	  break;
	}
    }
  while (arg);




//    system("PAUSE");

  return 0;
}


/**
 * Make device renumerate, and reload itself with the default hardware. This will allow you to re-program
 * the firmware using the Cypress tools.
 */
void
do_renum ()
{
  char buf[3];

  printf
    ("Resetting device to use default firmware. After this, you will be able to communicate with the device using the Cypress tools\n");
  buf[0] = DO_RENUM | DO_HEAVY;
  os_usb_write (cur_dev, EP1OUT, buf, 3);

  printf
    ("Done. If you don't make any changes with the Cypress tools, our custom firmware will be reloaded next time you cycle the power on the device.\n");
}

/**
 * Check if register read/writes work. This works by writing a rnadom value to the
 * register, then reading it back to make sure it is the same. If you extend this
 * to work with other registers beside register 0, keep in mind that not all registers
 * are readable by the usb core (they are write-only), so this technique wont work with
 * all registers without changes to the firmware.
 * 
 */
void
regtest ()
{
  int write_val;
  int read_val;
  int i, j, k;
  int err = 0;

  int reg;

  int num_trys = 10;		// # of times to r/w the register

  int byte_mask = 0x000000ff;
  int bit_mask = 0x01;


  printf
    ("This test makes sure the usb_write_reg() and usb_read_reg() functions work properly. Right now, it only tests register 0 (the debugging register not used for anything)\n");

  int bit[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  for (i = 0; i < num_trys; i++)
    {
      reg = 0;

      // rand only spits out 15 bits, but we want to write to all 32
      write_val =
	(0x07FFF & rand ()) | ((0x07FFF & rand ()) << 15) | ((0x03 & rand ())
							     << 31);

      read_val = 0;

      usb_write_reg (reg, write_val);
      usb_read_reg (reg, &read_val);

      if (write_val != read_val)
	{
	  printf ("try %d error: wrote 0x%x, read 0x%x\n", i, write_val,
		  read_val);
	}

      for (j = 0; j < 4; j++)
	{
	  bit_mask = 0x01;
	  for (k = 0; k < 8; k++)
	    {
	      if ((byte_mask & write_val & bit_mask) !=
		  (byte_mask & read_val & bit_mask))
		{
		  bit[k]++;
		  err = 1;
		}
	      bit_mask = bit_mask << 1;
	    }

	  read_val = read_val >> 8;
	  write_val = write_val >> 8;
	}

    }

  // This prints the percentage of the time each individual bit is incorrect. On a successful
  // run, this should be 0 all the way down. But if not, this can help determine whether or
  // not a particualr bit is stuck or otherwise having problems.
  printf("\n Percentage error for each bit (should all be zero)\n");
  for (j = 0; j < 8; j++)
    {
      printf ("\tbit %d: %f\n", j, (float) bit[j] / (float) (num_trys * 4));
    }


  usb_read_reg (0x0015, &read_val);
  printf ("firmware id was 0x%x\n", read_val);

  if (!err)
    {
      printf ("+------------------------------+\n");
      printf ("| register test was succesfull |\n");
      printf ("+------------------------------+\n");
    }
  else
    {
      printf ("register test was NOT successfull\n");
    }
}

int
ramtest ()
{
  int size = 256 * 1024 * 8;	// 256k points, which is the entire memory (each point is 8 bytes)
  int i;
  int err = 0;
  int reps = 1;

  unsigned int write_start, read_start, write_end, read_end;

  printf
    ("This function makes sure the usb_write_ram() and usb_read_ram() functions work by reading and writing from the data point ram. This reads and writes the ENTIRE contents of RAM.\n");

  char *write_buf = (char *) malloc (size);
  char *read_buf = (char *) malloc (size);
  if (!write_buf || !read_buf)
    {
      printf ("Error allocating buffers\n");
      system ("PAUSE");
      exit (0);
    }


  for (i = 0; i < size; i++)
    {
      write_buf[i] = 0x0FF & rand ();
      //write_buf[i] = 0;
      read_buf[i] = 0;
    }

  printf ("Starting write\n");

  write_start = timeGetTime ();
  for (i = 0; i < reps; i++)
    {
      usb_write_ram (BANK_DATARAM, 0, size, write_buf);
    }
  write_end = timeGetTime ();

  printf ("Write completed. Starting read\n");

  read_start = timeGetTime ();
  for (i = 0; i < reps; i++)
    {
      usb_read_ram (BANK_DATARAM, 0, size, read_buf);
    }
  read_end = timeGetTime ();

  // check and make sure that we read back exactly the same data that we wrote
  for (i = 0; i < size; i++)
    {
      if (write_buf[i] != read_buf[i])
	{
	  //printf("xfer error (%d): wrote 0x%x, read 0x%x\n", i, write_buf[i], read_buf[i]);
	  err = 1;
	}
    }

  // This prints out the first few bytes of what we wrote, and why
  printf ("We wrote this (only first 64 bytes shown):\n");
  print_data (64, write_buf);
  printf ("And read back this (only first 64 bytes shown):\n");
  print_data (64, read_buf);


  printf ("Write time was %dms. Transfer size was %d bytes. %f MBit/sec\n",
	  write_end - write_start, size * reps,
	  (float) (size * reps * 8) / ((float) (write_end - write_start) *
				       1e3));
  printf ("Read time was %dms. Transfer size was %d bytes. %f MBit/sec\n",
	  read_end - read_start, size * reps,
	  (float) (size * reps * 8) / ((float) (read_end - read_start) *
				       1e3));

  printf ("\n\n");
  if (!err)
    {
      printf ("============================\n");
      printf ("RAM transfer test succesfull\n");
      printf ("============================\n");
    }
  else
    {
      printf ("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      printf ("RAM transfer NOT succesfull\n");
      printf ("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    }

  free (read_buf);
  free (write_buf);


  return err;
}

void
print_data (int size, char *data_buf)
{
  int col, i;
  i = 0;
  while (i < size)
    {
      for (col = 0; col < 8; col++)
	{
	  printf ("0x%.2x,", 0x0FF & data_buf[i]);
	  i++;
	  if (i >= size)
	    {
	      goto done;
	    }
	}
      printf ("\n");
    }
done:
  printf ("\n\n");
}

// temp version, since rest of spinapi isnt linked in
//unsigned int reg_read(unsigned int addr)
//{
//    unsigned int data;
//    usb_read_reg(addr, &data);
//    return data; 
//    
//}
