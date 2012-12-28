/**
 * \file if.c
 * \brief Functions for interfacing with the RadioProcessor board.
 *
 * This page describes only the functions which are specific to the RadioProcessor
 * product. For a complete listing of spinapi functions, please see the
 * \link spinapi.h spinapi.h \endlink documentation.
 */

/* Copyright (c) 2009-2010 SpinCore Technologies, Inc.
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
#include <string.h>
#include <math.h>
#include <time.h>

#include "spinapi.h"
#include "if.h"
#include "caps.h"
#include "util.h"
#include "fid.h"
#include "usb.h"
#include "fftw/fftw.h"

extern char *noerr;

extern BOARD_INFO board[];
extern int cur_board;

extern double pow232;
extern int cur_dds;
extern double last_rounded_value;
extern int num_instructions;

static int set_shape_period (double period, int addr);

//Declare global variables used for AWG
static double shape_list[7]; //stores the length (in nanoseconds) for each use of shape.
static double shape_list1[7]; // stores the length (in nanoseconds) for each use of shape for the second DDS-II channel
static int shape_list_offset; //an index for shape_list[]
static int shape_list_offset1; //an index for shape_list1[] (used for second DDS-II channel)
int shape_period_array[2][7]; //stores the results of set_shape_period(...) to be written to DDS-II boards after pb_stop_programming() is called.


// 419 taps
// 18 bit coefficients
// cutoff: 1/8 of nyquist (so must use decimation factor of 8 or less after fir)
//
static int fir_default2[] = {
  8122, -846, -1110, -1484, -1865, -2141, -2211, -2002, -1494, -716, 240,
  1244, 2132,
  2744, 2944, 2655, 1872, 675, -780, -2272, -3558, -4397, -4605, -4079, -2836,
  -1005,
  1162, 3347, 5181, 6330, 6527, 5655, 3733, 1007, -2184, -5364, -7985, -9571,
  -9744,
  -8328, -5379, -1211, 3631, 8433, 12406, 14794, 15011, 12734, 7996, 1216,
  -6807, -14975,
  -21993, -26532, -27397, -23697, -14979, -1330, 16598, 37622, 60130, 82242,
  102009,
  117625, 127627, 131071, 127627, 117625, 102009, 82242, 60130, 37622, 16598,
  -1330,
  -14979, -23697, -27397, -26532, -21993, -14975, -6807, 1216, 7996, 12734,
  15011,
  14794, 12406, 8433, 3631, -1211, -5379, -8328, -9744, -9571, -7985, -5364,
  -2184,
  1007, 3733, 5655, 6527, 6330, 5181, 3347, 1162, -1005, -2836, -4079, -4605,
  -4397,
  -3558, -2272, -780, 675, 1872, 2655, 2944, 2744, 2132, 1244, 240, -716,
  -1494, -2002,
  -2211, -2141, -1865, -1484, -1110, -846, 8122
};

 /**
 * \internal
 * Write a 32 bit word to the extended register given by address.
 *
 */
void
reg_write (unsigned int address, unsigned int data)
{
  if (board[cur_board].is_usb)
    {
      usb_write_reg (address, data);
    }
  else
    {
      pb_outw (EXT_ADDRESS, address);
      pb_outw (EXT_DATA, data);
      pb_outw (EXT_ADDRESS, 0);
    }
}

/**
 * \internal
 * Read a 32 bit word from the extended register given by address.
 *
 */
unsigned int
reg_read (unsigned int address)
{
  unsigned int ret;

  if (board[cur_board].is_usb)
    {
      debug("Using usb_read_reg.");
      usb_read_reg (address, &ret);
    }
  else
    {
      pb_outw (EXT_ADDRESS, address);
      ret = pb_inw (EXT_DATA);
      pb_outw (EXT_ADDRESS, 0);
    }
  return ret;
}

/**
 * \internal
 * 
 * \bank Current choices are BANK_DATARAM (for the acquisition memory) and BANK_DDSRAM (for the shape waveforms)
 * \start_addr the starting address to write to. Note that this is the offset IN TERMS OF THE MEMORY WIDTH, and
 * not the offset in bytes. For example for the dataram memory with a width=8 bytes, address 0 is bytes 0-7, address
 * 1 is bytes 8-15, etc.
 * \param length (in bytes) of data to write to a RAM bank
 * \param data A buffer of the length you wish to write
 */

int
ram_write (unsigned int bank, unsigned int start_addr, unsigned int len,
	   char *data)
{
  int i;

  spinerr = noerr;

  int data_word;
  int write_flag = 0x0100;

  if (board[cur_board].is_usb)
    {
      return usb_write_ram (bank, start_addr, len, data);
    }
  else
    {
      if (bank == BANK_DDSRAM)
	{
	  debug ("Writing RAM with PCI method.");
	  // For each byte, write the data. Then set the write flag
	  for (i = 0; i < len; i++)
	    {
	      data_word = 0x0FF & ((int) data[i]);
	      reg_write (0x17, data_word);
	      reg_write (0x17, data_word | write_flag);
	      reg_write (0x17, data_word);
	    }
	  return 0;
	}
      else
	{
	  // Note: see pb_get_data() for how to read the acquisition memory on PCI boards
	  spinerr =
	    "Internal error: writing to RAMS other than BANK_DDSRAM not supported on PCI";
	  debug ("%s", spinerr);
	  return -1;
	}

    }

}


/**
 * \internal
 * Program a dds register using the extended register method. (this is how to control the dds if
 * dds_prog_method=DDS_PROG_EXTREG)
 * \param addr which register to program
 * \param cur_board which board to program
 * \param freq_word Frequency word for the "normal" speed DDS outputs (sin, cos, internal)
 * \param freq_word2 Frequency word for the "Fast" speed DDS, (DDS that feeds the DAC)
 */
int
dds_freq_extreg (int cur_board, int addr, int freq_word, int freq_word2)
{
  int control_word;

  reg_write (REG_DDS_DATA, freq_word);
  // REG_DDS_DATA2 writes the frequency word to the DDS unit which drives the DAC. In some
  // designs, that DDS runs at a speed FASTER than the other DDS units. Thus
  // it needs a seperate frequency word to ensure that the frequency it outputs
  // is the same as the slower DDS. See board.dds_clock_mult for how much faster
  // it runs
  reg_write (REG_DDS_DATA2, freq_word2);

  // put the address on the registers
  if (board[cur_board].custom_design == 4
      || board[cur_board].custom_design == 5)
    {
      debug("Using custom design %d DDS Controller bit order.",
	 board[cur_board].custom_design);
      control_word = DDS_WRITE_SEL | ((0x03FF & addr) << 8);
    }
  else
    {
      control_word = DDS_WRITE_SEL | ((0x0FF & addr) << 14);
    }

  reg_write (REG_DDS_CONTROL, control_word);
  // do the actual write
  reg_write (REG_DDS_CONTROL, control_word | DDS_FREQ_WE);
  reg_write (REG_DDS_CONTROL, control_word);
  // reset the control 
  reg_write (REG_DDS_CONTROL, DDS_RUN);	// in modern designs, DDS_RUN is not connected to anything and the DDS cores run all the time, but need this here for compatibilities sake

  return 0;
}

/**
 *\internal
 *\param cur_board which board to program
 *\param phase_bank
 *\param addr which register to program
 */
int
dds_phase_extreg (int cur_board, int phase_bank, int addr, int phase_word)
{
  int control_word = DDS_WRITE_SEL;
  int we = 0;

  switch (phase_bank)
    {

    case SIN_PHASE_REGS:
      control_word |= (addr << 10);
      we = DDS_TX_PHASE_WE;	// the sin dds output is called tx internally
      break;
    case COS_PHASE_REGS:
      control_word |= (addr << 6);
      we = DDS_RX_PHASE_WE;	// the cos dds output is called rx internally
      break;
    case TX_PHASE_REGS:
      control_word |= (addr << 19);
      we = DDS_REF_PHASE_WE;	// the tx dds output is called ref internally
      break;

    default:

      debug ("invalid phase bank");
      break;

    }

  reg_write (REG_DDS_DATA, phase_word);
  reg_write (REG_DDS_DATA2, phase_word);	// "Fast" DDS uses same phase info as other DDSes

  reg_write (REG_DDS_CONTROL, control_word);
  reg_write (REG_DDS_CONTROL, control_word | we);
  reg_write (REG_DDS_CONTROL, control_word);
  reg_write (REG_DDS_CONTROL, DDS_RUN);

  return 0;
}

/**
 * \internal
 * Find the most significant bit that is set in the given number.
 */
int
num_bits (int num)
{
  int i;
  int test = 1;
  int last_one = 0;

  for (i = 0; i < 32; i++)
    {
      if (num & test)
	{
	  last_one = i;
	}
      test = test << 1;
    }

  return last_one + 1;
}

/**
 * \internal
 * This function generates one period of a sine wave, and stores it in the given array.
 * \param shape_data array of 1024 floats, which will form the array
 */
static void
shape_make_sin (float *shape_data)
{
  int i;

  double pi = 3.1415926;

  for (i = 0; i < 1024; i++)
    {
      shape_data[i] = sin (2.0 * pi * ((float) i / 1024.0));
    }
}



////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//// Public SPINAPI functions for accessing the RadioProcessor board
////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

SPINCORE_API int
pb_set_defaults (void)
{
  spinerr = noerr;
  int i;


  if (board[cur_board].is_radioprocessor)
    {
      debug ("setting defaults for RadioProcessor");

      int adc_control = 3;
      int dac_control = 0;
      pb_set_radio_hw (adc_control, dac_control);

      reg_write (REG_CONTROL, 0);

      pb_set_scan_segments (1);

      if (board[cur_board].supports_dds_shape)
	{
	  debug ("setting shape defaults");
	  for (i = 0; i < board[cur_board].num_shape; i++)
	    {
	      shape_list[i] = 0.0;
	    }
	  shape_list_offset = 0;

	  // Make sure DDS unit has a sinewave loaded, and amp register 0 has a value of 1.0
	  // This will ensure the board works properly with shape-unaware code
	  float dds_data[1024];
	  shape_make_sin (dds_data);
	  pb_dds_load (dds_data, DEVICE_DDS);

	  pb_set_amp (1.0, 0);
	}

      return 0;
    }
  else
    {
      spinerr =
	"This function is for RadioProcessor only and does nothing on your board";
      debug ("%s", spinerr);
      return -1;
    }

}

