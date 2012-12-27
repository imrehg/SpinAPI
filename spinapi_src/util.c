/* util.c
 * Miscellaneous support functions.
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

#include "spinapi.h"
#include "driver-os.h"
#include "util.h"
#include "caps.h"

// setting this to 1 causes the debug() function to print out lots of debugging
// info
int do_debug = 0;

#ifndef BOOL
#define BOOL unsigned char
#endif

#ifndef true
#define true -1
#endif

#ifndef false
#define false 0
#endif

SPINCORE_API void
pb_set_debug (int debug)
{
  do_debug = debug;
}

/**
 * Write a byte to a board using the AMCC chip
 *
 * \return Negative number on failure and set spinerr, else 0 on success.
 */

int
do_amcc_outp (int card_num, unsigned int address, char data)
{
  unsigned int MAX_RECV_TIMEOUT = 1000;
  unsigned int RECV_START, RECV_TOGGLE, RECV_TIMEOUT = 0;
  int XFER_ERROR = 0;
  int RECV_POLLING = 0;

  unsigned int OGMB = 0x0C;
  unsigned int CHK_RECV = 0x1F;
  unsigned int SIG_TRNS = 0x0F;

  unsigned int CLEAR31 = 0x00000001;
  unsigned int CLEAR24 = 0x000000FF;
  unsigned int CLEAR28 = 0x0000000F;
  unsigned int SET_XFER = 0x01000000;

  unsigned int Temp_Address = address;
  unsigned int Temp_Data = data;

  // Prepare Address Transfer
  Temp_Address &= CLEAR28;
  Temp_Address |= SET_XFER;

  // Prepare Data Transfer
  Temp_Data &= CLEAR24;
  Temp_Data |= SET_XFER;

  // Clear the XFER bit (Should already be cleared)
  os_outp (card_num, SIG_TRNS, 0);

  // Transfer Address

  // Read RECV bit from the Board
  RECV_START = os_inp (card_num, CHK_RECV);
  RECV_START &= CLEAR31;	// Only Save LSB


  // Send Address to OGMB
  os_outw (card_num, OGMB, Temp_Address);

  RECV_POLLING = 1;		// Set Polling Flag
  RECV_TIMEOUT = 0;
  while ((RECV_POLLING == 1) && (RECV_TIMEOUT < MAX_RECV_TIMEOUT))
    {
      // Check for Toggled RECV
      RECV_TOGGLE = os_inp (card_num, CHK_RECV);

      RECV_TOGGLE &= CLEAR31;	// Only Save LSB
      if (RECV_TOGGLE != RECV_START)
	RECV_POLLING = 0;	// Finished if Different
      else
	RECV_TIMEOUT++;
      if (RECV_TIMEOUT == MAX_RECV_TIMEOUT)
	{
	  XFER_ERROR = -2;
	  spinerr = "timeout reached while sending address";
	  debug ("do_amcc_outp: %s\n", spinerr);
	}
    }

  // Transfer Complete (Clear) Signal
  os_outp (card_num, SIG_TRNS, 0);

  // Transfer Data

  // Read RECV bit from the Board
  RECV_START = os_inp (card_num, CHK_RECV);
  RECV_START &= CLEAR31;	// Only Save LSB

  // Send Data to OGMB
  os_outw (card_num, OGMB, Temp_Data);

  RECV_POLLING = 1;		// Set Polling Flag
  RECV_TIMEOUT = 0;
  while ((RECV_POLLING == 1) && (RECV_TIMEOUT < MAX_RECV_TIMEOUT))
    {
      // Check for Toggled RECV
      RECV_TOGGLE = os_inp (card_num, CHK_RECV);
      RECV_TOGGLE &= CLEAR31;	// Only Save LSB

      if (RECV_TOGGLE != RECV_START)
	RECV_POLLING = 0;	// Finished if Different
      else
	RECV_TIMEOUT++;
      if (RECV_TIMEOUT == MAX_RECV_TIMEOUT)
	{
	  XFER_ERROR = -2;
	  spinerr = "timeout reached while sending data";
	  debug ("do_amcc_outp: %s\n", spinerr);
	}

    }

  // Transfer Complete (Clear) Signal
  os_outp (card_num, SIG_TRNS, 0);

  return XFER_ERROR;
}

