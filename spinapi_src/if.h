/* if.h
 * Functions for interfacing with RadioProcessor board
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


#ifndef _IF_H
#define _IF_H

#define MEM_ADDRESS 8
#define MEM_DATA    12

#define EXT_ADDRESS 16
#define EXT_DATA    20

// Extended registers
#define REG_DDS_CONTROL          0x0001
#define REG_DDS_DATA             0x0002
#define REG_CONTROL              0x0003
#define REG_SAMPLE_NUM           0x0004
#define REG_DAC_CONTROL          0x0005
#define REG_ADC_CONTROL          0x0006
#define REG_CIC_CONTROL          0x0007
#define REG_FIR_COEF_DATA        0x0008
#define REG_FIR_COEF_ADDR        0x0009
#define REG_FIR_NUM_TAPS         0x000A
#define REG_FIR_CONTROL          0x000B
#define REG_EXPERIMENT_RUNNING   0x000C
#define REG_OVERFLOW_COUNT       0x000D
#define REG_OVERFLOW2_COUNT      0x000E
#define REG_SCAN_SEGMENTS        0x000F
#define REG_SCAN_COUNT           0x0010
#define REG_DDS_DATA2            0x0011
#define REG_PBRAM_ADDR           0x0012
#define REG_DATARAM_ADDR         0x0013
#define REG_PBCORE               0x0014
#define REG_SHAPE_CONTROL        0x0016
#define REG_CORE_SEL             0x0017
#define REG_CIC_CONTROL2         0x0018
#define REG_PULSE_PERIOD         0x0020
#define REG_PULSE_CLOCK_HIGH     0x0024
#define REG_PULSE_OFFSET         0x0028
#define REG_PULSE_SYNC		     0x002C

#define REG_FIRMWARE_ID          0x00FF

#define FIR_RESET 0x00010000

// this is valid on the COEF_ADDR register
#define FIR_COEF_LOAD_EN 0x0400

// bits of the control register

#define REG 0
#define AUX 1

// Bit defs for DDS_CONTROL register
#define DDS_RUN          0x00000020
#define DDS_FREQ_WE      0x00000004
#define DDS_TX_PHASE_WE  0x00000002
#define DDS_RX_PHASE_WE  0x00000001
#define DDS_REF_PHASE_WE 0x00040000
#define DDS_WRITE_SEL    0x80000000

// Bit defs for DDS_SHAPE register
#define SHAPE_FREQ_WE (1 << 4)
#define SHAPE_DDSRAM_WRITE_SEL (1 << 5)
#define SHAPE_SHAPERAM_WRITE_SEL (1 << 6)
#define SHAPE_WRITE_ADDR_SEL (1 << 7)
#define SHAPE_AMP_WE (1 << 8)


int dds_freq_extreg (int cur_board, int addr, int freq_word, int freq_word2);
int dds_phase_extreg (int cur_board, int phase_bank, int addr,
		      int phase_word);


void reg_write (unsigned int address, unsigned int data);
unsigned int reg_read (unsigned int address);
int ram_write (unsigned int bank, unsigned int start_addr, unsigned int len, char *data);


int num_bits (int num);


#endif /* #ifdef _IF_H */