SPINCORE_API int
pb_set_shape_defaults (void)
{
	int i;
    if (board[cur_board].firmware_id == 0xe01 || board[cur_board].firmware_id == 0xe02 || board[cur_board].firmware_id == 0x0E03 || board[cur_board].firmware_id == 0x0C13)
	{
		debug ("setting shape defaults");
		for (i = 0; i < board[cur_board].num_shape; i++)
	    {
			shape_list[i] = 0.0;
			shape_list1[i] = 0.0;
	    }
		shape_list_offset = 0;
		shape_list_offset1 = 0;
	}
	else
    {
		spinerr =
		  "This function is for DDS-II boards and DDS-I 12-19 only and does nothing on your board";
		debug ("%s", spinerr);
		return -1;
    }

    return 0;
}

SPINCORE_API int
pb_set_num_points (int num_points)
{
  spinerr = noerr;

  debug ("using %d points", num_points);

  if (num_points > board[cur_board].num_points || num_points < 0)
    {
      spinerr = "Number of points out of range";
      debug ("%s", spinerr);
      return -1;
    }

  reg_write (REG_SAMPLE_NUM, num_points);

  return 0;
}

SPINCORE_API int
pb_set_scan_segments (int num_segments)
{
  spinerr = noerr;

  debug ("setting number of segments to %d",
	 num_segments);

  if (!board[cur_board].supports_scan_segments)
    {
      spinerr = "Your firmware revision does not support this feature";
      debug ("%s", spinerr);
      return -1;
    }

  if (num_segments < 1 || num_segments > 65535)
    {
      spinerr = "Number of segments out of range";
      debug ("%s", spinerr);
      return -1;
    }

  // reset the internal segment counter (reset is bit 16)
  reg_write (REG_SCAN_SEGMENTS, 0x010000 | (num_segments - 1));
  // then write number of segments without the reset
  reg_write (REG_SCAN_SEGMENTS, (num_segments - 1));

  return 0;
}

SPINCORE_API int
pb_scan_count (int reset)
{
  spinerr = noerr;

  if (!board[cur_board].supports_scan_count)
    {
      spinerr = "Your firmware revision does not support this feature";
      debug ("%s", spinerr);
      return -1;
    }

  if (reset)
    {
      reg_write (REG_SCAN_COUNT, 0xFF);
      reg_write (REG_SCAN_COUNT, 0x00);
      return 0;
    }
  else
    {
      return reg_read (REG_SCAN_COUNT);
    }
}

SPINCORE_API int
pb_setup_filters (double spectral_width, int scan_repetitions, int cmd)
{
  spinerr = noerr;

  debug ("spectral_width %f, repetitions %d",
	 spectral_width, scan_repetitions);

  int *coef;

  int bypass_fir = cmd & BYPASS_FIR;
  int use_narrow_bw = cmd & NARROW_BW;

  int fir_dec_amount;
  if (bypass_fir)
    {
      fir_dec_amount = 1;
      pb_set_radio_control (BYPASS_FIR);
      debug ("bypassing FIR filter");
    }
  else
    {
      fir_dec_amount = 8;
      pb_unset_radio_control (BYPASS_FIR);
    }

  int cic_stages;		// Number of stages in cic filter
  if (use_narrow_bw)
    {
      cic_stages = 3;
      debug ("Using narrow CIC bandwidth option");
    }
  else
    {
      cic_stages = 1;
    }

  // Set CIC decimation amount based on the FIR dec. amount to achieve desired SW
  int cic_dec_amount = round(board[cur_board].clock * 1000.0 / (spectral_width * (double) fir_dec_amount));	// x 1000 because clock is internally stored as GHz
  debug ("cic_dec_amount %d", cic_dec_amount);

  // filter parameters        
  int fir_shift_amount;		// Will hold amount to shift right the output of the fir filter
  int cic_shift_amount;		// Will hold amount to shift right the output of the cic filter

  int cic_m = 1;		// M value for cic filter

  // Calculate the maximum bit growth due to averaging 
  int average_bit_growth =
    (int) (ceil (log ((double) scan_repetitions) / log (2.0)));
  debug ("average_bit_growth %d", average_bit_growth);

  // Figure out what is the worst case bit growth inside the CIC filter
  int cic_bit_growth =
    (int) (ceil
	   ((double) cic_stages * log ((double) (cic_m * cic_dec_amount)) /
	    log (2.0)));
  debug ("cic bit growth is %d", cic_bit_growth);
  //CIC has 28 bit input, 35 bit output
  cic_shift_amount = cic_bit_growth + 28 - 35;
  // If bypassing the FIR filter, need to account for bit growth of averaging here
  if (bypass_fir)
    {
      debug
	("taking care of average bit growth after CIC filter");
      cic_shift_amount += average_bit_growth;
    }
  if (cic_shift_amount < 0)
    {
      cic_shift_amount = 0;
    }
  debug ("cic_shift_amount %d", cic_shift_amount);

  // use builtin fir, which has a bit growth of 21 and 131 taps
  //coef = fir_default1;
  //int fir_bit_growth = 22;
  int num_taps = 131;
  int fir_bit_growth = 21;
  coef = fir_default2;

  // FIR has 35 bit input, 32 bit output
  fir_shift_amount = 35 + fir_bit_growth - 32;
  if (!bypass_fir)
    {
      debug
	("taking care of average bit growth after FIR filter");
      fir_shift_amount += average_bit_growth;
    }
  if (fir_shift_amount < 0)
    {
      fir_shift_amount = 0;
    }
  debug ("fir_shift_amount %d", fir_shift_amount);

  // The FIR filter requires num_taps/2 + 5 clock cycles to process each
  // point of data. Because of this, the CIC filter must decimate by at least
  // the much or the FIR filter will not be able to keep up.
  int min_dec_amount = num_taps / 2 + 5;
  
  if (cic_dec_amount < min_dec_amount && !bypass_fir)
    {
      debug
	("limiting cic_dec_amount to a minimum of %d (was %d)",
	 min_dec_amount, cic_dec_amount);
      cic_dec_amount = min_dec_amount;
    }
  // 8 is the minimum decimation no matter what
  if (cic_dec_amount < 8)
    {
      debug ("limiting cic_dec_amount to 8 (was %d)",
	     cic_dec_amount);
      cic_dec_amount = 8;
    }

  // Pass the actual parameters to the board  
  if (pb_setup_cic (cic_dec_amount, cic_shift_amount, cic_m, cic_stages) < 0)
    {
      return -1;
    }
  if (pb_setup_fir (num_taps, coef, fir_shift_amount, fir_dec_amount) < 0)
    {
      return -1;
    }

  return fir_dec_amount * cic_dec_amount;
}

SPINCORE_API int
pb_inst_radio (int freq, int cos_phase, int sin_phase, int tx_phase,
	       int tx_enable, int phase_reset, int trigger_scan, int flags,
	       int inst, int inst_data, double length)
{
  spinerr = noerr;
  int flag_word = 0;
  
  if(board[cur_board].firmware_id == 0x0C13) 
  {
	return pb_inst_dds2(freq, tx_phase, 0, tx_enable, phase_reset,0,0,0,0,0, flags, inst, inst_data, length);
  }

  if (freq >= board[cur_board].num_freq0 || freq < 0)
    {
      spinerr = "Frequency register out of range";
      debug ("%s", spinerr);
      return -1;
    }

  if (board[cur_board].acquisition_disabled == 1)
    {
      debug("Aquisition not supported, cos and sin phase registers ignored", spinerr);
    }
  else
    {
      if (cos_phase >= board[cur_board].num_phase0 || cos_phase < 0)
	{
	  spinerr = "Cos phase register out of range";
	  debug ("%s", spinerr);
	  return -1;
	}

      if (sin_phase >= board[cur_board].num_phase1 || sin_phase < 0)
	{
	  spinerr = "Sin phase register out of range";
	  debug ("%s", spinerr);
	  return -1;
	}
    }

  if (tx_phase >= board[cur_board].num_phase2 || tx_phase < 0)
    {
      spinerr = "TX phase register out of range";
      debug ("%s", spinerr);
      return -1;
    }

  if (board[cur_board].supports_dds_shape)
    {
      debug ("passing on values to pb_inst_radio_shape");
      int amp = 0;
      // Note: this will only work correctly if a value of 1.0 is written into amplitude register 0 (which is the default, and is taken care of by pb_set_defaults())
      return pb_inst_radio_shape (freq, cos_phase, sin_phase, tx_phase,
				  tx_enable, phase_reset, trigger_scan, 0,
				  amp, flags, inst, inst_data, length);
    }

  if (board[cur_board].acquisition_disabled == 1 && trigger_scan == 1)
    {
      spinerr = "Your version of the RadioProcessor does not support data acquisition.";
      debug ("%s", spinerr);
      return -1;
    }

  flag_word |= (0x03 & sin_phase) << 22;	// 2 bits
  flag_word |= (0x03 & cos_phase) << 20;	// 2 bits
  flag_word |= (0x7F & tx_phase) << 13;	// 7 bits
  flag_word |= (0x01 & tx_enable) << 12;	// 1 bit
  flag_word |= (0x0F & freq) << 8;	// 4 bits
  flag_word |= (0x01 & trigger_scan) << 7;	// 1 bit
  flag_word |= (0x01 & phase_reset) << 6;	// 1 bit

  flag_word |= (0x3F & flags);

  debug ("using standard RadioProcessor flag_word partitioning scheme(no shape support)");
  debug ("inst=%d, inst_data=%d,length=%f", inst, inst_data,length);

  return pb_inst_pbonly (flag_word, inst, inst_data, length);
}

