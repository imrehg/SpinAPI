/*
 * This example program tests the excitation PulseBlaster DDS 300.  This program will produce a 1MHz signal on the 
 * oscilloscope that is on for 10 microseconds then off for 1 milisecond, and will then repeat this pattern 
 * indefinately.
 *
 * SpinCore Technologies, Inc.
 * www.spincore.com
 * $Date: 2007/07/13 16:25:44 $
 */

#include <stdio.h>
#include "spinapi.h"

#define CLOCK_FREQ 75.0

// User friendly names for the control bits
#define TX_ENABLE 1
#define TX_DISABLE 0
#define PHASE_RESET 1
#define NO_PHASE_RESET 0

int main(int argc, char **argv)
{
	int start;
	//uncomment to turn on debug messages
    //pb_set_debug(1);//turns on debug messages. They will appear in log.txt
    printf ("Using spinapi library version %s\n", pb_get_version());
    
    if(pb_init()) {
        printf ("Error initializing board: %s\n", pb_get_error());   
	    system("pause"); 
        return -1;
    }
    pb_set_defaults();
    pb_set_clock(CLOCK_FREQ);

		printf("Clock frequency: %.2lfMHz\n\n", CLOCK_FREQ);
		printf("This example program tests the excitation PulseBlaster DDS 300.  This program will produce a 1MHz signal on the oscilloscope that is on for 8 microseconds, cycling through four pahse offsets. It will turn off for 1 milisecond, and will then repeat this pattern.\n\n");

	// Program the first 2 frequency registers, there are 16 available for the PBDDS-1-300
	pb_start_programming(FREQ_REGS);	
	pb_set_freq(1.0*MHz); // Register 0
	pb_set_freq(1.0*MHz); // Register 1
	pb_stop_programming();

	// Program the first 2 phase registers, there are 16 available for the PBDDS-1-300
	pb_start_programming(TX_PHASE_REGS);
	pb_set_phase(0.0);  // Register 0
	pb_set_phase(0.0);  // Register 1
	pb_stop_programming();
	
	
    // Write the pulse program
	pb_start_programming(PULSE_PROGRAM);

    //pb_inst_dds(freq, tx_phase, tx_enable, phase_reset, flags,
    //              inst, inst_data, length)

	start =
    pb_inst_dds(1, 1, TX_ENABLE, NO_PHASE_RESET, 0x1FF, CONTINUE, 0, 10.0*us);

    pb_inst_dds(1, 1, TX_DISABLE, PHASE_RESET, 0x000, BRANCH, start, 1.0*ms);

    pb_stop_programming();
	
	
	// Trigger program
	pb_start();					
  
	// Release control of the board
	pb_close();

	system("pause");
	return 0;
}