// PB02PC boards (which have device id 0x5920) use this method of transferring
// data to the amcc chip
int
do_amcc_outp_old (int card_num, unsigned int address, int data)
{
  unsigned int OGMB = 0x0C;
  unsigned int ICMB = 0x1C;

  int byte[4];
  int error_counter = 0;

  int expected[5];
  expected[0] = 7;
  expected[1] = 0;
  expected[2] = 1;
  expected[3] = 2;
  expected[4] = 7;

  BOOL NotReady = true;
  BOOL NotFinished = true;
  int nibble_counter = 0;
  int temp_data;


  byte[0] = 0x30;
  byte[1] = 0x00 | (address & 0x0F);
  byte[2] = 0x10 | (data >> 4 & 0x0F);
  byte[3] = 0x20 | (data & 0x0F);

  debug ("Writing %x to addr %x\n", data, address);

  while (NotReady)
    {
      // Read COUNTER byte from the Board
      temp_data = os_inp (card_num, ICMB);

      temp_data &= 0x07;
      if (temp_data != expected[nibble_counter])
	{
	  error_counter++;

	  // Attempt to Advance Counter to Idle State
	  os_outp (card_num, OGMB, temp_data << 4);
	}
      else
	{
	  NotReady = false;
	}
      if (error_counter > 10)
	{
	  return -3;
	}
    }

  NotReady = true;
  error_counter = 0;
  
  while (NotFinished)
    {
      // Write Data Byte
      os_outp (card_num, OGMB, byte[nibble_counter]);

      nibble_counter++;
      while (NotReady)
	{
	  // Check Counter State
	  temp_data = os_inp (card_num, ICMB);

	  temp_data &= 0x07;
	  if (temp_data != expected[nibble_counter])
	    {
	      error_counter++;
	    }
	  else
	    {
	      error_counter = 0;
	      NotReady = false;
	    }
	  if (error_counter > 100)	// timeout
	    {
	      return -6;
	    }
	}
      NotReady = true;

      if (nibble_counter == 4)
	{
	  NotFinished = false;
	}
    }

  return 0;
}

/**
 * Read a byte from a board using the AMCC chip
 *
 *
 * \return The byte requested.
 */

char
do_amcc_inp (int card_num, unsigned int address)
{
  unsigned int MAX_RECV_TIMEOUT = 10000;
  unsigned int RECV_START, RECV_STOP, RECV_TOGGLE, RECV_TIMEOUT = 0;
  int XFER_ERROR = 0;
  int RECV_POLLING = 0;

  //unsigned int OGMB = 0x0C;
  unsigned int CHK_RECV = 0x1F;
  unsigned int SIG_TRNS = 0x0F;
  unsigned int ICMB = 0x1C;

  //unsigned int CLEAR31 = 0x00000001;
  unsigned int CLEAR24 = 0x000000FF;
  //unsigned int CLEAR28 = 0x0000000F;

  unsigned int BIT1 = 0x00000002;
  unsigned int INV1 = 0x000000FD;

  //unsigned int SET_XFER = 0x01000000;
  //unsigned int SET_XFER = 0x01;
  unsigned short READ_ADDR = 0x09;

  int Toggle = 0;
  int Toggle_Temp = 0;

  //unsigned int Temp_Address = address;
  unsigned int Temp_Data = 0;


  // CHEK FOR 1 in MD1


  //pci_outp(SET_ADDR_REG, address); // Set address for incoming data
  //pci_outp(2,1);
  do_amcc_outp (card_num, 8, address);
  //pci_outp(4,0);
  do_amcc_outp (card_num, READ_ADDR, 0);	// Tell board to start a read cycle

  RECV_POLLING = 1;		// Set Polling Flag
  RECV_TIMEOUT = 0;
  RECV_START = 0x02;		// Value expected when data is ready

  while ((RECV_POLLING == 1) && (RECV_TIMEOUT < MAX_RECV_TIMEOUT))
    {
      // Check for Toggled RECV
      //RECV_TOGGLE = _inp(CHK_RECV);
      RECV_TOGGLE = os_inp (card_num, CHK_RECV);
      RECV_TOGGLE &= BIT1;	// Only Save Bit 1

      if (RECV_TOGGLE == RECV_START)
	RECV_POLLING = 0;	// Finished if Different
      else
	RECV_TIMEOUT++;
      if (RECV_TIMEOUT == MAX_RECV_TIMEOUT)
	{
	  XFER_ERROR = -2;
	  spinerr = "timeout reached while sending address";
	  debug ("%s\n", spinerr);
	}


    }
  if (XFER_ERROR != 0)
    {
      return XFER_ERROR;
    }

  //Temp_Data = _inp(ICMB);
  // Read RECV bit from the Board
  Temp_Data = os_inp (card_num, ICMB);
  Temp_Data &= CLEAR24;


  //Toggle = _inp(SIG_TRNS);
  Toggle = os_inp (card_num, SIG_TRNS);

  Toggle_Temp = Toggle & BIT1;	// Only Save Bit 1
  if (Toggle_Temp == 0x0)
    {
      Toggle |= BIT1;		// If Bit 1 is zero, set it to 1
    }
  else
    {
      Toggle &= INV1;		// IF Bit 1 is 1, set it to 0
    }

  //_outp(SIG_TRNS, Toggle);
  os_outp (card_num, SIG_TRNS, Toggle);

  RECV_POLLING = 1;		// Set Polling Flag
  RECV_TIMEOUT = 0;
  RECV_STOP = 0x0;

  while ((RECV_POLLING == 1) && (RECV_TIMEOUT < MAX_RECV_TIMEOUT))
    {
      // Check for Toggled RECV
      //RECV_TOGGLE = _inp(CHK_RECV);
      RECV_TOGGLE = os_inp (card_num, CHK_RECV);
      RECV_TOGGLE &= BIT1;	// Only Save Bit 1

      if (RECV_TOGGLE == RECV_STOP)
	RECV_POLLING = 0;	// Finished if Different
      else
	RECV_TIMEOUT++;
      if (RECV_TIMEOUT == MAX_RECV_TIMEOUT)
	{
	  XFER_ERROR = -3;
	  spinerr = "timeout reached while getting data";
	  debug ("%s\n", spinerr);
	}
    }
  if (XFER_ERROR != 0)
    {
      return XFER_ERROR;
    }

  if (XFER_ERROR == 0)
    {
      return Temp_Data;
    }
  else
    {
      //printf("Errored data is %x\n",Temp_Data);
      //printf("RECV_TOGGLE = %i\n",RECV_TOGGLE);
      return XFER_ERROR;
    }
}


