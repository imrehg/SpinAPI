/* spinapi.c
 * Main module for the spinapi libary. This module contains all the user-visible
 * functions that make up the API.
 *
 * $Date: 2009/09/18 18:29:51 $
 *
 * To get the latest version of this code, or to contact us for support, please
 * visit http://www.spincore.com
 */

/* Copyright (c) 2009 SpinCore Technologies, Inc.
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
#include <math.h>
#include <string.h>
//#include <windows.h>
#include <unistd.h>         
#include "FTD2XX/FTD2XX.h"  

#include "spinapi.h"
#include "driver-os.h"
#include "driver-usb.h"
#include "util.h"
#include "caps.h"
#include "if.h"
#include "usb.h"

/*
*
*
*  SpinPTS Includes & Defines
*
*
*/
#define _PTS_SET_NUM_DATA	3
#define ERROR_STR_SIZE	    25

static char *spinpts_version = "20110913";//VER_STRING_PTS;
static int  spinpts_err = -1;
static char spinpts_err_buf[ERROR_STR_SIZE];

SPINCORE_API int __set_pts_(int pts_index, BCDFREQ freq, int phase );
SPINCORE_API BCDFREQ* freq_double_to_bcd( double freq, BCDFREQ* bcdfreq, int is3200 );
/*
*
*
*  End of SpinPTS Includes & Defines
*
*
*/

//Need this char array to display status message
char status_message[120];

// Contains current number of instructions in pulse program
int num_instructions = 0;
extern int shape_period_array[2][7];

//default portbase supplied to backwards compatibility
static int port_base = 0;

//default is pb_only with 24 output bits
static int ISA_BOARD = 0;

// FIXME: this should go in board_info
static int MAX_FLAG_BITS = 0xFFFFFF;

//2^32
double pow232 = 4294967296.0;

double last_rounded_value;

// The number of the currently selected board.
int cur_board = 0;
// Number of boards present in system. -1 indicates we havent counted them yet
//static int num_boards = -1;
static int num_pci_boards = -1;
static int num_usb_devices = -1;
int cur_dds = 0;

// This array holds the capabilties info on each board
BOARD_INFO board[MAX_NUM_BOARDS];

static int cur_device = -1;
static int cur_device_addr = 0;

/** \internal
 * This is set to a description string whenever an error occurs inside a function */
char *spinerr;
/** \internal
 * spinerr is set to this whenever an error has NOT occurred */
char *noerr = "No Error";

//version of the DLL
char *version = "20110913";//VER_STRING_API;


int do_os_init (int board);
int do_os_close (int board);

/**
 * \mainpage SpinAPI Documentation
 *
 * This document describes the API used to control SpinCore Technologies, Inc.
 * products and is intended to serve primarily as a reference document. Please
 * see the manuals and example programs to see how the API is used in practice
 * to control the boards.
 *
 * \see spinapi.h for a complete listing of SpinAPI functions.
 * \see http://www.spincore.com/support to download the latest version of SpinAPI
 * \see http://www.spincore.com for information on our products.
 */

/**
 *\file spinapi.c
 *\brief General functions for use with all boards.
 *
 * This page describes functions which are used with all products. It also includes
 * several functions for use with DDS enabled boards that are not relevant to digital-only
 * PulseBlaster cards. For a complete
 * list of spinapi functions, including those which are used only with RadioProcessor
 * boards, please see the spinapi.h page.
 *
 */

SPINCORE_API int
pb_count_boards (void)
{
  spinerr = noerr;


  num_pci_boards = os_count_boards (VENDID);

  if (num_pci_boards < 0)
    {
      debug
	("pb_count_boards(): error counting PCI boards. Please check to make sure WinDriver is properly installed.\n");
      num_pci_boards = 0;
    }

  num_usb_devices = os_usb_count_devices (0);

  if (num_usb_devices < 0)
    {
      debug ("pb_count_boards(): error counting USB boards.\n");
      num_usb_devices = 0;
    }

  if (num_pci_boards + num_usb_devices > MAX_NUM_BOARDS)
    {
      spinerr = "Detected more boards than the driver can handle";
      return -1;
    }

  debug ("pb_count_boards(): Detected %d boards in your system.\n",
	 num_pci_boards + num_usb_devices);

  return num_pci_boards + num_usb_devices;
}

SPINCORE_API int
pb_select_board (int board_num)
{
  spinerr = noerr;

  int num_boards = num_pci_boards + num_usb_devices;

  if (num_boards < 0)
    {
      num_boards = pb_count_boards ();
      if (num_boards < 0)
	{
	  //spinerr should already be set by pb_count_boards
	  debug ("pb_select_board: %s\n", spinerr);
	  return -1;
	}
    }

  if (board_num < 0 || board_num >= num_boards)
    {
      spinerr = "Board number out of range";
      debug ("pb_select_board(..): %s (num_boards=%d)\n", spinerr,
	     num_boards);
      return -1;
    }

  if (board_num >= num_pci_boards)	//Set current USB Device
    {
      debug ("pb_select_board(..): Selecting usb board: %d",
	     board_num - num_pci_boards);
      usb_set_device (board_num - num_pci_boards);
    }

  cur_board = board_num;

  return 0;
}

SPINCORE_API int
pb_init (void)
{
  spinerr = noerr;

  int dac_control;
  int adc_control;

  int dev_id;

  debug ("Entering pb_init. cur_board = %d\n", cur_board);

  if (board[cur_board].did_init == 1)
    {
      spinerr = "Board already initialized. Only call pb_init() once.";
      debug ("pb_init: %s\n", spinerr);
      return -1;
    }

  if (pb_count_boards () > 0)
    {
      dev_id = do_os_init (cur_board);

      if (dev_id == -1)
	{
	  // spinerr should have been set inside the os_init
	  debug ("pb_init error (os_init failed): %s\n", spinerr);
	  return -1;
	}

      // Figure out what the capabilities of this board is
      if (get_caps (&(board[cur_board]), dev_id) < 0)
	{
	  // spinerr should have been set inside get_caps
	  debug ("pb_init error (get_caps failed): %s\n", spinerr);
	  return -1;
	}

      // If this is a RadioProcessor, set ADC and DAC defaults. This is done
      // here instead of pb_set_defaults() because the user should not ever
      // change these values.
      if (board[cur_board].is_radioprocessor)
	{
	  adc_control = 3;
	  dac_control = 0;
	  pb_set_radio_hw (adc_control, dac_control);
	}

      board[cur_board].did_init = 1;
    }
  else
    {
      spinerr = "No PulseBlaster Board found!";                           
      debug ("pb_init(): No board selected.\n");
      return -1;
    }

  return 0;
}

SPINCORE_API void
pb_core_clock (double clock_freq)
{
  spinerr = noerr;

  board[cur_board].clock = clock_freq / 1000.0;	// Put in GHz (for ns timescale)
}

SPINCORE_API void
pb_set_clock (double clock_freq)
{
	pb_core_clock(clock_freq);
}

SPINCORE_API int
pb_close (void)
{
  spinerr = noerr;

  if (board[cur_board].did_init == 0)
    {
      spinerr = "Board is already closed";
      debug ("pb_close: %s\n", spinerr);
      return -1;
    }

  board[cur_board].did_init = 0;
  return do_os_close (cur_board);
}

