// Example program for PulseBlasterESR-PRO boards
//
// This demonstrates a simple pulse loop and also the short pulse feature.
// $Date: 2006/08/25 15:12:19 $

// All four BNC connectors are high for three clock periods (making use
// of the short-pulse feature), then low for the duration of instruction 1,
// or 20 ns. The BNC connectors are then held high for 40ns.  
// All four BNC connectors are then low for 40ns and the program repeats.

// The largest value for the delay field of the pb_inst is 850ns.
// For longer delays, use the LONG_DELAY instruction.  The maximum value
// for the data field of the LONG_DELAY is 1048576.  Even longer delays can
// be achieved using the LONG_DELAY instruction inside of a loop.

#include <stdio.h>
#define PBESRPRO
#include "spinapi.h"

#define CLOCK 400.0  //The value of your clock oscillator in MHz

int main(int argc, char **argv)
{
	int start;

  printf ("Using spinapi library version %s\n", pb_get_version());
	if (pb_init() != 0)
	{
			printf("Error Initializing PulseBlaster: %s\n", pb_get_error());
			return -1;
	}
	
	printf("Clock frequency: %.2fMHz\n\n", CLOCK);
	printf("All four BNC outputs will output a pulse with a period of 20ns, the high time of which will be 3 clock cycles (%.1f ns).  The BNC outputs will then be held high for 40ns and then low for 40ns and the program will repeat.\n\n",3000/CLOCK);	
	
	// Tell driver what clock frequency the board uses
	pb_set_clock(CLOCK);
	
	// Prepare the board to receive pulse program instructions
	pb_start_programming(PULSE_PROGRAM);

	// Instruction 0 - Continue to instruction 1 in 20ns
    // The lower 4 bits (all BNC connectors) will be driving high
    // For PBESR-PRO boards, or-ing THREE_PERIOD with the flags
    // causes a 3 period short pulse to be used. 
	start = pb_inst(THREE_PERIOD|0xF, CONTINUE, 0, 20.0*ns);

	// Instruction 1 - Continue to instruction 2 in 40ns
    // The lower 4 bits (all BNC connectors) will be driving high
    // the entire 40ns.
	pb_inst(ON|0xF, CONTINUE, 0, 40.0*ns);

	// Instruction 2 - Branch to "start" (Instruction 0) in 40ns
    // Outputs are off
	pb_inst(0, BRANCH, start, 40.0*ns);

	pb_stop_programming();	// Finished sending instructions
	pb_start();				// Trigger the pulse program

    // End communication with the PulseBlasterESR board. The pulse program
    // will continue to run even after this is called
	pb_close();
	
	system("pause");
	return 0;
}