SPINCORE_API int
pb_inst_radio_shape (int freq, int cos_phase, int sin_phase, int tx_phase,
		     int tx_enable, int phase_reset, int trigger_scan,
		     int use_shape, int amp, int flags, int inst,
		     int inst_data, double length)
{		 
  if(board[cur_board].firmware_id == 0x0C13)
  {
     return pb_inst_dds2_shape(freq, tx_phase, amp, use_shape, tx_enable, phase_reset,0,0,0,0,0,0, flags, inst, inst_data, length);
  }
  spinerr = noerr;
  unsigned int flag_word = 0;

  int shape_period = -1;
  int i;

  debug
    ("freq=%d, cos_phase=%d, sin_phase=%d, tx_phase=%d, tx_enable=%d, phase_reset=%d, trigger_scan=%d, use_shape=%d, amp=%d",
     freq, cos_phase, sin_phase, tx_phase, tx_enable, phase_reset,
     trigger_scan, use_shape, amp);
  debug ("flag=0x%x, inst=%d, inst_data=%d, length=%f",
	 flags, inst, inst_data, length);

  if (!board[cur_board].supports_dds_shape)
    {
      spinerr = "Board does not support DDS shape capabilities";
      debug ("%s", spinerr);
      return -1;
    }

  if (board[cur_board].acquisition_disabled == 1 && trigger_scan == 1)
    {
      spinerr =
	"Your version of the RadioProcessor does not support data acquisition.";
      debug ("%s", spinerr);
      return -1;
    }


  if (amp >= board[cur_board].num_amp || amp < 0)
    {
      spinerr = "Amplitude register out of range";
      debug ("%s", spinerr);
      return -1;
    }

  if (use_shape)
    {
      // check if this shape period has already been programmed
      for (i = 0; i < shape_list_offset; i++)
	{
	  if (shape_list[i] == length)
	    {
	      shape_period = i;
	      debug ("using shape register %d",
		     shape_period);
	      break;
	    }
	}
      // if not, program it
      if (shape_period == -1)
	{
	  // if there is room for another shape period
	  if (shape_list_offset < board[cur_board].num_shape)
	    {
	      debug
		("adding shape period %f to register %d",
		 length, shape_list_offset);
	      set_shape_period (length, shape_list_offset);
	      shape_period = shape_list_offset;
	      shape_list[shape_list_offset] = length;
	      shape_list_offset++;
	    }
	  else
	    {
	      spinerr = "No more shape period registers available";
	      debug ("%s", spinerr);
	      return -1;
	    }
	}

    }
  else
    {
      // when shape_period is the maximum value + 1, this indicates that we are not using a shape at all
      // for example, if there are 7 shape period registers
      // shape_period=0-6 are some period
      // and shape_period=7 indicates no shape at all
      // num_shape must always fit the form 2^x-1
      debug ("bypassing shape");
      shape_period = board[cur_board].num_shape;
    }


  if (board[cur_board].custom_design == 1)
    {				//firmware revision 10-10, in which some register select lines have been converted to TTL output bits and re-arranged
      flag_word |= (0x03 & sin_phase) << 22;	// 2 bits
      flag_word |= (0x03 & cos_phase) << 20;	// 2 bits
      flag_word |= (0x01 & tx_phase) << 16;	// 1 bit
      flag_word |= (0x01 & tx_enable) << 15;	// 1 bit
      flag_word |= (0x03 & freq) << 11;	// 2 bits
      flag_word |= (0x01 & trigger_scan) << 10;	// 1 bit
      flag_word |= (0x01 & phase_reset) << 9;	// 1 bit
      flag_word |= (0x07 & shape_period) << 6;	// 3 bits
      flag_word |= (0x03 & amp) << 4;	// 2 bits
      flag_word |= (0x1C0 & flags) << 11;	//upper 3 bits
      flag_word |= (0x30 & flags) << 9;	//middle 2 bits
      flag_word |= (0x0F & flags);	// bottom 4 bits

      debug
	("using TOPSPIN flag_word partitioning scheme");
    }
  else if (board[cur_board].custom_design == 2)
    {				//firmware revision 12-6, in which some register select lines have been converted to TTL output bits and re-arranged
      flag_word |= (0x03 & sin_phase) << 22;	// 2 bits
      flag_word |= (0x03 & cos_phase) << 20;	// 2 bits
      flag_word |= (0x03 & tx_phase) << 18;	// 2 bits
      flag_word |= (0x01 & tx_enable) << 15;	// 1 bit
      flag_word |= (0x03 & freq) << 16;	// 2 bits
      flag_word |= (0x01 & trigger_scan) << 8;	// 1 bit
      flag_word |= (0x01 & phase_reset) << 9;	// 1 bit
      flag_word |= (0x07 & shape_period) << 12;	// 3 bits
      flag_word |= (0x03 & amp) << 10;	// 2 bits
      flag_word |= (0xFF & flags);	// 8 bits

      debug
	("using PROGRESSION flag_word partitioning scheme");
    }
  else if (board[cur_board].custom_design == 3)
    {				//firmware revision 10-15, PBDDS-i-300. RP board in which aquisition has been disabled and there are 9 TTL outputs

      flag_word |= (0x0F & ((flags & 0x1E0) >> 5)) << 20;	// 4 bits 
      flag_word |= (0x0F & tx_phase) << 16;	// 4 bits
      flag_word |= (0x01 & tx_enable) << 15;	// 1 bit
      flag_word |= (0x0F & freq) << 11;	// 4 bits
      flag_word |= (0x01 & ((flags & 0x010) >> 4)) << 10;	// 1 bit
      flag_word |= (0x01 & phase_reset) << 9;	// 1 bit
      flag_word |= (0x07 & shape_period) << 6;	// 3 bits
      flag_word |= (0x03 & amp) << 4;	// 2 bits
      flag_word |= (0x0F & flags);	// 4 bits

      debug
	("using PBDDS-I-300 (10-15) flag_word partitioning scheme");
    }
  else if (board[cur_board].custom_design == 4)
    {
      flag_word |= (0x3FF & freq) << 14;	// 10 bits 
      flag_word |= (0x01 & tx_enable) << 13;	// 1 bit
      flag_word |= (0x07 & tx_phase) << 10;	// 3 bits
      flag_word |= (0x01 & phase_reset) << 9;	// 1 bit
      flag_word |= (0x07 & shape_period) << 6;	// 3 bits
      flag_word |= (0x03 & amp) << 4;	// 2 bits
      flag_word |= (0x0F & flags);	// 4 bits  (TTL outputs.)  

      debug
	("using PBDDS-I-300 (10-16 and 10-17) flag_word partitioning scheme");
    }
  else if (board[cur_board].custom_design == 5)
    {
      flag_word |= (0x3FF & amp) << 22;	// 10 bits 
      flag_word |= (0x3FF & freq) << 12;	// 10 bits 
      flag_word |= (0x01 & tx_enable) << 11;	// 1 bits
      flag_word |= (0x03 & tx_phase) << 8;	// 3 bit
      flag_word |= (0x01 & phase_reset) << 7;	// 1 bits
      flag_word |= (0x07 & shape_period) << 4;	// 3 bits
      flag_word |= (0x0F & flags);	// 4 bits  (TTL outputs.)  

      debug
	("using PBDDS-I-300 (10-19) flag_word partitioning scheme");
    }
  else if (board[cur_board].custom_design == 6)
    {
      //firmware revision 10-21, 12-15, 15-03 - CYCLOPS control added to hardware
      flag_word |= (0x03 & sin_phase) << 22;	// 2 bits
      flag_word |= (0x03 & cos_phase) << 20;	// 2 bits
      flag_word |= (0x03 & tx_phase) << 18;		// 2 bits
      flag_word |= (0x07 & freq) << 15;			// 3 bits
      flag_word |= (0x06) << 12;				// 3 bits (hardcode CYCLOPS control bits if pb_inst_radio_shape_cyclops(...) not used
	  flag_word |= (0x01 & tx_enable) << 11;	// 1 bit
      flag_word |= (0x01 & trigger_scan) << 10;	// 1 bit
      flag_word |= (0x01 & phase_reset) << 9;	// 1 bit
      flag_word |= (0x07 & shape_period) << 6;	// 3 bits
      flag_word |= (0x03 & amp) << 4;			// 2 bits
      flag_word |= (0x0F & flags);				// bottom 4 bits 

      debug
	("using RadioProcessor CYCLOPS flag_word partitioning scheme");
    }
   else if (board[cur_board].custom_design == 7)
    {
      //firmware revision 12-16 - PROGRESSION Flag output with CYCLOPS control added to hardware
	  flag_word |= (0x03) << 28;				// 3 bits (hardcode CYCLOPS control bits if pb_inst_radio_shape_cyclops(...) not used
	  flag_word |= (0x03 & sin_phase) << 26;	// 2 bits
      flag_word |= (0x03 & cos_phase) << 24;	// 2 bits
      flag_word |= (0x0F & tx_phase) << 20;		// 4 bits
      flag_word |= (0x0F & freq) << 16;			// 4 bits
	  flag_word |= (0x01 & tx_enable) << 15;	// 1 bit
      flag_word |= (0x07 & shape_period) << 12;	// 3 bits
      flag_word |= (0x03 & amp) << 10;			// 2 bits
	  flag_word |= (0x01 & phase_reset) << 9;	// 1 bit
      flag_word |= (0x01 & trigger_scan) << 8;	// 1 bit
      flag_word |= (0xFF & flags);				// bottom 8 bits 

      debug
	("using RadioProcessor CYCLOPS flag_word partitioning scheme with PROGRESSION Flag outputs");
    }
  else
    {
      //this is the standard r2 flag word partition
      flag_word |= (0x03 & sin_phase) << 22;	// 2 bits
      flag_word |= (0x03 & cos_phase) << 20;	// 2 bits
      flag_word |= (0x0F & tx_phase) << 16;	// 4 bits
      flag_word |= (0x01 & tx_enable) << 15;	// 1 bit
      flag_word |= (0x0F & freq) << 11;	// 4 bits
      flag_word |= (0x01 & trigger_scan) << 10;	// 1 bit
      flag_word |= (0x01 & phase_reset) << 9;	// 1 bit
      flag_word |= (0x07 & shape_period) << 6;	// 3 bits
      flag_word |= (0x03 & amp) << 4;	// 2 bits
      flag_word |= (0x0F & flags);	// 4 bits

      debug
	("using standard flag_word partitioning scheme");
      if (board[cur_board].custom_design != 0)
	debug
	  ("WARNING: You are using the wrong instruction(i.e. pb_inst_onerf, pb_inst_radio, etc), Please refer to your manual.");
    }

  debug ("shape :%d, amp: %d", shape_period, amp);

  return pb_inst_pbonly (flag_word, inst, inst_data, length);
}