SPINCORE_API int
pb_start_programming (int device)
{
  spinerr = noerr;

  int return_value;

  debug ("pb_start_programming (device=%d)\n", device);

  // pb_stop_programming() didnt get called. some older code doesnt call
  // pb_stop_programming() properly, so ignore this error and continue anyway
  if (cur_device != -1)
    {
      debug
	("pb_start_programming: WARNING: pb_start_programming() called without previous stop\n",
	 spinerr);
    }

  if (board[cur_board].usb_method == 2)
    {
      debug ("pb_start_programming: Using new programming method.\n", device);

      if (device == PULSE_PROGRAM)
	{
	  num_instructions = 0;	// Clear number of instructions  
	  usb_write_address (0x80000);	//Write the address register with the start of the PB core memory.
	}

      if (device == FREQ_REGS)
	{
	  usb_write_address (board[cur_board].dds_address[cur_dds] + 0x0000);
	}
      if (device == TX_PHASE_REGS)
	{
	  if(board[cur_board].firmware_id == 0x0C13 || board[cur_board].firmware_id == 0x0E03) //These designs have different Register Base Addresses
	  {
        usb_write_address (board[cur_board].dds_address[cur_dds] + 0x2000);
      }
	  else{
	    usb_write_address (board[cur_board].dds_address[cur_dds] + 0x0400);
	  }
	}
  }
  else
    {
      // Dont reset PulseBlaster if this is a new style DDS program
      if (board[cur_board].dds_prog_method == DDS_PROG_OLDPB
	  || device == PULSE_PROGRAM)
	{

	  debug ("pb_start_programming: reset\n");

	  return_value = pb_outp (port_base + 0, 0);	// Reset PulseBlasterDDS
	  if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = my_strcat ("initial reset failed", spinerr);
	      debug ("pb_start_programming: %s\n", spinerr);
	      return return_value;
	    }
	}

      if (device == PULSE_PROGRAM)
	{
	  num_instructions = 0;	// Clear number of instructions

	  if (board[cur_board].firmware_id == 0xa13 || board[cur_board].firmware_id == 0xC10)	//Fix me.
	    {
	      //For 0x0a13, the IMW size is 88 bits. (11x8=88)
	      return_value = pb_outp (port_base + 2, 11);	// This is an instruction, therefore Bytes Per Word (BPW) = 11
	    }
      else if(board[cur_board].firmware_id == 0x0908)	//Fix me.
	    {
	      //For 0x0908, the IMW size is 64 bits. (8x8=64)
	      return_value = pb_outp (port_base + 2, 8);	// This is an instruction, therefore Bytes Per Word (BPW) = 8
	    }
      else if(board[cur_board].firmware_id == 0x1105 || board[cur_board].firmware_id == 0x1106 || board[cur_board].firmware_id == 0x1107)	//Fix me.
	    {
	      //For 0x1105, the IMW size is 32 bits. (4x8=64)
	      return_value = pb_outp (port_base + 2, 4);	// This is an instruction, therefore Bytes Per Word (BPW) = 4
	    }
	  else
	    {
	      return_value = pb_outp (port_base + 2, 10);	// This is an instruction, therefore Bytes Per Word (BPW) = 10
	    }

	  if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = my_strcat ("BPW=10 write failed", spinerr);
	      debug ("pb_start_programming: %s\n", spinerr);
	      return return_value;
	    }

	  return_value = pb_outp (port_base + 3, device);	// Device = RAM
	  if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = my_strcat ("Device=RAM write failed", spinerr);
	      debug ("pb_start_programming: %s\n", spinerr);
	      return return_value;
	    }

	  return_value = pb_outp (port_base + 4, 0);	// Reset mem counter
	  if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr =
		my_strcat ("mem counter write failed (PULSE_PROGRAM)",
			   spinerr);
	      debug ("pb_start_programming: %s\n", spinerr);
	      return return_value;
	    }

	}
      else if (device == FREQ_REGS || device == TX_PHASE_REGS
	       || device == RX_PHASE_REGS)
	{
	  // We only need to do anything here if this the old style dds interface
	  if (board[cur_board].dds_prog_method == DDS_PROG_OLDPB)
	    {

	      return_value = pb_outp (port_base + 2, 4);	// This is a DDS reg, therefore BPW = 4
	      if (return_value != 0 && (!(ISA_BOARD)))
		{
		  spinerr = my_strcat ("BPW=4 write failed", spinerr);
		  return return_value;
		}

	      return_value = pb_outp (port_base + 3, device);	// Device = FREQ_REGS, TX_PHASE_REGS, or RX_PHASE_REGS
	      if (return_value != 0 && (!(ISA_BOARD)))
		{
		  spinerr =
		    my_strcat ("Device=XXX_REGS write failed", spinerr);
		  return return_value;
		}

	      return_value = pb_outp (port_base + 4, 0);	// Reset mem counter
	      if (return_value != 0 && (!(ISA_BOARD)))
		{
		  spinerr =
		    my_strcat ("mem counter write failed (REGS)", spinerr);
		  return return_value;
		}
	    }
	}
    }
  cur_device = device;
  cur_device_addr = 0;

  return 0;
}

SPINCORE_API int
pb_stop_programming (void)
{
  spinerr = noerr;

  int return_value;

  debug ("pb_stop_programming: (device=%d)\n", cur_device);

  if (board[cur_board].usb_method != 2)
  {
      return_value = pb_outp (port_base + 7, 0);
      if (return_value != 0 && (!(ISA_BOARD)))
	  {
	    // spinerr will be set by pb_outp
	    return return_value;
	  }
  }
  else
  {    
	  if(cur_device == PULSE_PROGRAM)
	  {
	    if(board[cur_board].firmware_id == 0x0C13)
		{
		  debug("pb_stop_programming(PULSE_PROGRAM): Writing shape period information to DDS-I board\n");
	      usb_write_address(board[cur_board].dds_address[0] + 0x6000);
		  usb_write_data(shape_period_array[0], 7);
		}
		else if(board[cur_board].firmware_id == 0x0E03)
		{
		  debug("pb_stop_programming(PULSE_PROGRAM): Writing shape period information to DDS-II board\n");
	      usb_write_address(board[cur_board].dds_address[0] + 0x6000);
		  usb_write_data(shape_period_array[0], 7);
		  usb_write_address(board[cur_board].dds_address[1] + 0x6000);
		  usb_write_data(shape_period_array[1], 7);
		}
		else{
	      debug("pb_stop_programming(PULSE_PROGRAM): Writing shape period information to DDS-II board\n");
	      usb_write_address(board[cur_board].dds_address[0] + 0x0C00);
		  usb_write_data(shape_period_array[0], 7);
		  usb_write_address(board[cur_board].dds_address[1] + 0x0C00);
		  usb_write_data(shape_period_array[1], 7);
		}
	  }
      usb_write_address (0);
  }

  cur_device = -1;
  cur_device_addr = 0;

  return 0;
}

SPINCORE_API int
pb_start (void)
{
  spinerr = noerr;
  debug("pb_start():");
 
  if (board[cur_board].usb_method == 2)
    {
      int start_flag = 0x01;
      usb_write_address (board[cur_board].pb_base_address + 0x00);
      usb_write_data (&start_flag, 1);
      return 0;
    }

  int return_value;

  return_value = pb_outp (port_base + 1, 0);
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      spinerr = my_strcat ("+1 write failed", spinerr);
      return return_value;
    }

  return 0;
}

SPINCORE_API int
pb_stop (void)
{
  spinerr = noerr;
  debug("pb_stop():");
  
  if (board[cur_board].usb_method == 2)
    {
      int stop_flag = 0x02;
      usb_write_address (board[cur_board].pb_base_address + 0x00);
      usb_write_data (&stop_flag, 1);
      return 0;
    }
	
  int return_value;
  return_value = pb_outp (port_base + 0, 0);
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      // spinerr will be set by pb_outp
      return return_value;
    }
	
   return_value = pb_outp (port_base + 2, 0xFF);
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      spinerr = my_strcat ("+2 write failed", spinerr);
      return return_value;
    }

  return_value = pb_outp (port_base + 3, 0xFF);
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      spinerr = my_strcat ("+3 write failed", spinerr);
      return return_value;
    }

  return_value = pb_outp (port_base + 4, 0xFF);
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      spinerr = my_strcat ("+4 write failed", spinerr);
      return return_value;
    }

  return_value = pb_outp (port_base + 7, 0);
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      spinerr = my_strcat ("+7 write failed", spinerr);
      return return_value;
    }
   
   
  return 0;
}

SPINCORE_API int
pb_reset (void)
{
   spinerr = noerr;
   debug("pb_reset():"); 
   if (board[cur_board].usb_method == 2)
   {
      /* Equivalent to pb_stop() for PBDDS-II Boards */
      int stop_flag = 0x02;
      usb_write_address (board[cur_board].pb_base_address + 0x00);
      usb_write_data (&stop_flag, 1);
      return 0;
      return 0;
   }
	  
  int return_value;
  return_value = pb_outp (port_base + 0, 0); /* Software Reset */
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      spinerr = my_strcat ("+0 write failed", spinerr);
      return return_value;
    }

  return_value = pb_outp (port_base + 2, 0xFF); /* Latch # Bytes per Instruction Memory Word */
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      spinerr = my_strcat ("+2 write failed", spinerr);
      return return_value;
    }

  return_value = pb_outp (port_base + 3, 0xFF); /* Latch Memory Device Number */
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      spinerr = my_strcat ("+3 write failed", spinerr);
      return return_value;
    }

  return_value = pb_outp (port_base + 4, 0xFF); /* Reset the Address Counter */
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      spinerr = my_strcat ("+4 write failed", spinerr);
      return return_value;
    }

  return_value = pb_outp (port_base + 7, 0); /* Signal Programming Finished */
  if (return_value != 0 && (!(ISA_BOARD)))
    {
      spinerr = my_strcat ("+7 write failed", spinerr);
      return return_value;
    }
	
   return 0;
	
}

SPINCORE_API int
pb_inst_tworf (int freq, int tx_phase, int tx_output_enable, int rx_phase,
	       int rx_output_enable, int flags, int inst, int inst_data,
	       double length)
{
  spinerr = noerr;

  unsigned int flag_word;

  // 
  // FIXME: this should check against the values on the BOARD_INFO structure instead
  // of predefined values
  //

  //Check for valid passed parameters
  if (freq < 0 || freq >= board[cur_board].num_freq0)
    {
      spinerr = "Frequency register out of range";
      debug ("pb_inst_tworf: %s\n");
      return -99;
    }

  if (tx_phase < 0 || tx_phase >= board[cur_board].num_phase2)
    {
      spinerr = "TX phase register out of range";
      debug ("pb_inst_tworf: %s\n");
      return -98;
    }
  if (tx_output_enable != ANALOG_OFF)
    {
      tx_output_enable = ANALOG_ON;
    }
  if (rx_phase < 0 || rx_phase >= board[cur_board].num_phase2)
    {
      spinerr = "RX phase register out of range";
      debug ("pb_inst_tworf: %s\n");
      return -96;
    }
  if (rx_output_enable != ANALOG_OFF)
    {
      rx_output_enable = ANALOG_ON;
    }
  //NOT NEEDED - should be warning not error and max_flag_bots shouldnt be constant
  /*if(flags < 0 || flags > MAX_FLAG_BITS)
     {
     spinerr = "Too many flag bits";
     debug("pb_inst_tworf: %s\n");
     return -94;
     } */


  /* MOVED TO PB_INST_RADIO_SHAPE()
     if(board[curr_board].custom_design == 3)
     {   //firmware revision 10-15, PBDDS-300. RP board in which aquisition has been disabled and there are 9 TTL outputs
     unsigned int flags8to5 = (flags & 0x1E0) >> 5;
     unsigned int flag4     = (flags & 0x010) >> 4;
     unsigned int flags3to0 = (flags & 0x00F);

     flag_word |= (0x0F & flags8to5)    << 20;  // 4 bits 
     flag_word |= (0x0F & tx_phase)     << 16;  // 4 bits
     flag_word |= (0x01 & tx_enable)    << 15;  // 1 bit
     flag_word |= (0x0F & freq)         << 11;  // 4 bits
     flag_word |= (0x01 & flag4)        << 10;  // 1 bit
     flag_word |= (0x01 & phase_reset)  << 9;   // 1 bit
     flag_word |= (0x07 & shape_period) << 6;   // 3 bits
     flag_word |= (0x03 & amp)          << 4;   // 2 bits
     flag_word |= (0x0F & flags3to0)            // 4 bits

     debug("pb_inst_tworf: using PBDDS-300 flag_word partitioning scheme\n");    
     }
     else
     { */
  flag_word = (freq << 20) |
    (tx_phase << 16) |
    (!(tx_output_enable) << 11) |
    (rx_phase << 12) | (!(rx_output_enable) << 10) | (flags & MAX_FLAG_BITS);

  debug ("pb_inst_tworf: using standard PB flag_word partitioning scheme\n");

  if (board[cur_board].custom_design != 0)
    debug
      ("WARNING 001: You are using the wrong instruction(i.e. pb_inst_onerf, pb_inst_radio, etc), Please refer to your manual.\nContact www.SpinCore.com for help.\n");
  //}

  return pb_inst_pbonly (flag_word, inst, inst_data, length);
}