/**
 * Return a string which is of the form:<br>
 * a: b
 *
 */
char *
my_strcat (char *a, char *b)
{
  char *string;

  string = malloc (sizeof (char) * (strlen (a) + strlen (b) + 5));

  strcpy (string, a);
  strcat (string, ": ");
  strcat (string, b);

  return string;
}

char *
my_sprintf (char *format, ...)
{
  va_list ap;
  char *string;

  string = (char *) malloc (1024 * sizeof (char));

  va_start (ap, format);
  vsprintf (string, format, ap);
  va_end (ap);


  return string;
}


/**
 *
 */
void
_debug (const char* function, char *format, ...)
{
  struct tm *timeinfo;
  
  static FILE* fp[32]; /*This is essentially the max number of boards, which isn't defined at an easy access point in the API at this time..*/
  char fname_buf[32];
  char ftime[10];
  
  va_list ap;
  time_t t;
  
  extern int cur_board;

  /*Check to see if a file handle already exists for the current board.*/
  if(fp[cur_board] == NULL) {
    t = time (0); /*Get the time the log file was created*/
	
    snprintf(fname_buf, sizeof(fname_buf), "log_board%d.txt", cur_board);
	fp[cur_board] = fopen(fname_buf,"w");
	
    if(fp[cur_board] == NULL) {
	  printf("WARNING: Couldn't open %s (%s). Using stdout instead\n",fname_buf,strerror (errno));
	  fp[cur_board] = stdout;
    }
    else {
	  fprintf (fp[cur_board], "Logfile created: %s\n", asctime (localtime (&t)));
      fprintf (fp[cur_board], "SpinAPI version: %s\n", pb_get_version ());
	  fprintf (fp[cur_board], "Current Board #: %d\n", cur_board);
      fprintf (fp[cur_board], "================================================================================================\n");
	  fprintf (fp[cur_board], "%-8s %-25s %-10s\n", "TIME", "FUNCTION", "MESSAGE");
      fprintf (fp[cur_board], "================================================================================================\n");		
    }
   }
	
  time (&t);
  timeinfo = localtime (&t);
  strftime (ftime, 10, "%H:%M:%S", timeinfo);
  
  va_start (ap, format);
  fprintf (fp[cur_board], "%s %-25s", ftime, function);
  vfprintf (fp[cur_board], format, ap);
  fprintf (fp[cur_board], "\n");
  va_end (ap);

  fflush (fp[cur_board]);
}