SPINCORE_API int
pb_inst_radio_shape_cyclops (int freq, int cos_phase, int sin_phase, int tx_phase,
		     int tx_enable, int phase_reset, int trigger_scan, int use_shape, 
			 int amp, int real_add_sub, int imag_add_sub, int channel_swap,
			 int flags, int inst, int inst_data, double length)
{
  spinerr = noerr;
  unsigned int flag_word = 0;

  int shape_period = -1;
  int i;

  debug
    ("freq=%d, cos_phase=%d, sin_phase=%d, tx_phase=%d, tx_enable=%d, phase_reset=%d, trigger_scan=%d, use_shape=%d, amp=%d, real_add_sub=%d, imag_add_sub=%d, channel_swap=%d",
     freq, cos_phase, sin_phase, tx_phase, tx_enable, phase_reset,
     trigger_scan, use_shape, amp, real_add_sub, imag_add_sub, channel_swap);
  debug ("flag=0x%x, inst=%d, inst_data=%d, length=%f",
	 flags, inst, inst_data, length);

  if (!board[cur_board].supports_dds_shape)
    {
      spinerr = "Board does not support DDS shape capabilities";
      debug ("%s", spinerr);
      return -1;
    }

  if (board[cur_board].acquisition_disabled == 1 && trigger_scan == 1)
    {
      spinerr =
	"Your version of the RadioProcessor does not support data acquisition.";
      debug ("%s", spinerr);
      return -1;
    }


  if (amp >= board[cur_board].num_amp || amp < 0)
    {
      spinerr = "Amplitude register out of range";
      debug ("%s", spinerr);
      return -1;
    }

  if (use_shape)
    {
      // check if this shape period has already been programmed
      for (i = 0; i < shape_list_offset; i++)
	{
	  if (shape_list[i] == length)
	    {
	      shape_period = i;
	      debug ("using shape register %d",
		     shape_period);
	      break;
	    }
	}
      // if not, program it
      if (shape_period == -1)
	{
	  // if there is room for another shape period
	  if (shape_list_offset < board[cur_board].num_shape)
	    {
	      debug
		("adding shape period %f to register %d",
		 length, shape_list_offset);
	      set_shape_period (length, shape_list_offset);
	      shape_period = shape_list_offset;
	      shape_list[shape_list_offset] = length;
	      shape_list_offset++;
	    }
	  else
	    {
	      spinerr = "No more shape period registers available";
	      debug ("%s", spinerr);
	      return -1;
	    }
	}

    }
  else
    {
      // when shape_period is the maximum value + 1, this indicates that we are not using a shape at all
      // for example, if there are 7 shape period registers
      // shape_period=0-6 are some period
      // and shape_period=7 indicates no shape at all
      // num_shape must always fit the form 2^x-1
      debug ("bypassing shape");
      shape_period = board[cur_board].num_shape;
    }


  if (board[cur_board].supports_cyclops && board[cur_board].custom_design == 6)
    {				
	//firmware revision 10-21, 12-15, 15-03 - CYCLOPS control added to hardware
      flag_word |= (0x03 & sin_phase) << 22;	// 2 bits
      flag_word |= (0x03 & cos_phase) << 20;	// 2 bits
      flag_word |= (0x03 & tx_phase) << 18;		// 2 bits
      flag_word |= (0x07 & freq) << 15;			// 3 bits
      flag_word |= (0x01 & real_add_sub) << 14;	// 1 bit
	  flag_word |= (0x01 & imag_add_sub) << 13;	// 1 bit
	  flag_word |= (0x01 & channel_swap) << 12;	// 1 bit
	  flag_word |= (0x01 & tx_enable) << 11;	// 1 bit
      flag_word |= (0x01 & trigger_scan) << 10;	// 1 bit
      flag_word |= (0x01 & phase_reset) << 9;	// 1 bit
      flag_word |= (0x07 & shape_period) << 6;	// 3 bits
      flag_word |= (0x03 & amp) << 4;			// 2 bits
      flag_word |= (0x0F & flags);				// bottom 4 bits
	}
   else if (board[cur_board].supports_cyclops && board[cur_board].custom_design == 7)
    {
    //firmware revision 12-16 - PROGRESSION Flag output with CYCLOPS control added to hardware
	  flag_word |= (0x01 & channel_swap) << 30;	// 1 bit
	  flag_word |= (0x01 & imag_add_sub) << 29;	// 1 bit	  
      flag_word |= (0x01 & real_add_sub) << 28;	// 1 bit
	  flag_word |= (0x03 & sin_phase) << 26;	// 2 bits
      flag_word |= (0x03 & cos_phase) << 24;	// 2 bits
      flag_word |= (0x0F & tx_phase) << 20;		// 4 bits
      flag_word |= (0x0F & freq) << 16;			// 4 bits
	  flag_word |= (0x01 & tx_enable) << 15;	// 1 bit
      flag_word |= (0x07 & shape_period) << 12;	// 3 bits
      flag_word |= (0x03 & amp) << 10;			// 2 bits
	  flag_word |= (0x01 & phase_reset) << 9;	// 1 bit
      flag_word |= (0x01 & trigger_scan) << 8;	// 1 bit
      flag_word |= (0xFF & flags);				// bottom 8 bits
    }	  
  
  else
    {
      spinerr =
	"Your version of the RadioProcessor does not support cyclops in hardware.";
      debug ("%s", spinerr);
      return -1;
	}


  return pb_inst_pbonly (flag_word, inst, inst_data, length);
}

SPINCORE_API int
pb_inst_dds2_shape (int freq0, int phase0, int amp0, int use_shape0, int dds_en0, int phase_reset0,
	      int freq1, int phase1, int amp1, int use_shape1, int dds_en1, int phase_reset1,
	      int flags, int inst, int inst_data, double length)
{

  if (board[cur_board].firmware_id != 0xe01 && board[cur_board].firmware_id != 0xe02 && board[cur_board].firmware_id != 0x0E03 && board[cur_board].firmware_id != 0x0C13)
  {
      debug
	("Your current board does not support this function. Please check your manual.");
      return 0;

  }
  spinerr = noerr;

  if (freq0 >= board[cur_board].dds_nfreq[0] || freq0 < 0)
  {
      spinerr = "Frequency register select 0 out of range";
      debug ("%s", spinerr);
      return -1;
  }

  if (freq1 >= board[cur_board].dds_nfreq[1] || freq1 < 0)
  {
      spinerr = "Frequency register select 1 out of range";
      debug ("%s", spinerr);
      return -1;
  }

  if (phase0 >= board[cur_board].dds_nphase[0] || phase0 < 0)
  {
      spinerr = "TX phase register select 0 out of range";
      debug ("%s", spinerr);
      return -1;
  }

  if (phase1 >= board[cur_board].dds_nphase[1] || phase1 < 0)
  {
      spinerr = "TX phase register select 1 out of range";
      debug ("%s", spinerr);
      return -1;
  }

  if (amp0 >= board[cur_board].dds_namp[0] || amp0 < 0)
  {
      spinerr = "Amplitude register select 0 out of range";
      debug ("%s", spinerr);
      return -1;
  }

  if (amp1 >= board[cur_board].dds_namp[1] || amp1 < 0)
  {
      spinerr = "Amplitude register select 1 out of range";
      debug ("%s", spinerr);
      return -1;
  }

  debug ("using DDS 96 bit partitioning scheme");
  debug ("inst=%d, inst_data=%d,length=%f", inst, inst_data,length);


  int delay;
  double pb_clock, clock_period;

  spinerr = noerr;

  pb_clock = board[cur_board].clock * board[cur_board].pb_clock_mult;
  clock_period = 1.0 / pb_clock;

  delay = (int) rint ((length * pb_clock) - 3.0);	//(Assumes clock in GHz and length in ns)

  if (delay < 2)
    {
      spinerr = "Instruction delay is too small to work with your board";
      debug ("%s", spinerr);
      return -91;
    }

  if (inst == LOOP)
    {
      if (inst_data == 0)
	{
	  spinerr = "Number of loops must be 1 or more";
	  debug ("%s", spinerr);
	  return -1;
	}
      inst_data -= 1;
    }

  if (inst == LONG_DELAY)
    {
      if (inst_data == 0 || inst_data == 1)
	{
	  spinerr = "Number of repetitions must be 2 or more";
	  debug ("%s", spinerr);
	  return -1;
	}
      inst_data -= 2;
    }

  //setup shape stuff
  int shape_period = -1;
  int shape_period1 = -1;
  int i;
  
  if (use_shape0)
    {
	  cur_dds = 0;
      // check if this shape period has already been programmed
		for (i = 0; i < shape_list_offset; i++)
		{
			if (shape_list[i] == length)
			{
			  shape_period = i;
			  debug ("using shape register %d", shape_period);
			  break;
			}
		}
		// if not, program it
		if (shape_period == -1)
		{
		  // if there is room for another shape period
			if (shape_list_offset < board[cur_board].num_shape)
			{
			  debug ("adding shape period %f to register %d", length, shape_list_offset);
				  set_shape_period (length, shape_list_offset);
				  shape_period = shape_list_offset;
				  shape_list[shape_list_offset] = length;
				  shape_list_offset++;
			}
			else
			{
			  spinerr = "No more shape period registers available";
			  debug ("%s", spinerr);
			  return -1;
			}
		}

    }
	else
      debug ("bypassing shape0");

	if (use_shape1)
    {
		cur_dds = 1;
		// check if this shape period has already been programmed
		for (i = 0; i < shape_list_offset1; i++)
		{
			if (shape_list1[i] == length)
			{
			  shape_period1 = i;
			  debug ("using shape register %d",
				 shape_period1);
			  break;
			}
		}
		// if not, program it
		if (shape_period1 == -1)
		{
		  // if there is room for another shape period
			if (shape_list_offset1 < board[cur_board].num_shape)
			{
			  debug
				("adding shape period %f to register %d",
				 length, shape_list_offset1);
				  set_shape_period (length, shape_list_offset1);
				  shape_period1 = shape_list_offset1;
				  shape_list1[shape_list_offset1] = length;
				  shape_list_offset1++;
			}
			else
			{
			  spinerr = "No more shape period registers available";
			  debug ("%s", spinerr);
			  return -1;
			}
		}

    }
	else
      debug ("bypassing shape1");
	  
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
	  flag_word[1] |= (shape_period & 0x7) << 1; // 3 Shape Period Select Bits
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
    flag_word[1] |= (shape_period & 0x3) << 30;   // Lower 2 DDS1 Shape Period Select Bits
	
	flag_word[2] |= (shape_period & 0x4) >> 2;    // Upper DDS1 Shape Period Select Bit
    flag_word[2] |= (shape_period1 & 0x7) << 1;   // DDS2 Shape Period Select Bits
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
    flag_word[1] |= (shape_period & 0x7) << 2;
    flag_word[1] |= (shape_period1 & 0x7) << 5;
  }
	return pb_inst_direct(flag_word, inst, inst_data, delay);
}