SPINCORE_API int
pb_inst_onerf (int freq, int phase, int rf_output_enable, int flags, int inst,
	       int inst_data, double length)
{
  spinerr = noerr;

  return pb_inst_tworf (freq, phase, rf_output_enable, 0, 1, flags, inst,
			inst_data, length);
}

SPINCORE_API int 
pb_4C_inst(int flag, double length)
{    
    debug("Firmware ID: 0x%x\n", board[cur_board].firmware_id);  
      
    if(board[cur_board].firmware_id == 0x1105 || board[cur_board].firmware_id == 0x1106 || board[cur_board].firmware_id == 0x1107)
    {
        int temp = 0;
        int return_value;
        double pb_clock = board[cur_board].clock * board[cur_board].pb_clock_mult;

        unsigned int delay = (int) rint ((length * pb_clock) - 1.0);	//(Assumes clock in GHz and length in ns)
        
        if(delay > 0x3FFFFFFF || delay < 2)
        {
             spinerr = "Instruction delay will not work with your board";
             debug ("pb_4C_inst: %s\n", spinerr);
             return -91;
        }

        //writing instruction to memory in this format:
        //  |  31 | 30 | 29 .. 0 |
        //  |Flag | Op-Code  | Delay   |
        // write 8-bits at a time
		temp = 0;
        temp += (flag << 7); // put flag bit in most significant location of byte     
        temp += ((0x3F000000 & delay) >> 24);       // put 3 MSBits of the delay in correct location of byte
        return_value = pb_outp(port_base + 6, temp);

        if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = "Communications error (loop 1)";
	      debug ("pb_4C_inst: %s\n", spinerr);
	      debug ("return value was: %d\n", return_value);
	      return return_value;
	    }
	    
        temp = 0;
        temp = ((0x00FF0000 & delay) >> 16);
        return_value = pb_outp(port_base + 6, temp);
        if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = "Communications error (loop 2)";
	      debug ("pb_4C_inst: %s\n", spinerr);
	      debug ("return value was: %d\n", return_value);
	      return return_value;
	    }
		
	    temp = 0;
        temp = ((0x0000FF00 & delay) >> 8);
        return_value = pb_outp(port_base + 6, temp);
        if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = "Communications error (loop 3)";
	      debug ("pb_4C_inst: %s\n", spinerr);
	      debug ("return value was: %d\n", return_value);
	      return return_value;
	    }
		
	    temp = 0;
        temp = (0x000000FF & delay);
        return_value = pb_outp(port_base + 6, temp);
        if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = "Communications error (loop 4)";
	      debug ("pb_4C_inst: %s\n", spinerr);
	      debug ("return value was: %d\n", return_value);
	      return return_value;
	    }
		
		
    }
    else
        return pb_inst_pbonly(flag, CONTINUE, 0, length);  

	return 0;
}

SPINCORE_API int
pb_4C_stop(void)
{
    if(board[cur_board].firmware_id == 0x1105 || board[cur_board].firmware_id == 0x1106 || board[cur_board].firmware_id == 0x1107)
    {
        int temp = 0;
        int return_value;
        double pb_clock = board[cur_board].clock * board[cur_board].pb_clock_mult;

        unsigned int delay = (int) rint ((30.0*ns * pb_clock) - 1.0);	//(Assumes clock in GHz and length in ns)
        if(delay > 0x3FFFFFFF || delay < 2)
        {
             spinerr = "Instruction delay will not work with your board";
             debug ("pb_4C_inst: %s\n", spinerr);
             return -91;
        }

        //writing instruction to memory in this format:
        //  |  31 | 30  | 29 .. 0 |
        //  |Flag | Op-Code  | Delay   |
        // write 8-bits at a time    
		
		temp = 0;
        temp += (1 << 6);  // put opcode in correct location
        temp += ((0x3F000000 & delay) >> 24);       // put MSBits of the delay in correct location of byte
        return_value = pb_outp(port_base + 6, temp);
        if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = "Communications error (loop 1)";
	      debug ("pb_4C_stop: %s\n", spinerr);
	      debug ("return value was: %d\n", return_value);
	      return return_value;
	    }
	    
        temp = 0;
        temp = ((0x00FF0000 & delay) >> 16);
        return_value = pb_outp(port_base + 6, temp);
        if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = "Communications error (loop 2)";
	      debug ("pb_4C_stop: %s\n", spinerr);
	      debug ("return value was: %d\n", return_value);
	      return return_value;
	    }
		
	    temp = 0;
        temp = ((0x0000FF00 & delay) >> 8);
        return_value = pb_outp(port_base + 6, temp);
        if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = "Communications error (loop 3)";
	      debug ("pb_4C_stop: %s\n", spinerr);
	      debug ("return value was: %d\n", return_value);
	      return return_value;
	    }
	    temp = 0;
        temp = (0x000000FF & delay);
        return_value = pb_outp(port_base + 6, temp);
        if (return_value != 0 && (!(ISA_BOARD)))
	    {
	      spinerr = "Communications error (loop 4)";
	      debug ("pb_4C_stop: %s\n", spinerr);
	      debug ("return value was: %d\n", return_value);
	      return return_value;
	    }

    }
    else
        return pb_inst_pbonly(0, STOP, 0, 25 * ns); 

	return 0;
}

SPINCORE_API int
pb_inst_pbonly (unsigned int flags, int inst, int inst_data, double length)
{
	__int64 flags64 = flags;
	return pb_inst_pbonly64(flags64, inst, inst_data, length);
}

SPINCORE_API int
pb_inst_pbonly64 (__int64 flags, int inst, int inst_data, double length)
{
  unsigned int delay;
  double pb_clock, clock_period;

  spinerr = noerr;

  pb_clock = board[cur_board].clock * board[cur_board].pb_clock_mult;
  clock_period = 1.0 / pb_clock;

  delay = (unsigned int) rint ((length * pb_clock) - 3.0);	//(Assumes clock in GHz and length in ns)


  debug ("pb_inst_pbonly: inst=%lld, inst_data=%d,length=%f, flags=0x%.8x\n", inst, inst_data,
	 length, flags);

  if (delay < 2)
    {
      spinerr = "Instruction delay is too small to work with your board";
      debug ("pb_inst_pbonly: %s\n", spinerr);
      return -91;
    }

  if (inst == LOOP)
    {
      if (inst_data == 0)
	{
	  spinerr = "Number of loops must be 1 or more";
	  debug ("pb_inst_pbonly: %s\n", spinerr);
	  return -1;
	}
      inst_data -= 1;
    }
  if (inst == LONG_DELAY)
    {
      if (inst_data == 0 || inst_data == 1)
	{
	  spinerr = "Number of repetitions must be 2 or more";
	  debug ("pb_inst_pbonly: %s\n", spinerr);
	  return -1;
	}
      inst_data -= 2;
    }

  //-------------------PB CORE counter issue--------------------------------------------------------------------------------------------------------------------------------------------------
  //An extra clock cycle must be subtracted from all instructions that result in a value ending in 0xFF being sent to the core counter (with the exception of 0x0FF).
  //Boards which have been fixed (and which have readable firmware IDs) do not require this and will bypass the following software fix.
  //
  //For boards which have been fixed but do not have firmware registers, there is no way for spinapi to know whether or not the fault has been corrected.
  //Therefore, on all generic PulseBlaster boards and also on the older PBESR boards (boards using AMCC_DEVID and PBESR_PRO_DEVID) the 'FF' fix will
  //be applied by default.  If you wish to bypass this fix, please use:
  //      pb_bypass_FF_fix (1);     //bypass the fix below - no extra clock cycle will be subtracted ( board[cur_board].has_FF_fix will be set to 1 )
  //      pb_bypass_FF_fix(0):    //turn on the software fix - ( board[cur_board].has_FF_fix will be set to 0 )
  //
  if (board[cur_board].has_FF_fix != 1)
    {
      if (((delay & 0xFF) == 0xFF) && (delay > 0xFF))
	{
	  delay--;
	  debug ("pb_inst_pbonly: __ONE CLOCK CYCLE SUBTRACTED__\n", spinerr);
	}
    }
  //_______________________________________________________________________________________________________________

  //------------------------
  // SP16 Designs 15-1, 15-2, and 15-3 have Flag0 and Flag1 reversed in Firmware
  if( (board[cur_board].firmware_id > 0xF0) && (board[cur_board].firmware_id <= 0xF3) )
      flags = (flags & 0xFFFFFFFC) + ((flags & 0x01) << 1) + ((flags & 0x02) >> 1);

  return pb_inst_direct (((int*)&flags), inst, inst_data, delay);
}

