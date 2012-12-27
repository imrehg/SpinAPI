/*
 * This example program tests demonstrates a frequency sweep using all the registers of the 
 * PBDDS-I-300.
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

#define NUM_FREQUENCY_REGISTERS 1024                   //Specify the number of registers your board has.
#define RATE 10                                        //Time (in milliseconds) between frequency changes.
#define MIN  1.0                                       //Start frequency.
#define MAX  10.0                                      //End frequency.
#define FREQ_STEP ((MAX-MIN)/NUM_FREQUENCY_REGISTERS)
 
#define STEP FREQ_STEP                                 //Step amount in MHz.

int main(int argc, char **argv)
{
	int start, i;
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

		printf("Clock frequency: %.2lf MHz\n\n", CLOCK_FREQ);
		printf("This example program demonstrates a frequency sweep using the PBDDS-I-300.\n\nYou have specified %d frequency registers.\nYou should see the DDS sweep frequencies from %.4lf MHz to %.4lf MHz with a\nstep size of %.4lf MHz and a time between steps of %d ms.\nThe frequencies should oscillate back and forth from minimum to maximum, maximum to minimum.\n", NUM_FREQUENCY_REGISTERS,MIN,MAX,STEP,RATE);

	// Program the frequency registers.
	pb_start_programming(FREQ_REGS);	  
    for(i=0;i<NUM_FREQUENCY_REGISTERS; ++i)
        pb_set_freq(MIN + STEP*((double)i));                                       
	pb_stop_programming();

	// Program the first 2 phase registers.
	pb_start_programming(TX_PHASE_REGS);
	pb_set_phase(0.0);  // Register 0
	pb_set_phase(0.0);  // Register 1
	pb_stop_programming();
	
	
    // Write the pulse program
	pb_start_programming(PULSE_PROGRAM);

    //pb_inst_dds(freq, tx_phase, tx_enable, phase_reset, flags,
    //              inst, inst_data, length)

	start = pb_inst_dds(0, 0, TX_DISABLE, PHASE_RESET, 0x000, CONTINUE, 0, 1.0 * us);
	
	for(i=0; i < NUM_FREQUENCY_REGISTERS; ++i) //Program instructions (increasing)
        pb_inst_dds(i, 0, TX_ENABLE, NO_PHASE_RESET, 0x1FF, CONTINUE, 0, RATE*ms);
        
 	for(i= NUM_FREQUENCY_REGISTERS-1; i >= 1; --i) //Program instructions (decreasing)
        pb_inst_dds(i, 0, TX_ENABLE, NO_PHASE_RESET, 0x1FF, CONTINUE, 0, RATE*ms);
        
    pb_inst_dds(0, 0, TX_ENABLE, NO_PHASE_RESET, 0x1FF, BRANCH, start, RATE*ms);


    pb_stop_programming();
	
	
	// Trigger program
	pb_start();					
  
	// Release control of the board
	pb_close();

	system("pause");
	return 0;
}