SPINCORE_API int
pb_get_data (int num_points, int *real_data, int *imag_data)
{
  int i;
  int control;
  int tmp[2 * 16 * 1024];
  int board_points;
  int pos;

  spinerr = noerr;

  // This holds the number of points that can be stored on the board
  board_points = board[cur_board].num_points;

  if (num_points > board_points)
    {
      spinerr = "Too many points";
      debug ("%s (%d > %d)", spinerr, num_points, board[cur_board].num_points);
      return -1;
    }
  if (num_points < 1)
    {
      spinerr = "num_points must be > 0";
      debug ("%s", spinerr);
      return -1;
    }

  if (board[cur_board].is_usb)
    {
      
      pb_set_radio_control (0x02);	// turn on the PCI_READ bit

      char *read_buf;
      read_buf = malloc (num_points * 8);
      if (!read_buf)
	{
	  spinerr = "Internal error: can't allocate read buffer";
	  debug ("%s", spinerr);
	  return -1;
	}

      usb_read_ram (BANK_DATARAM, 0, num_points * 8, read_buf);
    
    for (i = 0; i < num_points * 8; i += 8)
	{
	  real_data[i / 8] = (0x0FF & read_buf[i]);
	  real_data[i / 8] |= (0x0FF & read_buf[i + 1]) << 8;
	  real_data[i / 8] |= (0x0FF & read_buf[i + 2]) << 16;
	  real_data[i / 8] |= (0x0FF & read_buf[i + 3]) << 24;

	  imag_data[i / 8] = (0x0FF & read_buf[i + 4]);
	  imag_data[i / 8] |= (0x0FF & read_buf[i + 5]) << 8;
	  imag_data[i / 8] |= (0x0FF & read_buf[i + 6]) << 16;
	  imag_data[i / 8] |= (0x0FF & read_buf[i + 7]) << 24;
	}

      free (read_buf);

      pb_unset_radio_control (0x02);	// turn off the PCI_READ bit

      return 0;
    }


  //debug("pb_get_data: mem address (start): %d", pb_inw(MEM_ADDRESS));

  control = reg_read (REG_CONTROL);
  // The PCI_READ control bit must be set to be able to read data from RAM
  reg_write (REG_CONTROL, control | PCI_READ);

  // this is a fix for the "wraparound" issue in the 10-4 firmware (and 10-5, 10-6)
  // the problem is that writing to the address register does not work (but reading does), so
  // there is no way to specify where to start reading in the RAM. so we have
  // to just read it all, and then shift appropriately based on reading the value
  // from the address register
#define F4_RSIZE (2*16*1024)	// 16k for each channel
  if (board[cur_board].has_wraparound_problem)
    {
      debug ("using wraparound fix");

      pos = pb_inw (MEM_ADDRESS);
      // read in all data. tmp will hold data in the correct order
      for (i = 0; i < F4_RSIZE; i++)
	{
	  while (pos >= F4_RSIZE)
	    {
	      pos -= F4_RSIZE;
	    }
	  tmp[pos] = pb_inw (MEM_DATA);
	  pos++;
	}
      for (i = 0; i < num_points; i++)
	{
	  real_data[i] = tmp[2 * i];
	  imag_data[i] = tmp[2 * i + 1];
	}
    }
  // Otherwise just read ram in the normal way
  else
    {
      // Reset memory address register
      pb_outw (MEM_ADDRESS, 0);

      //loop num_points times and read values from ram
      for (i = 0; i < num_points; i++)
	{
	  real_data[i] = pb_inw (MEM_DATA);
	  imag_data[i] = pb_inw (MEM_DATA);
	}
    }



  reg_write (REG_CONTROL, control);

  //debug("pb_get_data: mem address (end): %d", pb_inw(MEM_ADDRESS));

  return 0;
}

/** Deprecated legacy function.  Please use pb_write_ascii_verbose instead. */
SPINCORE_API int
pb_write_ascii (char *fname, int num_points, float SW, int *real_data,
		int *imag_data)
{
  debug
    ("Spectrometer Frequency will be written to output file as zero (use pb_write_ascii_verbose to avoid this).");

  int i;
  i =
    pb_write_ascii_verbose (fname, num_points, SW, 0.0, real_data, imag_data);
  return (i);
}

SPINCORE_API int
pb_write_ascii_verbose (char *fname, int num_points, float SW, float SF,
			int *real_data, int *imag_data)
{
  int i;

  spinerr = noerr;

  //try to open file
  FILE *fp = fopen (fname, "w");
  if (fp == NULL)
    {
      fprintf(stderr, "Error opening file %s.\n", fname);
      spinerr = "Couldnt open file";
      debug ("%s", spinerr);
      return -1;
    }

  //write file tag info
  fprintf (fp, "#SpinCore Technologies, Inc.\n#SF in MHz\n#SW in Hz\n\n");
  fprintf (fp, "[HEADER]\n");
  fprintf (fp, "@CLASS=	RADIOPROCESSOR\n");
  fprintf (fp, "@FIRMWARE= %d\n", board[cur_board].firmware_id);
  fprintf (fp, "@SPINAPI= %s\n", pb_get_version ());
  fprintf (fp, "@POINTS= %d\n", num_points);
  fprintf (fp, "@SF= %f\n", SF);
  fprintf (fp, "@SW= %f\n\n", SW);
  fprintf (fp, "[DATA]\n");

  for (i = 0; i < num_points; i++)
    {
      fprintf (fp, "%d\n", real_data[i]);
      fprintf (fp, "%d\n", imag_data[i]);
    }

  fclose (fp);

  return 0;
}

SPINCORE_API int
pb_write_jcamp (char *fname, int num_points, float SW, float SF,
		int *real_data, int *imag_data)
{


  const float points_per_line = 6.0f;
  int i;
  int sevenPoints;
  float tmp_delay;
  float tmp_time;
  float inc;
  float time_accumulator = 0.0f;


  int min_real = real_data[0];
  int min_imag = imag_data[0];

  int max_real = real_data[0];
  int max_imag = imag_data[0];

  for (i = 1; i < num_points; ++i)
    {
      if (min_real > real_data[i])
	min_real = real_data[i];
      if (max_real < real_data[i])
	max_real = real_data[i];

      if (min_imag > imag_data[i])
	min_imag = imag_data[i];
      if (max_imag < imag_data[i])
	max_imag = imag_data[i];
    }

  spinerr = noerr;

  //try to open file
  FILE *fp = fopen (fname, "w");
  if (fp == NULL)
    {
      fprintf(stderr, "Error opening file %s.\n", fname);
      spinerr = "Couldnt open file";
      debug ("pb_write_jcamp: %s", spinerr);
      return -1;
    }

  tmp_delay = (float) ((1 / SW) * 1000000);	//Time between points in seconds
  tmp_time = (float) ((1 / (SW)) * num_points);	// Acquisition Time in seconds
  inc = tmp_time * 1000000 / (float) (num_points - 1);

  fprintf (fp, "##TITLE= \n");
  fprintf (fp, "##JCAMP-DX= 5.00 $$Spinapi version %s\n", "pb_get_version()");
  fprintf (fp, "##DATA TYPE= NMR FID\n");
  fprintf (fp, "##DATA CLASS= NTUPLES\n");
  fprintf (fp, "##ORIGIN=\n");
  fprintf (fp, "##OWNER=\n");
  fprintf (fp, "##.OBSERVE FREQUENCY=%f\n", SF);
  fprintf (fp, "##.OBSERVE NUCLEUS=\n");
  fprintf (fp, "##.DELAY= (0,0)\n");	//these should be the pre-acquisition delays
  fprintf (fp, "##.ACQUISITION MODE= SIMULTANEOUS\n");
  fprintf (fp, "##.DIGITISER RES=14\n");
  fprintf (fp,
	   "##SPECTROMETER/DATA SYSTEM= SpinCore Technologies, Inc.  Radio Processor\n");
  fprintf (fp, "##NTUPLES= NMR FID\n");
  fprintf (fp,
	   "##VAR_NAME=	TIME,		FID/REAL,	FID/IMAG,	PAGE NUMBER\n");
  fprintf (fp,
	   "##SYMBOL=	X,		R,		I,		N\n");
  fprintf (fp,
	   "##VAR_TYPE=	INDEPENDENT,	DEPENDENT,	DEPENDENT,	PAGE\n");
  fprintf (fp,
	   "##VAR_FORM=	AFFN,		AFFN,		AFFN,		AFFN\n");
  fprintf (fp,
	   "##VAR_DIM=	%u,		%u,		%u,		2\n",
	   num_points, num_points, num_points);
  fprintf (fp,
	   "##UNITS=	SECONDS,	ARBITRARY UNITS,ARBITRARY UNITS,\n");
  fprintf (fp,
	   "##FIRST=	0.0,		%d,		%d,		1\n",
	   real_data[0], imag_data[0]);
  fprintf (fp, "##LAST=	 %f,		%d,		%d,		2\n",
	   tmp_time, real_data[num_points - 1], imag_data[num_points - 1]);
  fprintf (fp, "##MIN=     0.0,        %d,     %d,     1\n", min_real,
	   min_imag);
  fprintf (fp, "##MAX=      %f,        %d,     %d,     1\n", tmp_time,
	   max_real, max_imag);
  fprintf (fp,
	   "##FACTOR=	%.4E,		1,		1,		1\n",
	   inc);

  //start real data
  fprintf (fp, "##PAGE=		N=1\n");
  fprintf (fp, "##DATA TABLE=	(X++(R..R)), XYDATA $$Real data points	\n");
  //do  points on a line until finished real_data

  for (i = 0; i < num_points;)
    {

      fprintf (fp, "%d ", i);

      for (sevenPoints = 0;
	   sevenPoints < (int) points_per_line && i < num_points;
	   sevenPoints++)
	fprintf (fp, "%d ", real_data[i++]);

      time_accumulator += points_per_line * inc;
      fprintf (fp, "\n");

    }
  time_accumulator = 0.0f;

  //start imag data
  fprintf (fp, "##PAGE=		N=2\n");
  fprintf (fp,
	   "##DATA TABLE=	(X++(I..I)), XYDATA $$Imaginary data points\n");


  for (i = 0; i < num_points;)
    {

      fprintf (fp, "%d ", i);

      for (sevenPoints = 0;
	   sevenPoints < (int) points_per_line && i < num_points;
	   sevenPoints++)
	fprintf (fp, "%d ", imag_data[i++]);

      time_accumulator += points_per_line * inc;
      fprintf (fp, "\n");

    }

  fprintf (fp, "##END NTUPLES=	NMR FID\n");
  fprintf (fp, "##END=\n");

  fclose (fp);

  return 0;
}