SPINCORE_API int
pb_inst_dds2 (int freq0, int phase0, int amp0, int dds_en0, int phase_reset0,
	      int freq1, int phase1, int amp1, int dds_en1, int phase_reset1,
	      int flags, int inst, int inst_data, double length)
{
  if (board[cur_board].firmware_id != 0xe01 && board[cur_board].firmware_id != 0xe02 && board[cur_board].firmware_id != 0x0E03 && board[cur_board].firmware_id != 0x0C13)
    {
      debug
	("pb_inst_dds2: Your current board does not support this function. Please check your manual.\n");
      return 0;

    }
  spinerr = noerr;

  if (freq0 >= board[cur_board].dds_nfreq[0] || freq0 < 0)
    {
      spinerr = "Frequency register select 0 out of range";
      debug ("pb_inst_dds2: %s\n", spinerr);
      return -1;
    }

  if (freq1 >= board[cur_board].dds_nfreq[1] || freq1 < 0)
    {
      spinerr = "Frequency register select 1 out of range";
      debug ("pb_inst_dds2: %s\n", spinerr);
      return -1;
    }

  if (phase0 >= board[cur_board].dds_nphase[0] || phase0 < 0)
    {
      spinerr = "TX phase register select 0 out of range";
      debug ("pb_inst_dds2: %s\n", spinerr);
      return -1;
    }

  if (phase1 >= board[cur_board].dds_nphase[1] || phase1 < 0)
    {
      spinerr = "TX phase register select 1 out of range";
      debug ("pb_inst_dds2: %s\n", spinerr);
      return -1;
    }

  if (amp0 >= board[cur_board].dds_namp[0] || amp0 < 0)
    {
      spinerr = "Amplitude register select 0 out of range";
      debug ("pb_inst_dds2: %s\n", spinerr);
      return -1;
    }

  if (amp1 >= board[cur_board].dds_namp[1] || amp1 < 0)
    {
      spinerr = "Amplitude register select 1 out of range";
      debug ("pb_inst_dds2: %s\n", spinerr);
      return -1;
    }

  debug
    ("pb_inst_dds2: using DDS 96 bit partitioning scheme(no shape support)\n",
     inst, inst_data, length);

  unsigned int delay;
  double pb_clock, clock_period;

  spinerr = noerr;

  pb_clock = board[cur_board].clock * board[cur_board].pb_clock_mult;
  clock_period = 1.0 / pb_clock;

  delay = (unsigned int) rint ((length * pb_clock) - 3.0);	//(Assumes clock in GHz and length in ns)

  debug ("pb_inst_dds2: inst=%d, inst_data=%d,length=%f\n", inst, inst_data,
	 length);
  debug ("pb_inst_dds2: freq0=0x%X, phase0=0x%X, amp0=0x%X, freq1=0x%X, phase1=0x%X, amp1=0x%X\n",freq0,phase0,amp0,freq1,phase1,amp1);

  if (delay < 2)
    {
      spinerr = "Instruction delay is too small to work with your board";
      debug ("pb_inst_dds2: %s\n", spinerr);
      return -91;
    }

  if (inst == LOOP)
    {
      if (inst_data == 0)
	{
	  spinerr = "Number of loops must be 1 or more";
	  debug ("pb_inst_dds2: %s\n", spinerr);
	  return -1;
	}
      inst_data -= 1;
    }

  if (inst == LONG_DELAY)
    {
      if (inst_data == 0 || inst_data == 1)
	{
	  spinerr = "Number of repetitions must be 2 or more";
	  debug ("pb_inst_dds2: %s\n", spinerr);
	  return -1;
	}
      inst_data -= 2;
    }
  int flag_word[3];

  flag_word[0] = flag_word[1] = flag_word[2] = 0;
  
  if(board[cur_board].firmware_id == 0x0C13)
  {
	  flag_word[0] |= (flags & 0xF) << 0;        // 4 Flag Bits
	  flag_word[0] |= (dds_en0 & 0x1) << 4;      // DDS Tx Enable
	  flag_word[0] |= (phase_reset0 & 0x1) << 5; // Phase Reset
	  flag_word[0] |= (freq0 & 0x3FF) << 6;      // 10 Frequency Select Bits (1024 Freq. Registers)
	  flag_word[0] |= (phase0 & 0x7F) << 16;     // 7 Phase Select Bits (128 Phase Registers)
	  flag_word[0] |= (amp0 & 0x1FF) << 23;      // lower 9 of 10 Amp. Select Bits
	  
	  flag_word[1] |= (amp0 & 0x200) >> 9;       // upper 1 of 10 Amp. Select Bits.
  }
  else if(board[cur_board].firmware_id == 0x0E03)
  {
    flag_word[0] |= (flags & 0xF) << 0;           // 4 Flag Bits
    flag_word[0] |= (dds_en0 & 0x1) << 4;         // DDS1 Tx Enable
    flag_word[0] |= (dds_en1 & 0x1) << 5;         // DDS2 Tx Enable
    flag_word[0] |= (phase_reset0 & 0x1) << 6;    // DDS1 Phase Reset
    flag_word[0] |= (phase_reset1 & 0x1) << 7;    // DDS2 Phase Reset
    flag_word[0] |= (freq0 & 0x3FF) << 8;         // DDS1 Frequency Select Bits (1024 Freq. Registers)
    flag_word[0] |= (freq1 & 0x3FF) << 18;        // DDS2 Frequency Select Bits (1024 Freq. Registers)
    flag_word[0] |= (phase0 & 0xF) << 28;         // Lower 4 DDS1 Phase Select Bits (128 Phase Registers)

    flag_word[1] |= (phase0 & 0x70) >> 4;         // Upper 3 DDS1 Phase Select Bits
	flag_word[1] |= (phase1 & 0x7F) << 3;         // DDS2 Phase Select Bits (128 Phase Registers)
    flag_word[1] |= (amp0 & 0x3FF) << 10;         // DDS1 Amplitude Select Bits (1024 Amp. Registers)
    flag_word[1] |= (amp1 & 0x3FF) << 20;         // DDS2 Amplitude Select Bits (1024 Amp. Registers)
  }
  else
  {
    flag_word[0] |= (flags & 0xFFF) << 0;
    flag_word[0] |= (dds_en0 & 0x1) << 12;
    flag_word[0] |= (dds_en1 & 0x1) << 13;
    flag_word[0] |= (phase_reset0 & 0x1) << 14;
    flag_word[0] |= (phase_reset1 & 0x1) << 15;
    flag_word[0] |= (freq0 & 0xF) << 16;
    flag_word[0] |= (freq1 & 0xF) << 20;
    flag_word[0] |= (phase0 & 0x7) << 24;
    flag_word[0] |= (phase1 & 0x7) << 27;
    flag_word[0] |= (amp0 & 0x3) << 30;

    flag_word[1] |= (amp1 & 0x3) << 0;
  }

  return pb_inst_direct(flag_word, inst, inst_data, delay);
}

