/*
 * The example program tests the excitation portion of the RadioProcessor.
 * This program will produce a 1MHz signal on the oscilloscope that is on
 * for 10 microseconds, off for 1 milisecond, and the repeat this pattern.
 *
 * SpinCore Technologies, Inc.
 * www.spincore.com
 * $Date: 2006/05/01 17:54:22 $
 */

#include <stdio.h>
#include "spinapi.h"

#define CLOCK_FREQ 75.0

// User friendly names for the control bits
#define TX_ENABLE 1
#define TX_DISABLE 0
#define PHASE_RESET 1
#define NO_PHASE_RESET 0
#define DO_TRIGGER 1
#define NO_TRIGGER 0

int main(int argc, char **argv)
{
	int start;

	pb_set_debug(1);
    printf ("Using spinapi library version %s\n", pb_get_version());
    if(pb_init()) {
        printf ("Error initializing board: %s\n", pb_get_error());    
	    system("pause");
        return -1;
    }
    pb_set_defaults();
    pb_set_clock(CLOCK_FREQ);

		printf("Clock frequency: %.2lfMHz\n\n", CLOCK_FREQ);
		printf("The example program tests the excitation portion of the RadioProcessor.  This program will produce a 1MHz signal on the oscilloscope that is on for 10 microseconds, off for 1 milisecond, and will then repeat this pattern.\n\n");

	// Write 1 MHz to the first frequency register
	pb_start_programming(FREQ_REGS);	
	pb_set_freq(1.0*MHz);
	pb_stop_programming();

	// Write 0.0 degrees to the first phase register of all channels
	pb_start_programming(TX_PHASE_REGS);
	pb_set_phase(0.0);
	pb_stop_programming();

	pb_start_programming(COS_PHASE_REGS);
	pb_set_phase(0.0);
	pb_stop_programming();

	pb_start_programming(SIN_PHASE_REGS);
	pb_set_phase(0.0);
	pb_stop_programming();
	
	
	// Write the pulse program
	pb_start_programming(PULSE_PROGRAM);

//pb_inst_radio(freq, cos_phase, sin_phase, tx_phase, tx_enable, phase_reset, trigger_scan, flags,
//              inst, inst_data, length)


	// This instruction enables a 1 MHz analog output
	start =
    pb_inst_radio(0,0,0,0, TX_ENABLE, NO_PHASE_RESET, NO_TRIGGER, 0x3F,
                  CONTINUE, 0, 10.0*us);

	// This instruction disables the analog output (and resets the phase in preparation for the next instruction)
    pb_inst_radio(0,0,0,0, TX_DISABLE, PHASE_RESET, NO_TRIGGER, 0x0,
                  BRANCH, start, 1.0*ms);

	pb_stop_programming();

	// Trigger program
	pb_start();					
  
	// Release control of the board
	pb_close();
	return 0;
}