SPINCORE_API int
pb_write_felix (char *fnameout, char *title_string, int num_points, float SW, float SF, 
                int *real_data, int *imag_data)
{

  spinerr = noerr;
  
  int NP = num_points;
  float mySW = SW / 1000.0;
  float spectrometer_freq = SF;

  // FELIX will crash if the spectrometer frequency is DC
  // we shift it ever so slightly to avoid this.
  if (spectrometer_freq == 0.0)
     spectrometer_freq = 0.0000000001;
  
  //Get time to put in header
  char buff_time[20];
  time_t rawtime;
  struct tm * timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buff_time,20,"%x %X",timeinfo);
  
  //Get SpinAPI version to put in header
  char *version = pb_get_version();
  
  //try to open fnameout for writing
  FILE *pFile = fopen (fnameout, "wb");

  //make sure fopen worked
  if (pFile == NULL)
    {
      fprintf(stderr, "Error opening file %s.\n", fnameout);
      spinerr = "Couldnt open output file";
      debug ("%s", spinerr);
      return -1;
    }

  //the following code was mostly cut and paste from spinwrap.dll source

  long paramsize = 32;
  long tmp_long;
  float tmp_fl;
  short int tmp_unit;
  bool tmp_bool;
  short int tmp_ft;
  short int tmp_dm;

	/*******************************************
	 * Marker
	 * First 32 bytes of file
	 *******************************************/
  char felixForWindows[32] = "Felix for Windows             \0";
  int index = 0;
  unsigned int UUINT = 0x01000000;

  for (index = 0; index < 32 - sizeof (unsigned int); index++)
    {
      if (felixForWindows[index] == 0x20 && index > 9)
	fputc (0x00, pFile);
      else
	fputc (felixForWindows[index], pFile);
    }
  //UINT 0x0100
  fwrite (&UUINT, sizeof (unsigned int), 1, pFile);


	/*********************************************
	 * parameters: 1024 bytes - Marker (32 bytes)
	 *********************************************/

  //Acqusition Parameters
  tmp_ft = FID_TYPE;		// Type of FID data
  fwrite (&tmp_ft, sizeof (short), 1, pFile);
  paramsize += sizeof (short);

  tmp_long = NP;		//Number of Points
  fwrite (&tmp_long, sizeof (long), 1, pFile);
  paramsize += sizeof (long);

  tmp_fl = mySW * (float) 1000;	// Spectral Width
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);

  tmp_fl = (float) (1 / (mySW * 1000)) * NP;	// Acquisition Time
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);

  tmp_fl = (float) spectrometer_freq;	// Spectrometer Frequency
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);

  //Processing Parameters
  tmp_long = DC;		// Drift Correction Points
  fwrite (&tmp_long, sizeof (long), 1, pFile);
  paramsize += sizeof (long);
  tmp_long = FOURIER_NUMBER;	// Fourier Number
  fwrite (&tmp_long, sizeof (long), 1, pFile);
  paramsize += sizeof (long);
  tmp_fl = LINE_BROAD;		// Line Broadening
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);
  tmp_fl = GAUSS_BROAD;		// Gaussian Broadening
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);

  //Display Parameters
  tmp_dm = DISP_MODE;		// Display Mode
  fwrite (&tmp_dm, sizeof (short), 1, pFile);
  paramsize += sizeof (short);
  tmp_fl = PH0;			// Zero order phase
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);
  tmp_fl = PH1;			// first order phase
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);
  tmp_fl = REF_LINE_FREQ;	//reference line frequency
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);
  // Set the reference point for the zero frequency line. If this is
  // set to half of the number of points in the FFT, the zero frequency
  // will be in the center
  tmp_long = num_points / 2;
  fwrite (&tmp_long, sizeof (long), 1, pFile);
  paramsize += sizeof (long);

  tmp_unit = FID_UNITS;		// FID units
  fwrite (&tmp_unit, sizeof (short), 1, pFile);
  paramsize += sizeof (short);
  tmp_unit = SPEC_UNITS;	// Spectrum Units
  fwrite (&tmp_unit, sizeof (short), 1, pFile);
  paramsize += sizeof (short);
  tmp_bool = AXIS;		// Axis on/off
  fwrite (&tmp_bool, sizeof (bool), 1, pFile);
  paramsize += sizeof (bool);

  putc (0x00, pFile);		// Extra blank character ?
  paramsize += 1;

  tmp_fl = SCALE_OFFSET;	// Scale offset
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);
  tmp_long = START_PLOT;	// Start of plot
  fwrite (&tmp_long, sizeof (long), 1, pFile);
  paramsize += sizeof (long);
  tmp_long = WIDTH_PLOT;	// Width of plot
  fwrite (&tmp_long, sizeof (long), 1, pFile);
  paramsize += sizeof (long);
  tmp_fl = VERT_OFFSET;		// Vertical offset
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);
  tmp_fl = VERT_SCALE;		// vertical scale
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);
  tmp_long = FID_START;		// FID start of plot
  fwrite (&tmp_long, sizeof (long), 1, pFile);
  paramsize += sizeof (long);
  tmp_long = NP;		// FID width of plot
  fwrite (&tmp_long, sizeof (long), 1, pFile);
  paramsize += sizeof (long);
  tmp_fl = FID_VERT_OFFSET;	// FID vertical offset
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);
  tmp_fl = FID_VERT_SCALE;	// FID vertical scale
  fwrite (&tmp_fl, sizeof (float), 1, pFile);
  paramsize += sizeof (float);

  tmp_long = 0x00000000;
  for (index = 0; index < 1024 - paramsize; index++)
    {
      fwrite (&tmp_long, 1, 1, pFile);
    }

	/**************************
	 * Title Block: 512 bytes *
	 **************************/
  /*
     int t = 00;
     int aa = 0;
     for (aa = 0; aa < 128; aa++)
     fwrite(&t, sizeof(t), 1, pFile);
   */
                            
  char buffer[512];
  sprintf (buffer,
	   "RadioProcessor\r\nSpinCore Technologies, Inc.\r\nwww.spincore.com\r\n\r\nTime = %s\r\nSpinAPI = %s\r\n%s",
	   buff_time,version,title_string);
  fwrite (buffer, sizeof (char), 512, pFile);

  
  int k;
  long temp;
  for (k = 0; k < NP; k++)
    {
      //get real value from ram file
      temp = real_data[k];
      //fscanf(pRam, "%ld", &temp);

      //write to felix file
      //if (k != 0)
      fwrite (&temp, sizeof (long), 1, pFile);


      //get imag value from ram file
      //fscanf(pRam, "%ld", &temp);
      temp = imag_data[k];

      //write to felix file
      //if (k != 0)
      fwrite (&temp, sizeof (long), 1, pFile);
    }

  fwrite (&temp, sizeof (long), 1, pFile);
  fwrite (&temp, sizeof (long), 1, pFile);

  //fwrite(&temp,sizeof(long),1,pFile);
  //fwrite(&temp,sizeof(long),1,pFile);


  //close the files
  if (pFile != NULL)
    fclose (pFile);


  return 0;
}

SPINCORE_API int
pb_setup_cic (int dec_amount, int shift_amount, int m, int stages)
{
  unsigned int word;
  unsigned int ret;

  spinerr = noerr;

  debug ("dec_amount=%d shift_amount=%d m=%d stages=%d",
	 dec_amount, shift_amount, m, stages);

  if (dec_amount > board[cur_board].cic_max_decim || dec_amount < 8)
    {
      spinerr = "dec_amount out of range";
      debug ("%s", spinerr);
      debug ("max_dec_amnt = %d",board[cur_board].cic_max_decim);
      return -1;
    }

  if (shift_amount > board[cur_board].cic_max_shift || shift_amount < 0)
    {
      spinerr = "shift_amount out of range";
      debug ("%s", spinerr);
      return -1;
    }

  if (m < 1 || m > 2)
    {
      spinerr = "m must be 1 or 2";
      debug ("%s", spinerr);
      return -1;
    }

  if (stages < 1 || stages > board[cur_board].cic_max_stages)
    {
      spinerr = "stages out of range";
      debug ("%s", spinerr);
      return -1;
    }

  // subtract 1 from stages and dec_amount, because hardware accepts values
  // in this way

  word =
    (stages - 1) + ((m - 1) << 12) + (shift_amount << 4) +
    ((dec_amount - 1) << 16);
  debug ("cic word is %x", word);

  reg_write (REG_CIC_CONTROL, word);
  
  if(board[cur_board].firmware_id == 0xC0F || board[cur_board].firmware_id == 0xF02 || 
     board[cur_board].firmware_id == 0xA16 || board[cur_board].firmware_id == 0xF03 ||
	 board[cur_board].firmware_id == 0xC10 ) {
     word = ((dec_amount - 1) >> 16);
     debug ("cic word2 is %x", word);
     reg_write (REG_CIC_CONTROL2, word);
  }

  ret = reg_read (REG_CIC_CONTROL);

  return 0;

}