SPINCORE_API int
pb_inst_direct (int *pflags, int inst, int inst_data_direct, int length)
{
  int instruction[8];
  int i;
  int flags;
  int return_value;
  unsigned int temp_byte;
  unsigned int BIT_MASK = 0xFF0000;
  unsigned int OCW = 0;
  unsigned int OPCODE = 0;
  unsigned int DELAY = 0;

  spinerr = noerr;

    if (board[cur_board].usb_method == 2) {
		instruction[0] = length;
        instruction[1] = (0xF & inst) | ((0xFFFFF & inst_data_direct) << 4) | ((0xFF & pflags[0]) <<24);
        instruction[2] = ((0xFFFFFF & (pflags[0] >> 8)) << 0) | ((pflags[1] & 0xFF) << 24);
        instruction[3] = ((0xFFFFFF & (pflags[1] >> 8)) << 0) | ((pflags[2] & 0xFF) << 24);
		instruction[4] = 0;
		instruction[5] = 0;
		instruction[6] = 0;
		instruction[7] = 0;

        debug ("pb_inst_direct: Programming DDS2 IMW: 0x%X %X %X %X.\n", instruction[3], instruction[2], instruction[1], instruction[0]);

		if(board[cur_board].firmware_id == 0x0E03)
           usb_write_data (instruction, 8);
		else
		   usb_write_data (instruction, 4);
    }
	else{
	
  // Make sure the different fields of the instruction are within 
  // the proper range
  if (board[cur_board].firmware_id == 0xa13 || board[cur_board].firmware_id == 0xC10)
    {				//Need to replace this asap.
		flags = pflags[0];
      if (flags != (flags & 0x0FFFFFFFF))
	{
	  spinerr = "Flag word is limited to 32 bits";
	  debug ("pb_inst_direct: %s\n", spinerr);
	  return -1;
	}
    }
  else
    {
		flags = pflags[0];
		  if (flags != (flags & 0x0FFFFFF))
		{
		  spinerr = "Flag word is limited to 24 bits";
		  debug ("pb_inst_direct: %s\n", spinerr);
		  return -1;
		}
		}

	  if (inst > 8)
		{
		  spinerr = "Invalid opcode";
		  debug ("pb_inst_direct: %s\n", spinerr);
		  return -1;
		}
	  if (inst_data_direct != (inst_data_direct & 0x0FFFFF))
		{
		  spinerr = "Instruction is limited to 20 bits";
		  debug ("pb_inst_direct: %s\n", spinerr);
		  return -1;
		}

	  OPCODE = (inst) | (inst_data_direct << 4);
	  OCW = flags;
	  DELAY = length;

	  debug ("pb_inst_direct: OPCODE=0x%x, flags=0x%.8x, delay=%d\n", OPCODE, OCW,
		 DELAY);
	  if (board[cur_board].firmware_id == 0xa13 || board[cur_board].firmware_id == 0xC10)
		{				//Need to replace this asap.
		  for (i = 0; i < 4; i++)
		{
		  temp_byte = 0xFF000000 & OCW;
		  temp_byte >>= 24;
		  return_value = pb_outp (port_base + 6, temp_byte);
		  if (return_value != 0 && (!(ISA_BOARD)))
			{
			  spinerr = "Communications error (loop 1)";
			  debug ("pb_inst_direct: %s\n", spinerr);
			  debug ("return value was: %d\n", return_value);
			  return return_value;
			}
		  OCW <<= 8;
		}
		}
		else if (board[cur_board].firmware_id == 0x0908)
		{				//Need to replace this asap.
		  temp_byte = 0xFF & OCW;
		  return_value = pb_outp (port_base + 6, temp_byte);
		  if (return_value != 0 && (!(ISA_BOARD)))
			{
			  spinerr = "Communications error (loop 1)";
			  debug ("pb_inst_direct: %s\n", spinerr);
			  debug ("return value was: %d\n", return_value);
			  return return_value;
			}
		}
	  else
		{
		  for (i = 0; i < 3; i++)
		{
		  temp_byte = BIT_MASK & OCW;
		  temp_byte >>= 16;
		  return_value = pb_outp (port_base + 6, temp_byte);
		  if (return_value != 0 && (!(ISA_BOARD)))
			{
			  spinerr = "Communications error (loop 1)";
			  debug ("pb_inst_direct: %s\n", spinerr);
			  debug ("return value was: %d\n", return_value);
			  return return_value;
			}
		  OCW <<= 8;
		}
		}
	  BIT_MASK = 0xFF0000;
	  for (i = 0; i < 3; i++)
		{

		  temp_byte = BIT_MASK & OPCODE;
		  temp_byte >>= 16;
		  return_value = pb_outp (port_base + 6, temp_byte);
		  if (return_value != 0 && (!(ISA_BOARD)))
		{
		  spinerr = "Communications error (loop 2)";
		  debug ("pb_inst_direct: %s\n", spinerr);
		  return return_value;
		}
		  OPCODE <<= 8;
		}
	  BIT_MASK = 0xFF000000;
	  for (i = 0; i < 4; i++)
		{
		  temp_byte = BIT_MASK & DELAY;
		  temp_byte >>= 24;
		  return_value = pb_outp (port_base + 6, temp_byte);
		  if (return_value != 0 && (!(ISA_BOARD)))
		{
		  spinerr = "Communications error (loop 3)";
		  debug ("pb_inst_direct: %s\n", spinerr);
		  return return_value;
		}
		  DELAY <<= 8;
    }
  }
  num_instructions += 1;
  return num_instructions - 1;
}

SPINCORE_API int
pb_set_freq (double freq)
{
  spinerr = noerr;

  unsigned int freq_byte = 0xFF000000;	//Bit Mask to send 1 byte at a time
  double dds_clock;
  unsigned int freq_word;
  unsigned int freq_word2;
  unsigned int temp_byte;
  int return_value;
  int i = 0;

  dds_clock = board[cur_board].clock;
  freq_word = (unsigned int) rint (((freq * pow232) / (dds_clock * 1000.0)));	// Desired data to trasnfer
  freq_word2 =
    (unsigned int)
    rint (((freq * pow232) /
	   (dds_clock * 1000.0 * board[cur_board].dds_clock_mult)));

  debug
    ("pb_set_freq: address:%d freq:%lf freq_word:%x freq_word2:%x clock:%lf\n",
     cur_device_addr, freq, freq_word, freq_word2, dds_clock);

  if (board[cur_board].usb_method == 2)
    {
      if (cur_device_addr >= board[cur_board].dds_nfreq[cur_dds])
	{
	  spinerr = "Frequency registers full";
	  debug ("pb_set_freq: %s\n", spinerr);
	  return -1;
	}

      usb_write_data (&freq_word2, 1);

    }
  else
    {
      // Check if use has already written to all registers
      if (cur_device_addr >= board[cur_board].num_freq0)
	{
	  spinerr = "Frequency registers full";
	  debug ("pb_set_freq: %s\n", spinerr);
	  return -1;
	}

      if (board[cur_board].dds_prog_method == DDS_PROG_OLDPB)
	{
	  debug ("pb_set_freq: using old programming method\n");

	  for (i = 0; i < 4; i++)	// Loop to send 4 bytes
	    {
	      temp_byte = freq_byte & freq_word;	// Get current byte to transfer
	      temp_byte >>= 24;	// Shift data into LSB
	      return_value = pb_outp (port_base + 6, temp_byte);	// Send byte to PulseBlasterDDS
	      if (return_value != 0 && (!(ISA_BOARD)))
		{
		  return -1;
		}

	      freq_word <<= 8;	// Shift word up one byte so that next byte to transfer is in MSB
	    }
	}
      else
	{
	  debug ("pb_set_freq: using new programming method\n");
	  dds_freq_extreg (cur_board, cur_device_addr, freq_word, freq_word2);
	}
    }

  cur_device_addr++;

  return 0;
}

SPINCORE_API int
pb_set_phase (double phase)
{
  spinerr = noerr;

  unsigned int phase_byte = 0xFF000000;	//Bit Mask to send 1 byte at a time

  double temp = phase / 360.0;

  // fix phase to be 0 <phase <360.
  while (temp >= 1.0)
    {
      temp -= 1.0;
    }
  while (temp < 0.0)
    {
      temp += 1.0;
    }

  double temp2 = temp * pow232;
  unsigned int phase_word = (unsigned int) rint (temp2);

  unsigned int temp_byte;
  int return_value;
  int i = 0;

  if (board[cur_board].usb_method == 2)
    {
      // Check if user has already written to all registers
      if (cur_device_addr >= board[cur_board].dds_nphase[cur_dds])
	{
	  spinerr = "Phase registers full";
	  debug ("pb_set_phase: %s\n", spinerr);
	  return -1;
	}
      phase_word >>= 20;
      debug ("pb_set_phase: phase word: 0x%x\n", phase_word);
      usb_write_data (&phase_word, 1);
    }
  else
    {
      int max_phase_regs = 0;

      if (cur_device == COS_PHASE_REGS)
	{
	  max_phase_regs = board[cur_board].num_phase0;
	}
      if (cur_device == SIN_PHASE_REGS)
	{
	  max_phase_regs = board[cur_board].num_phase1;
	}
      if (cur_device == TX_PHASE_REGS)
	{
	  if (board[cur_board].is_radioprocessor)
	    {
	      max_phase_regs = board[cur_board].num_phase2;
	    }
	  else
	    {
	      max_phase_regs = board[cur_board].num_phase1;
	    }
	}
      if (cur_device == RX_PHASE_REGS)
	{
	  max_phase_regs = board[cur_board].num_phase0;
	}

      // Check if use has already written to all registers
      if (cur_device_addr >= max_phase_regs)
	{
	  spinerr = "Phase registers full";
	  debug ("pb_set_phase: %s\n", spinerr);
	  return -1;
	}

      if (board[cur_board].dds_prog_method == DDS_PROG_OLDPB)
	{
	  for (i = 0; i < 4; i++)	// Loop to send 4 bytes
	    {
	      temp_byte = phase_byte & phase_word;	// Get current byte to transfer
	      temp_byte >>= 24;	// Shift data into LSB
	      return_value = pb_outp (port_base + 6, temp_byte);	// Send byte to PulseBlasterDDS
	      if (return_value != 0 && (!(ISA_BOARD)))	// conio's _outp returns value passed to it
		{
		  return -1;
		}

	      phase_word <<= 8;	// Shift word up one byte so that next byte to transfer is in MSB
	    }
	}
      else
	{
	  dds_phase_extreg (cur_board, cur_device, cur_device_addr,
			    phase_word);
	}
    }

  cur_device_addr++;

  return 0;
}

/*
// Disable for now, need to add support to the appropriate functions first
SPINCORE_API double pb_get_rounded_value()
{
    spinerr = noerr;
    
    return last_rounded_value;
}
*/

SPINCORE_API int
pb_read_status (void)
{
  int status;

  spinerr = noerr;

  if (board[cur_board].usb_method == 2) {
      debug ("pb_read_status: Using partial address decoding method.\n:");
      return status = reg_read (board[cur_board].pb_base_address);
  }
  if (board[cur_board].status_oldstyle) {
      debug ("pb_read_status: using oldstyle\n");
      status =  pb_inp (0) & 0xF;
    }
  else{
      status = reg_read (REG_EXPERIMENT_RUNNING);
    }
	
	return status;
}
 
