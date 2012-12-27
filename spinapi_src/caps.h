/* caps.h
 * Database of the capabilties of each product
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

#ifndef _CAPS_H
#define _CAPS_H

#define AMCC_DEVID 0x8852	//dev id for boards using amcc bridge chip
#define	PBESR_PRO_DEVID	0x8875	//dev id for vhdl pci core
#define VENDID	0x10e8		//our vendor id  (AMCC)


#define DDS_PROG_OLDPB   0
#define DDS_PROG_EXTREG  1

// Note: this structure should not be made available to the user, since
// otherwise we could not guarantee binary compatibility
typedef struct
{

  int did_init;		      /** nonzero if this board has been initialized */
  int is_usb;	  /** nonzero if this is a usb devices */
  int usb_method;  /** method 1 is the original method, method 2 is what the DDSII usb boards use along with future PB24 USB boards*/
  
  int use_amcc;	  /** nonzero if this board uses an amcc bridge chip. 1 for "modern" amcc protocol. 2 for "old" protocol, used by PB02PC boards*/
  double clock;	  /** the clock speed of this board (in GHz)*/

  double dds_clock_mult;       /** this times clock is the dds clock speed */
  double pb_clock_mult;	   /** this times clock is the pulseblaster clock speed */

  int has_firmware_id;	     /** nonzero if the firmware id register exists. if 1, it is at location 0xFF, if 2, it is at location 0x15 */
  int firmware_id;	 /** */
  //bugfixes
  int has_FF_fix;	/** PB Core "1FF" issue status (equal to 1 if the board has the fix) */

  // pusle program limits
  int num_instructions;	      /** number of pulse instructions the design can hold **/
  int num_IMW_bytes;          /** number of bytes making up the internal memory word **/

  int status_oldstyle;	 /** ... **/


  // dds limits
  int dds_prog_method;	 /** either DDS_PROG_OLDPB
                             -or-   DDS_PROG_EXTREG */
  int num_phase0;   /** number of phase registers in bank 0 (tx)*/
  int num_phase1;   /** number of phase registers in bank 1 (rx)*/
  int num_phase2;   /** number of phase registers in bank 2 (ref)*/
  int num_freq0;	/** number of frequency registers */

  // RadioProcessor
  int is_radioprocessor;
  int cic_max_stages;
  int cic_max_decim;
  int cic_max_shift;

  int fir_max_taps;
  int fir_max_decim;
  int fir_max_shift;
  int fir_coef_width;	    /** width in bits of the fir coefficients */

  int num_points;	/** number of complex points the board can hold */
  int supports_scan_segments;
  int supports_scan_count;

  int has_wraparound_problem;

  int supports_cyclops;
  int supports_dds_shape;
  int num_shape;   /** number of period registers for the dds shape */
  int num_amp;	 /** number of amplitude registers that are available */

  int acquisition_disabled;   /** set to 1 if board does not allow data acquisition */
  int custom_design;   /** 0 => not a custom design
                           1 => Topsin(Israel) custom design (firmware 10-10)
                           2 => Progression Systems custom design (firmware 12-6)
                           3 =>  custom design (firmware 10-15) */

  int number_of_dds;
  unsigned int dds_list[4];

  unsigned int dds_nfreq[4];
  unsigned int dds_nphase[4];
  unsigned int dds_namp[4];

  unsigned int pb_base_address;

  unsigned int prog_clock_base_address; /** base address for a programmable clock, used with SP9 boards */
  unsigned int dds_address[4];

  unsigned int pb_core_version;


  //int is_sasi_mod; /** set to 1 if board is firmware rev. 10-10, a custom design for Israel */
  //int is_progression_mod; /** set to 1 if board is firmware rev. 12-6, a custom design for Progression Systems */

} BOARD_INFO;


int get_caps (BOARD_INFO * board, int dev_id);

#endif /* #define _CAPS_H */