SPINCORE_API int
pb_load_coef_file (int *coef, char *fname, int num_coefs)
{
  char line[256];
  char *endptr;
  int i;

  double *coef_float;
  double scale = 0.0;
  double largest_tap = 0.0;

  spinerr = noerr;

  debug ("fname=%s num_coefs=%d", fname, num_coefs);

  int coef_max;
  int coef_min;

  // Find the maximum and minimum values for the coefficients based
  // on the number of bits
  coef_max = 1;

  for (i = 1; i < board[cur_board].fir_coef_width; i++)
    {
      coef_max *= 2;
    }

  coef_min = -coef_max;
  coef_max = coef_max - 1;

  debug ("coef_max %d, coef_min %d", coef_max, coef_min);


  coef_float = (double *) malloc (num_coefs * sizeof (double));

  if (!coef_float)
    {
      spinerr = "Out of memory: couldnt allocate internal buffer";
      debug ("%s", spinerr);
      return -1;
    }

  FILE *f = fopen (fname, "r");

  if (!f)
    {
      spinerr = "error opening coefficent file";
      debug ("%s", spinerr);
      return -1;
    }

  // Read in all the coefficients as floating point
  for (i = 0; i < num_coefs; i++)
    {
      if (!fgets (line, 256, f))
	{
	  spinerr =
	    my_sprintf
	    ("Reached end of file before read in all coefficients (line %d)",
	     i + 1);
	  debug ("%s", spinerr);
	  return -1;
	}
	
      endptr = 0;
      coef_float[i] = strtod (line, &endptr);

      // if an error occurred converting number
      if (coef_float[i] == 0.0 && endptr == line)
	{
	  spinerr = my_sprintf ("Error reading coefficent at line %d", i + 1);
	  debug ("%s", spinerr);
	  return -1;
	}

      //printf ("tap: %f", coef_float[i]);

      // Record the absolute value of the largest tap        
      if (coef_float[i] > largest_tap)
	{
	  largest_tap = coef_float[i];
	}
      if (-coef_float[i] > largest_tap)
	{
	  largest_tap = -coef_float[i];
	}
    }

  fclose (f);			//This is very important.

  scale = (double) (coef_max + 1) / largest_tap;

  for (i = 0; i < num_coefs; i++)
    {

      coef[i] = (int) (coef_float[i] * scale);

      if (coef[i] > coef_max)
	{
	  coef[i] = coef_max;
	  debug ("rounding coef[%d] down", i);
	}
      if (coef[i] < coef_min)
	{
	  coef[i] = coef_min;
	  debug ("rounding coef[%d] up", i);
	}

    }

  free (coef_float);

  // Figure out the worst case bit growth by adding the absolute value
  // of all the coefficients together
  int sum = 0;

  for (i = 0; i < num_coefs; i++)
    {
      if (coef[i] > 0)
	{
	  sum += coef[i];
	}
      else
	{
	  sum -= coef[i];
	}
    }

  return num_bits (sum);
}

SPINCORE_API int
pb_setup_fir (int num_coefs, int *coef, int shift_amount, int dec_amount)
{
  int i;
  int num_coefs_stored;

  spinerr = noerr;

  debug ("num_coefs=%d shift_amount=%d dec_amount=%d",
	 num_coefs, shift_amount, dec_amount);

  if (num_coefs < 0 || num_coefs > board[cur_board].fir_max_taps)
    {
      spinerr = "Number of coefficients out or range";
      debug ("%s", spinerr);
      return -1;
    }

  if (shift_amount > board[cur_board].fir_max_shift || shift_amount < 0)
    {
      spinerr = "shift_amount out of range";
      debug ("%s", spinerr);
      return -1;
    }

  if (dec_amount > board[cur_board].fir_max_decim || dec_amount < 1)
    {
      spinerr = "dec_amount out of range";
      debug ("%s", spinerr);
      return -1;
    }

  reg_write (REG_FIR_NUM_TAPS, (num_coefs - 1));
  reg_write (REG_FIR_NUM_TAPS, FIR_RESET | (num_coefs - 1));
  reg_write (REG_FIR_NUM_TAPS, (num_coefs - 1));

  if( board[cur_board].firmware_id == 0xC0D ||  board[cur_board].firmware_id == 0xC0E )
  {
      num_coefs_stored = num_coefs;
  }
  else
  {
       if (num_coefs % 2 == 1)
       {
           num_coefs_stored = num_coefs / 2 + 1;
       }
       else
       {
           num_coefs_stored = num_coefs / 2;
       }
   }

  debug ("using num_coefs_stored = %d", num_coefs_stored);

  // Only need to store half the coefficients, since this is even symmetric,
  // the second half of the coefficients are redundant.
  for (i = 0; i < num_coefs_stored; i++)
    {
      // write the address
      reg_write (REG_FIR_COEF_ADDR, i);

      // write data
      reg_write (REG_FIR_COEF_DATA, coef[i]);

      reg_write (REG_FIR_COEF_ADDR, FIR_COEF_LOAD_EN | i);
      reg_write (REG_FIR_COEF_ADDR, i);

    }


// address 0 is the outermost coefficient, and the last address is the center
// tap. what the last address is depends on the number of taps.
// example filter: 8,-4,3,-4,8
//addr   data 
// 0  :  8
// 1  : -4
// 2  :  3

  // shift_amount and dec_amount are both 8 bit numbers
  reg_write (REG_FIR_CONTROL, (shift_amount << 8) + (dec_amount - 1));

  return 0;
}

SPINCORE_API int
pb_overflow (int reset, PB_OVERFLOW_STRUCT * of)
{
  int of_reg;
  int control;

  spinerr = noerr;

  if (reset)
    {
      control = reg_read (REG_CONTROL);
      reg_write (REG_CONTROL, control | OVERFLOW_RESET);
      reg_write (REG_CONTROL, control);
    }
  else
    {
      if (of)
	{
	  of_reg = reg_read (REG_OVERFLOW_COUNT);
	  of->fir = 0x0000FFFF & (of_reg >> 16);
	  of->cic = 0x0000FFFF & (of_reg);

	  of_reg = reg_read (REG_OVERFLOW2_COUNT);

	  of->average = 0x0000FFFF & (of_reg >> 16);
	  of->adc = 0x0000FFFF & (of_reg);

	  debug ("fir: %d, cic: %d, adc: %d average: %d",
		 of->fir, of->cic, of->adc, of->average);

	}
    }

  return 0;
}

SPINCORE_API int
pb_set_radio_hw (int adc_control, int dac_control)
{
  spinerr = noerr;

  reg_write (REG_ADC_CONTROL, adc_control);
  reg_write (REG_DAC_CONTROL, dac_control);

  return 0;
}

SPINCORE_API int
pb_set_radio_control (unsigned int control)
{
  unsigned int old_control;

  spinerr = noerr;

  old_control = reg_read (REG_CONTROL);

  reg_write (REG_CONTROL, old_control | control);

  return 0;
}

SPINCORE_API int
pb_unset_radio_control (unsigned int control)
{
  unsigned int old_control;

  old_control = reg_read (REG_CONTROL);

  reg_write (REG_CONTROL, old_control & (~control));

  return 0;
}

/**
 * \internal
 * This function sets the given shape period register with the given value. It
 * is used by pb_inst_radio_shape() to manage the values of the shape registers.
 * For now, we hide the existance of these registers from the user.
 * \param period in ns
 * \param addr Address of shape period register to write to
 */
static int
set_shape_period (double period, int addr)
{
  spinerr = noerr;

  if (!board[cur_board].supports_dds_shape)
    {
      spinerr = "DDS Shape capabilities not supported on this board";
      debug ("%s", spinerr);
      return -1;
    }

  // convert period in ns to frequency in MHz
  double freq = (1.0 / period) * 1e3;

  if (addr >= board[cur_board].num_shape)
    {
      spinerr = "Shape period registers full";
      debug ("%s", spinerr);
      return -1;
    }

  debug ("addr=%d, period=%f, freq=%f", addr, period,
	 freq);

  double dds_clock;
  double dds_clock_mult;
  unsigned int freq_word2;

  dds_clock = board[cur_board].clock; //PB_Core clock value in GHz
  dds_clock_mult = board[cur_board].dds_clock_mult; //DDS clock multiplier
  freq_word2 = (unsigned int) rint( ((freq * pow232) / (dds_clock * 1000.0 * dds_clock_mult)) ); 


	if(board[cur_board].firmware_id == 0xe01 || board[cur_board].firmware_id == 0xe02 || board[cur_board].firmware_id == 0x0E03 || board[cur_board].firmware_id == 0x0C13)
	{
		debug("writing shape period info to shape_period_array[%d]",cur_dds);
		shape_period_array[cur_dds][addr] = freq_word2;
	}
	else
	{  
		reg_write (REG_DDS_DATA2, freq_word2);

		reg_write (REG_SHAPE_CONTROL, SHAPE_WRITE_ADDR_SEL | (0x07 & addr));
		reg_write (REG_SHAPE_CONTROL, SHAPE_WRITE_ADDR_SEL | SHAPE_FREQ_WE | (0x07 & addr));
		reg_write (REG_SHAPE_CONTROL, SHAPE_WRITE_ADDR_SEL | (0x07 & addr));
		reg_write (REG_SHAPE_CONTROL, 0);
	}

	return 0;
}