SPINCORE_API char
*pb_status_message()
{
    int message_count=0;
    int status=pb_read_status();
    
    if(status < 0){
        strcpy (status_message, "Status error: ");
        if(status == -1)
        {
            strcat (status_message, "can't communicate with board.");
        }
        else if(status == -2)
        {
            strcat (status_message, "connection with the board timed out while sending address.");
        }
        else if(status == -3)
        {
            strcat (status_message, "connection with board timed out while receiving data.");
        }
        else
        {
            strcat (status_message, "can't communicate with board.");
        }
        strcat (status_message, "\nTry reinstalling SpinAPI.\n");
    }
    else
    {
        strcpy (status_message,"Board is ");
        
        if((status&0x01)!=0x00){
            //Stopped
            strcat(status_message,"stopped");
            message_count++;
        }
        if((status&0x02)!=0x00){
            //Reset
            if(message_count>0){
                strcat(status_message," and ");   
            }
            strcat(status_message,"reset");
            message_count++;
        }
        if((status&0x04)!=0x00){
            //Running
            if(message_count>0){
                strcat(status_message," and ");   
            }
            strcat(status_message,"running");
            message_count++;
        }
        if((status&0x08)!=0x00){
            //Waiting
            if(message_count>0){
                strcat(status_message," and ");   
            }
            strcat(status_message,"waiting");
            message_count++;
        }
        if((status&0x10)!=0x00){
            //Scanning
            if(message_count>0){
                strcat(status_message," and ");   
            }
            strcat(status_message,"scanning");
            message_count++;
        }
        strcat(status_message,".\n"); 
    }
    return status_message;
}

//Couldn't call reg_read from outside spinapi, so this is a quick workaround
//    used for the PBESR that was turned into a Timing Analyzer 
//To read from Status31..0, pass address 0x000C.
SPINCORE_API int
pb_read_fullStat (int address)
{
  int status;

  spinerr = noerr;

  status = reg_read (address);
  return status;
}

SPINCORE_API char *
pb_get_version (void)
{
  spinerr = noerr;

  return version;
}

SPINCORE_API int
pb_get_firmware_id (void)
{
  spinerr = noerr;

  if (board[cur_board].has_firmware_id)
    {

      debug ("pb_get_firmware_id: has id\n");
      /*
         if(board[cur_board].has_firmware_id == 1) {
         return reg_read(REG_FIRMWARE_ID);    
         }
         else {
         return reg_read(0x15); // usb boards have the firmware ID stored in register 0x15
         } */
      return board[cur_board].firmware_id;
    }
  else
    {
      return 0;
    }
}

SPINCORE_API char *
pb_get_error (void)
{
  return spinerr;
}

SPINCORE_API void
pb_set_ISA_address (int address)
{
  spinerr = noerr;

  port_base = address;
}

///
/// Low level IO functions. Typically the end user will use the above functions
/// to access the hardware, and these functions are mainly included in the
/// API for backwards compatibility.
///

SPINCORE_API int
pb_outp (unsigned int address, char data)
{
  spinerr = noerr;

  // If this is a USB device...
  if (board[cur_board].is_usb) {
	  debug (" pb_outp: addr %x, data %x. Using the USB protocol.\n", address, data);
      return usb_do_outp (address, data);
  }
  else {  // Otherwise, if it is a PCI device...
		if (board[cur_board].use_amcc == 1) {
		  debug (" pb_outp: addr %x, data %x. Using the AMCC protocol.\n", address, data);
		  return do_amcc_outp (cur_board, address, data);
		}
		else if (board[cur_board].use_amcc == 2) {
		  debug (" pb_outp: addr %x, data %x. Using the AMCC protocol (old).\n", address, data);
		  return do_amcc_outp_old (cur_board, address, data);
		}
		else {
		  debug (" pb_outp: addr %x, data %x. Using the direct protocol.", address, data);
		  return os_outp (cur_board, address, data);
		}
    }
}

SPINCORE_API char
pb_inp (unsigned int address)
{
  spinerr = noerr;

  if (board[cur_board].is_usb)
    {
      debug ("pb_inp: no support for usb devices\n");
      return -1;
    }

  if (board[cur_board].use_amcc)
    {
      if (board[cur_board].use_amcc == 2)
	{
	  spinerr = "Input from board not supported with this board revision";
	  debug ("pb_inp: %s\n", spinerr);
	  return -1;
	}
      return do_amcc_inp (cur_board, address);
    }
  else
    {
      return os_inp (cur_board, address);
    }
}

SPINCORE_API int
pb_outw (unsigned int address, unsigned int data)
{
  spinerr = noerr;

  if (board[cur_board].is_usb)
    {
      debug ("pb_outw: no support for usb devices\n");
      return -1;
    }

  // amcc chip does not use 32 bit I/O, so this must be our custom PCI core
  return os_outw (cur_board, address, data);
}

SPINCORE_API unsigned int
pb_inw (unsigned int address)
{
  spinerr = noerr;

  if (board[cur_board].is_usb)
    {
      debug ("pb_inw: no support for usb devices\n");
      return -1;
    }

  // amcc chip does not use 32 bit I/O, so this must be our custom PCI core
  return os_inw (cur_board, address);
}

SPINCORE_API void
pb_sleep_ms (int milliseconds)
{
#ifdef WINDOWS
  Sleep (milliseconds);		// on windows, sleep() function takes values in ms.
#else
#include <unistd.h>
  usleep (milliseconds * 1000);	// this linux function takes time in us
#endif
}




int
do_os_init (int board)
{
  int dev_id;

  debug ("do_os_init: board # %d\n", board);
  debug ("do_os_init: num_pci_boards: %d\n", num_pci_boards);
  debug ("do_os_init: num_usb_devices: %d\n", num_usb_devices);


  if (board < num_pci_boards)
    {
      debug ("do_os_init: initializing pci\n");
      dev_id = os_init (board);
    }
  else
    {
      debug ("do_os_init: initializing usb\n");
      dev_id = os_usb_init (board - num_pci_boards);
      usb_reset_gpif (board - num_pci_boards);
    }

  return dev_id;
}

int
do_os_close (int board)
{
  int ret = -1;

  if (board < num_pci_boards)
    {
      debug ("do_os_close: closing pci\n");
      ret = os_close (board);
    }
  else
    {
      debug ("do_os_close: closing usb\n");
      ret = os_usb_close ();
    }

  return ret;
}

SPINCORE_API void
pb_bypass_FF_fix (int option)
{
  switch (option)
    {
    case 1:
      {
	debug
	  ("pb_bypass_FF_fix: bypassing software fix.. no clock cycles will be subtracted from 0x..FF delays.\n");
	board[cur_board].has_FF_fix = 1;
	break;
      }
    case 0:
      {
	debug
	  ("pb_bypass_FF_fix: software fix turned on: one clock cycle will be subtracted from 0x..FF delays.\n");
	board[cur_board].has_FF_fix = 0;
	break;
      }
    default:
      {
	debug
	  ("pb_bypass_FF_fix: invalid argument. please enter 1 to bypass or 0 to enable.\n");
	break;
      }
    }

}

SPINCORE_API int
pb_inst_hs8(char* Flags, double length)
{
     int i;
     int num_cycles;
     char hex_flags0;
     double clock_freq = board[cur_board].clock;
     spinerr = noerr;
     
     //Check for 8-bits 
     if(strlen(Flags) != 8)
     {
        spinerr = "Must define a value (1 or 0) for all 8 bits!";
        debug("pb_inst_hs8: %s\n",spinerr);
        return -2;
     }
     
     //Verify all bits are either 0 or 1 
     for(i=0;i<8;i++)
     {
        if((Flags[i] > 49) || (Flags[i] < 48))
        {
           spinerr = "Flag bits must be either 0 or 1!";
           debug("pb_inst_hs8: %s\n",spinerr);
           printf("flag[%d]: %d\n",i,(Flags[i]));
           return -3;
        }
     }
     
     //Convert the string of ones and zeros to an 8-bit decimal number
     hex_flags0 =(Flags[0] - 48)*128 + 
                 (Flags[1] - 48)*64 + 
                 (Flags[2] - 48)*32 + 
                 (Flags[3] - 48)*16 + 
                 (Flags[4] - 48)*8 + 
                 (Flags[5] - 48)*4 + 
                 (Flags[6] - 48)*2 + 
                 (Flags[7] - 48)*1;
                 
     
     //Convert the length from nanoseconds to a number of clock cycles        
     num_cycles = (int) rint(length*clock_freq); 
     if(num_cycles < 1)
     {
        spinerr = "Length must be greater than or equal to one clock period";
        debug("pb_inst_hs8: %s\n",spinerr);
        return -4;
     }
     
     //Each pb_outp instruction controls the output for one clock cycle,
     //  so send this line to the board as many times as it takes to
     //  create the requested instruction length.
     for (i = 1; i <= num_cycles; i++)
         pb_outp(6,hex_flags0);     
        
     return num_cycles;
}

SPINCORE_API int
pb_inst_hs24(char* Flags, double length)
{
     int i,num_cycles;
     char hex_flags0,hex_flags1,hex_flags2;
     double clock_freq = board[cur_board].clock;
     spinerr = noerr;
     
     //Check for 24-bits 
     if(strlen(Flags) != 24)
     {
        spinerr = "Must define states (1 or 0) for all 24 bits!";
        debug("pb_inst_hs24: %s\n",spinerr);
        return -2;
     }
     
     //Verify all bits are either 0 or 1 
     for(i=0;i<24;i++)
     {
        if(Flags[i] > 49 || Flags[i] < 48)
        {
           spinerr = "Flag bits must be either 0 or 1";
           debug("pb_inst_hs24: %s\n",spinerr);
           return -3;
        }
     }
     
     //Convert the string of ones and zeros to an 8-bit decimal number 
     hex_flags0 =(Flags[0] - 48)*128 + 
                 (Flags[1] - 48)*64 + 
                 (Flags[2] - 48)*32 + 
                 (Flags[3] - 48)*16 + 
                 (Flags[4] - 48)*8 + 
                 (Flags[5] - 48)*4 + 
                 (Flags[6] - 48)*2 + 
                 (Flags[7] - 48)*1;
     hex_flags1 =(Flags[8] - 48)*128 + 
                 (Flags[9] - 48)*64 + 
                 (Flags[10] - 48)*32 + 
                 (Flags[11] - 48)*16 + 
                 (Flags[12] - 48)*8 + 
                 (Flags[13] - 48)*4 + 
                 (Flags[14] - 48)*2 + 
                 (Flags[15] - 48)*1; 
     hex_flags2 =(Flags[16] - 48)*128 + 
                 (Flags[17] - 48)*64 + 
                 (Flags[18] - 48)*32 + 
                 (Flags[19] - 48)*16 + 
                 (Flags[20] - 48)*8 + 
                 (Flags[21] - 48)*4 + 
                 (Flags[22] - 48)*2 + 
                 (Flags[23] - 48)*1;
                 
     
     //Convert the length from nanoseconds to a number of clock cycles        
     num_cycles = (int) rint(length*clock_freq);
     if(num_cycles < 1)
     {
        spinerr = "Length must be greater than or equal to one clock period";
        debug("pb_inst_hs24: %s\n",spinerr);
        return -4;
     } 
     
     //Each pb_outp instruction controls the output for one clock cycle,
     //  so send this line to the board as many times as it takes to
     //  create the requested instruction length.
     for (i = 1; i <= num_cycles; i++)
     {
         pb_outp(6,hex_flags0);
         pb_outp(6,hex_flags1);
         pb_outp(6,hex_flags2);
     }        
        
     return num_cycles;
}


SPINCORE_API int
pb_select_dds (int dds_num)
{
  if (board[cur_board].firmware_id == 0xe01 || board[cur_board].firmware_id == 0xe02 || board[cur_board].firmware_id == 0x0E03)
    {
      if (dds_num >= 0 && dds_num <= board[cur_board].number_of_dds)
	{
	  debug ("pb_select_dds: Setting current dds to dds #%d\n", dds_num);
	  cur_dds = dds_num;
	}
    }
  else
    debug
      ("pb_select_dds: Your current board does not support this function.\n");

  return 0;
}

SPINCORE_API int
pb_set_isr (int irq_num, unsigned int isr_addr)
{
  if (board[cur_board].firmware_id != 0xe01 && board[cur_board].firmware_id != 0xe02 && board[cur_board].firmware_id != 0x0E03)
    {
      debug
	("pb_set_isr: Your current board does not support this function.\n");
      return -1;
    }

  if (irq_num < 0 || irq_num > 3)
    {
      debug ("pb_set_isr: The IRQ number specified is invalid.\n");
      return -1;
    }

  usb_write_address (board[cur_board].pb_base_address + 0x03 + irq_num);
  usb_write_data (&isr_addr, 1);
  debug ("pb_set_isr: IRQ #%d set to ISR Address 0x%x.\n", irq_num, isr_addr);

  return 0;

}
SPINCORE_API int
pb_set_irq_enable_mask (char mask)
{
  int imask = (int) mask;

  if (board[cur_board].firmware_id != 0xe01 && board[cur_board].firmware_id != 0xe02 && board[cur_board].firmware_id != 0x0E03)
    {
      debug
	("pb_set_irq_enable_mask: Your current board does not support this function.\n");
      return -1;
    }

  usb_write_address (board[cur_board].pb_base_address + 0x01);
  usb_write_data (&imask, 1);
  debug ("pb_set_irq_enable_mask: IRQ Enable Mask set to 0x%x.\n",
	 mask & 0x0F);

  return 0;
}
SPINCORE_API int
pb_set_irq_immediate_mask (char mask)
{
  int imask = (int) mask;

  if (board[cur_board].firmware_id != 0xe01 && board[cur_board].firmware_id != 0xe02 && board[cur_board].firmware_id != 0x0E03)
    {
      debug
	("pb_set_irq_immediate_mask: Your current board does not support this function.\n");
      return -1;
    }

  usb_write_address (board[cur_board].pb_base_address + 0x02);
  usb_write_data (&imask, 1);
  debug ("pb_set_irq_immediate_mask: IRQ Immediate Mask set to 0x%x.\n",
	 mask & 0x0F);

  return 0;
}

SPINCORE_API int
pb_generate_interrupt (char mask)
{
  int imask = (int) mask;

  if (board[cur_board].firmware_id != 0xe01 && board[cur_board].firmware_id != 0xe02 && board[cur_board].firmware_id != 0x0E03)
    {
      debug
	("pb_generate_interrupt: Your current board does not support this function.\n");
      return -1;
    }
  imask = (imask & 0xF) << 4;	//Shift into the IRQ control bits.
  usb_write_address (board[cur_board].pb_base_address + 0x00);
  usb_write_data (&imask, 1);

  debug ("pb_generate_interrupt: IRQ Mask set to 0x%x.\n", mask & 0x0F);

  return 0;
}

SPINCORE_API int
pb_write_register (unsigned int address, unsigned int value)
{
  if (board[cur_board].usb_method != 2) {
      reg_write(address, value);
  }
  else {
	usb_write_address (address);
	usb_write_data (&value, 1);
  }
  
  debug ("pb_write_register: Wrote 0x%x to 0x%x.\n", value, address);

  return 0;
}

SPINCORE_API 
int pb_select_core (unsigned int core_sel)
{
    reg_write (REG_CORE_SEL, core_sel);
    debug ("pb_select_core: Wrote 0x%x.\n", core_sel);
    return 0;
}

SPINCORE_API 
int pb_set_pulse_regs (unsigned int channel, double period, double clock_high, double offset)
{  
    int data[3];
    
	// period
    data[0] = (period/20.0);
    // clock high
    data[1] = (clock_high/20.0);
    // offset
	data[2] = (offset/20.0);
    
    int one = 1;
	int zero = 0;
    
    if(channel < 4){
			if( board[cur_board].prog_clock_base_address != 0x0 ) {
				// program the registers
				usb_write_address( board[cur_board].prog_clock_base_address + (channel+1) * 0x10 );
				usb_write_data( data, 3 );
				usb_write_address( board[cur_board].prog_clock_base_address );
				usb_write_data( &one, 1 );
				
				// sync the clocks
				usb_write_address( board[cur_board].prog_clock_base_address );
				usb_write_data( &zero, 1 );
			} else {
				// There needs to be a way to check if the board has this functionality
             	reg_write ((REG_PULSE_PERIOD + channel), data[0]);
             	reg_write ((REG_PULSE_CLOCK_HIGH + channel), data[1]);
             	reg_write ((REG_PULSE_OFFSET + channel), data[2]);
             	reg_write(REG_PULSE_SYNC,1);
             	reg_write(REG_PULSE_SYNC,0);
			}
        	debug ("Programmable Fixed Pulse Output #:0x%x.\n", channel);
            debug ("pb_set_pulse_regs: Wrote period: 0x%x.\n", data[0]);
            debug ("pb_set_pulse_regs: Wrote pulse width: 0x%x.\n", data[1]);
            debug ("pb_set_pulse_regs: Wrote offset: 0x%x.\n", data[2]);
        }
        else{
             
             debug ("Channel must be 0, 1, 2 or 3.\n");    
        }

    return 0;  
}

SPINCORE_API int set_pts(double maxFreq, int is160, int is3200, int allowPhase, int noPTS, double frequency, int phase)
{
	return set_pts_ex(1, maxFreq, is160, is3200, allowPhase, noPTS, frequency, phase);
}

SPINCORE_API int set_pts_ex(int pts_index, double maxFreq, int is160, int is3200, int allowPhase, int noPTS, double frequency, int phase)// , PTSDevice* device)
{
	BCDFREQ fbcd;
	int ret_val;
	
	debug("set_pts: Attempting to write %lf MHz and %d degrees to PTS\n",frequency,phase);

	if( !(phase==-1) && (phase<0 || phase>270 || (phase%90)!=0) )
	    ret_val = PHASE_INVALID;
		
	phase/=90; //Convert phase from degrees to identifying integer.

	if( is3200 )
		frequency /= 10.0;

	freq_double_to_bcd( frequency, &fbcd, is3200);
	
	if( noPTS == 0 )
		ret_val = __set_pts_(pts_index, fbcd, phase);
	else
	{
		/*	
        if( frequency > device->mfreq || frequency < 0.0 )
			return FREQ_ORANGE;
		
		if( device->allowPhase == 0)
			phase = -1;
		
		if( device->fullRange10MHz)
			fbcd.bcd_MHz[1] += fbcd.bcd_MHz[2] * 10; // if PTS160, then multiply 100MHz value by ten and add to 10MHz value
		
		ret_val = __set_pts_(pts_index, fbcd, phase);
		*/	
		if( frequency > maxFreq || frequency < 0.0 )
			return FREQ_ORANGE;
		
		if( allowPhase == 0)
			phase = -1;
		
		if( is160 )
			fbcd.bcd_MHz[1] += fbcd.bcd_MHz[2] * 10; // if PTS160, then multiply 100MHz value by ten and add to 10MHz value
		
		ret_val = __set_pts_(pts_index, fbcd, phase);
		
	}
	
	spinpts_err = ret_val; //Set last error.
	
	return ret_val;
}

 /**
 *\internal
 * This function sends the frequency and phase information to the PTS Device. The frequency must be contained within an BCDFREQ structure and the phase must be -1, 0, 1, 2 or 3. 
 * Its is not recommended that this function is used for setting the PTS directly. Please refer to int set_pts( double frequency, int phase , PTSDevice* device) to set the device frequency.
 * 
 * \param freq BCDFREQ structure
 * \param phase Must equal -1, 0, 1, 2, 3 (-1 means that this PTS does not support phase.)
 * \return Returns 0 or an error code defined in \link spinpts.h \endlink
 */