SPINCORE_API int
pb_set_amp (float amp, int addr)
{
  spinerr = noerr;

  int amp_word = (int) (amp * (float) (1 << 14)) - 1;


  if (!board[cur_board].supports_dds_shape)
    {
      spinerr = "DDS Shape capabilities not supported on this board";
      debug ("%s", spinerr);
      return -1;
    }

  // FIXME: it may be possible to have negative amplitudes as well, though this could also
  // be accomplished through the phase registers
  if (amp > 1.0 || amp < 0.0)
    {
      spinerr = "Amplitude must be between 0.0 and 1.0, inclusive";
      debug ("%s", spinerr);
      return -1;
    }

  if (board[cur_board].firmware_id == 0xe01 || board[cur_board].firmware_id == 0xe02 || board[cur_board].firmware_id == 0x0E03 || board[cur_board].firmware_id == 0x0C13)
    {
      if (addr >= board[cur_board].dds_namp[cur_dds])
	{
	  spinerr = "Amplitude registers full";
	  debug ("%s", spinerr);
	  return -1;
	}

      if (addr < 0 || addr > board[cur_board].dds_namp[cur_dds])
	{
	  spinerr = "Must use valid amplitude register";
	  debug ("%s", spinerr);
	  return -1;
	}
	
    if(board[cur_board].firmware_id == 0x0C13 || board[cur_board].firmware_id == 0x0E03) //These designs have different Register Base Addresses
	{
      usb_write_address (board[cur_board].dds_address[cur_dds] + 0x4000 + addr);
	}
	else{
	  usb_write_address (board[cur_board].dds_address[cur_dds] + 0x0800 +
			 addr);
	}
      usb_write_data (&amp_word, 1);

      return 0;

  }
  else
    {
      if (addr >= board[cur_board].num_amp)
	{
	  spinerr = "Amplitude registers full";
	  debug ("%s", spinerr);
	  return -1;
	}

      debug ("addr=%d, amp=%f", addr, amp);


      reg_write (REG_DDS_DATA2, (unsigned int) amp_word);

      unsigned int control;

      if (board[cur_board].firmware_id == 0xa13)	//Firmware Revision 10-19 has 1024 amplitude registers.
	{			//address partion starts at bit 7 of REG_SHAPE_CONTROL.
	  control = (addr & 0x3FF) << 7;	//The amplitude control bits for the 0xa13 design start at bit 7. This implementation
	  //was to reduce software changes for the extended number of amplitude registers.
	}
      else
	{
	  control = SHAPE_WRITE_ADDR_SEL | (0x03 & addr);
	}

      reg_write (REG_SHAPE_CONTROL, control);

      // FIXME: I think address select is no longer relevant
      pb_set_radio_control (0x10);	// the imag_add is the address select
      pb_set_radio_control (0x08);	// the real_add signal is the we
      pb_unset_radio_control (0x08);
      pb_unset_radio_control (0x10);
      reg_write (REG_SHAPE_CONTROL, control);


      reg_write (REG_DDS_DATA2, (unsigned int) 0);
    }

  return 0;
}

SPINCORE_API int
pb_dds_load (float *data, int device)
{
  spinerr = noerr;

	if (!board[cur_board].supports_dds_shape)
    {
      spinerr = "DDS Shape capabilities not supported on this board";
      debug ("%s", spinerr);
      return -1;
    }

	int i;
	int val;
	
	//If the board is a PBDDS-II-300 AWG or PBDDS-I-300 12-19 board program the DDS and SHAPE RAMs as follows:
	if(board[cur_board].firmware_id == 0xe01 || board[cur_board].firmware_id == 0xe02 || board[cur_board].firmware_id == 0x0E03 || board[cur_board].firmware_id == 0x0C13)
	{
		int data_to_dds[1024];
		switch(device)
		{
			case DEVICE_SHAPE:
				for(i=0;i<1024;i++)
				{
					if(data[i] < -1.0 || data[i] > 1.0)
					{
						spinerr = "Data must be between -1.0 and 1.0, inclusive";
						debug ("%s (point %d)", spinerr, i);
						return -1;
					}
					else
						data_to_dds[i] = (int)(data[i] * 16383.0); //Shape value range is -16384 to 16383 (15 bits)
				}
				debug("Writing data to DDS channel %d Shape RAM",cur_dds);
				if(board[cur_board].firmware_id == 0x0C13 || board[cur_board].firmware_id == 0x0E03)
				  usb_write_address(board[cur_board].dds_address[cur_dds] + 0xA000);
				else
				  usb_write_address(board[cur_board].dds_address[cur_dds] + 0x1400);
				usb_write_data(data_to_dds, 1024);
				break;
			case DEVICE_DDS:
				for(i=0;i<1024;i++)
				{
					if(data[i] < -1.0 || data[i] > 1.0)
					{
						spinerr = "Data must be between -1.0 and 1.0, inclusive";
						debug ("%s (point %d)", spinerr, i);
						return -1;
					}
					else
						data_to_dds[i] = (int)(data[i] * 8191.0); //DDS value range is -8192 to 8191 (14 bits)
				}
				debug("Writing data to actual DDS channel %d output RAM",cur_dds);
				if(board[cur_board].firmware_id == 0x0C13 || board[cur_board].firmware_id == 0x0E03)
				  usb_write_address(board[cur_board].dds_address[cur_dds] + 0x8000);
				else
				  usb_write_address(board[cur_board].dds_address[cur_dds] + 0x1000);
				usb_write_data(data_to_dds, 1024);
				break;
			default:
				spinerr = "Invalid device number";
				debug ("%s");
				return -1;
		}
	}
	//Otherwise use this programming method instead:
	else
	{
		  char raw_data[2048];

		  debug ("writing to device 0x%x", device);

		  for (i = 0; i < 2048; i += 2)
			{
			  if (data[i / 2] > 1.0 || data[i / 2] < -1.0)
			{
			  spinerr = "Data must be between -1.0 and 1.0, inclusive";
			  debug ("%s (point %d)", spinerr, i / 2);
			  return -1;
			}

			  switch (device)
			{
			case DEVICE_SHAPE:
			  val = (int) (data[i / 2] * (float) (1 << 14));
			  if (val > 16383)
				{
				  val = 16383;
				}
			  if (val < -16384)
				{
				  val = -16384;
				}
			  break;
			case DEVICE_DDS:
			  val = (int) (data[i / 2] * 8192.0);
			  if (val > 8191)
				{
				  val = 8191;
				}
			  if (val < -8191)
				{
				  val = -8191;
				}

			  break;
			default:
			  spinerr = "Invalid device number";
			  debug ("%s");
			  return -1;
			}

			  raw_data[i] = 0x0FF & (val);
			  raw_data[i + 1] = 0x0FF & (val >> 8);

			}

		  switch (device)
			{
			case DEVICE_SHAPE:
			  reg_write (REG_SHAPE_CONTROL, SHAPE_SHAPERAM_WRITE_SEL);
			  break;
			case DEVICE_DDS:
			  reg_write (REG_SHAPE_CONTROL, SHAPE_DDSRAM_WRITE_SEL);
			  break;
			default:
			  spinerr = "Invalid device number";
			  debug ("%s");
			  return -1;
			}


		  ram_write (BANK_DDSRAM, 0, 2048, raw_data);

		  reg_write (REG_SHAPE_CONTROL, 0);
	}

  return 0;
}

SPINCORE_API int pb_adc_zero(int set)
{
  int offset;
   
  if(set == 0) //If we are clearing the offset correction.
  {
   reg_write (REG_ADC_CONTROL, 0x8 ); //Set the clear bit.
   reg_write (REG_ADC_CONTROL, 0 ); //Turn off the clear bit.
  }
  else
  {
   reg_write (REG_ADC_CONTROL, 0x4 ); //Set the set offset bit.
   reg_write (REG_ADC_CONTROL, 0 ); //Turn off the set offset bit.
  }
  
  offset = reg_read (REG_ADC_CONTROL); //Read the offset applied.
  debug("DC Offset correction set to: %d", offset);

  return offset; //Return the offset we read.
}

SPINCORE_API int
pb_fft (int n, int *real_in, int *imag_in, double *real_out, double *imag_out,
	double *mag_fft)
{
  int i;
  fftw_complex in[n], out[n];
  fftw_plan p;

  for (i = 0; i < n; ++i)
    {
      in[i].re = real_in[i];
      in[i].im = imag_in[i];
    }

  p = fftw_create_plan (n, FFTW_FORWARD, FFTW_ESTIMATE);

  fftw_one (p, in, out);

  for (i = 0; i < n; i++)
    {
      real_out[i] = out[i].re;
      imag_out[i] = out[i].im;
      mag_fft[i] = sqrt ((out[i].re * out[i].re) + (out[i].im * out[i].im));
    }

  fftw_destroy_plan (p);
  return 0;
}

SPINCORE_API double
pb_fft_find_resonance (int num_points, double SF, double SW, int *real, int *imag)
{
  int i, index, offset;
  double max, step, resFreq;
  double *temp, *magFFT;
  temp = (double *)malloc(num_points*sizeof(double));
  magFFT = (double *)malloc(num_points*sizeof(double)); 
  
  // Perform FFT calculation  
  pb_fft(num_points,real,imag,temp,temp,magFFT);
  
  // Perform FFT shift
  for(i=0; i<num_points/2; i++)
  {
    temp[num_points/2+i] = magFFT[i];         
  }
  for(i=num_points/2; i<num_points; i++)
  {
    temp[i-num_points/2] = magFFT[i];                    
  }
  
  // Determine Peak of FFT
  max = temp[0];
  index = 0;
  for(i=1; i<num_points; i++)
  {
    if(max < temp[i])
    {
      max = temp[i];
      index = i;
    }        
  }
  
  //Calculate Resonance Frequency
  step = SW/num_points;
  offset = index - num_points/2;
  resFreq = SF + offset*step;
  
  return resFreq;
  
}

SPINCORE_API int
pb_zero_ram()
{
	return 0;
}