SPINCORE_API	int __set_pts_(int pts_index, BCDFREQ freq, int phase)
{
	int cntDataSent;
	
	FT_STATUS ftStatus;
    FT_HANDLE ftHandle;
	
    DWORD lpdwBytesWritten;
	DWORD ptsData[_PTS_SET_NUM_DATA];
	DWORD numDevs;

	char buffer1[64]; 				// buffer for description of first device
    char buffer2[64]; 				// buffer for description of second device
	
	char *ptr_buffers[] = { buffer1, buffer2, NULL };	
	 
	 
	if(!(phase==-1||phase==0||phase==1||phase==2||phase==3))
		return PHASE_INVALID;

	/*
     *	Set the data up to be sent to the USB-PTS 4 bytes at a time. 
	 *  Each byte sent is a  ID/BCD pair. The ^BCDBYTEMASK is used to 
	 *  invert the BCD bits because the PTS uses negative logic (low-true). 
	 *  For example, 0x4D would sent the command to set BCD 2 to  10 kHz.
	 */
	ptsData[0] = ( ((freq.bcd_MHz[2]<<0) + (freq.bcd_MHz[1]<<8) + (freq.bcd_MHz[0]<<16) + (freq.bcd_kHz[2]<<24)) ^BCDBYTEMASK) + ((ID_MHz100<<4)+(ID_MHz10 << 12)+(ID_MHz1<<20)+(ID_kHz100<<28));
	ptsData[1] = (((freq.bcd_kHz[1]<<0) + (freq.bcd_kHz[0]<<8) + (freq.bcd_Hz[2]<<16 ) + (freq.bcd_Hz[1]<<24 )) ^BCDBYTEMASK) + ((ID_kHz10<<4 )+(ID_kHz1<<12)+(ID_Hz100<<20)+(ID_Hz10 << 28));
	// Note: For devices with phase support, the  ID_pHz is used for phase.
	ptsData[2] = (((freq.bcd_Hz[0] <<0) + (((phase!=-1)?(3-phase):freq.bcd_pHz[2])<<8) )^BCDBYTEMASK ) + ((ID_Hz1<<4)+(ID_pHz<<12)+(ID_UNUSED<<20)+(ID_UNUSED<<28));
    		
    ftStatus = FT_ListDevices(ptr_buffers,&numDevs,FT_LIST_ALL|FT_OPEN_BY_DESCRIPTION);
	
	if(numDevs == 0 || pts_index < 1 || pts_index > numDevs)
	           return NO_DEVICE_FOUND;
	
	if (ftStatus == FT_OK) 
    {
                    // Open the device
                     ftStatus = FT_OpenEx(ptr_buffers[pts_index - 1],FT_OPEN_BY_DESCRIPTION,&ftHandle);
					 
                     if (ftStatus == FT_OK) 
                     {
					   debug("__set_pts_: Opened comm. with %s device\n",ptr_buffers[pts_index - 1]);
					 
						for(cntDataSent=0; cntDataSent <  _PTS_SET_NUM_DATA; cntDataSent++){
					   
							//Opening of the device succeeded. Begin writing frequency setting data
							ftStatus = FT_Write(ftHandle,&ptsData[cntDataSent],4,&lpdwBytesWritten);
						   
							if (ftStatus != FT_OK) { 
								debug("__set_pts_: Error writing to device\n");
								return DWRITE_FAIL;
							}
							debug("__set_pts_: Wrote word #%d = 0X%X to device.\n",cntDataSent,ptsData[cntDataSent]);
					   }
	   
                       FT_Close(ftHandle); 
                    }
                    else 
                    {
						return DEVICE_OPEN_FAIL;
					} 
    }
    else 
    {
	 debug("__set_pts_: Error opening Device\n");
     return DEVICE_OPEN_FAIL;
    }             
   
   return 0;
}

 /**
 * \internal
 * This function converts a double value into a BCDFREQ structure. 
 * 
 * \param freq Double value for the frequency to be converted.
 * \param bcdfreq BCDFREQ structure pointer. This is the structure that will be filled out by the function.
 * \return Returns the pointer to the structure passed in param 2.
 */
 
SPINCORE_API BCDFREQ* freq_double_to_bcd(double freq, BCDFREQ* bcdfreq, int is3200)
{
	memset((void*)bcdfreq,0,sizeof(BCDFREQ));
    double tmp, result;			
	  
    freq=freq*10000000.0; //get frequency in 0.1Hz
      	
    result = modf( freq/1000000000.0 , &tmp);
    freq=freq-1000000000.0*tmp;             
    bcdfreq->bcd_MHz[2] = (int) tmp;

    result = modf( freq/100000000.0 , &tmp);
    freq=freq-100000000.0*tmp;      
    bcdfreq->bcd_MHz[1] = (int) tmp;
       
    result = modf( freq/10000000.0 , &tmp);
    freq=freq-10000000.0*tmp;
    bcdfreq->bcd_MHz[0] = (int) tmp;

    result = modf( freq/1000000.0 , &tmp);
    freq=freq-1000000.0*tmp;
    bcdfreq->bcd_kHz[2] = (int) tmp;

    result = modf( freq/100000.0 , &tmp);
    freq=freq-100000.0*tmp;
    bcdfreq->bcd_kHz[1] = (int) tmp;

    result = modf( freq/10000.0 , &tmp);
    freq=freq-10000.0*tmp;
    bcdfreq->bcd_kHz[0] = (int) tmp;

    result = modf( freq/1000.0 , &tmp);
    freq=freq-1000.0*tmp;
    bcdfreq->bcd_Hz[2] = (int) tmp;

    result = modf( freq/100.0 , &tmp);
    freq=freq-100.0*tmp;
    bcdfreq->bcd_Hz[1] = (int) tmp;

    result = modf( freq/10.0 , &tmp);
    freq=freq-10.0*tmp;
    bcdfreq->bcd_Hz[0] = (int) tmp;
	
	result = modf( freq/1.0 , &tmp);
    freq=freq-1.0*tmp;
    bcdfreq->bcd_pHz[2] = (int) tmp;

	if(is3200 != 1){
	debug("freq_double_to_bcd:\n\t100MHz: 0X%x\n\t10MHz:  0X%x\n\t1MHz:   0X%x"
							 "\n\t100kHz: 0X%x\n\t10kHz:  0X%x\n\t1kHz:   0X%x"
							 "\n\t100Hz:  0X%x\n\t10Hz:   0X%x\n\t1Hz:    0X%x\n\t0.1Hz:  0X%x\n", 
							 bcdfreq->bcd_MHz[2],bcdfreq->bcd_MHz[1],bcdfreq->bcd_MHz[0],
							 bcdfreq->bcd_kHz[2],bcdfreq->bcd_kHz[1],bcdfreq->bcd_kHz[0],
							 bcdfreq->bcd_Hz[2],bcdfreq->bcd_Hz[1],bcdfreq->bcd_Hz[0],bcdfreq->bcd_pHz[2]);
	}
	else{
	debug("freq_double_to_bcd:\n\t1GHz:   0X%x"
							 "\n\t100MHz: 0X%x\n\t10MHz:  0X%x\n\t1MHz:   0X%x"
							 "\n\t100kHz: 0X%x\n\t10kHz:  0X%x\n\t1kHz:   0X%x"
							 "\n\t100Hz:  0X%x\n\t10Hz:   0X%x\n\t1Hz:    0X%x\n", 
							 bcdfreq->bcd_MHz[2],bcdfreq->bcd_MHz[1],bcdfreq->bcd_MHz[0],
							 bcdfreq->bcd_kHz[2],bcdfreq->bcd_kHz[1],bcdfreq->bcd_kHz[0],
							 bcdfreq->bcd_Hz[2],bcdfreq->bcd_Hz[1],bcdfreq->bcd_Hz[0],bcdfreq->bcd_pHz[2]);
	}
 
    return  bcdfreq;
}
 
SPINCORE_API char* spinpts_get_error()
{
	memset((void*)spinpts_err_buf,'\0',ERROR_STR_SIZE);
	
	switch(spinpts_err)
	{
		case PHASE_INVALID:
			strncpy(spinpts_err_buf,"Invalid Phase.",ERROR_STR_SIZE);
			break;
		case FREQ_ORANGE:
			strncpy(spinpts_err_buf,"Frequency out of range.",ERROR_STR_SIZE);
			break;
		case DWRITE_FAIL:
			strncpy(spinpts_err_buf,"Error writing to PTS.",ERROR_STR_SIZE);
			break;
		case DEVICE_OPEN_FAIL:
			strncpy(spinpts_err_buf,"Error opening PTS.",ERROR_STR_SIZE);
			break;
		case NO_DEVICE_FOUND:
			strncpy(spinpts_err_buf,"No PTS Found.",ERROR_STR_SIZE);
			break;
		case 0:
			strncpy(spinpts_err_buf,"PTS Operation Successful.",ERROR_STR_SIZE);
			break;
		default:
			strncpy(spinpts_err_buf,"Unknown error.",ERROR_STR_SIZE);
			break;
	};
	
	return spinpts_err_buf;
}

SPINCORE_API char* spinpts_get_version()
{
            return spinpts_version;
}
/*
 *
 *  End ofSpinPTS Functions
 *
 */
