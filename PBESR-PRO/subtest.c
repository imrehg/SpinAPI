/* 
 * PulseBlasterESR-PRO example program. This demonstrates the use of the 
 * subroutine instruction.
 *
 * $Date: 2006/08/25 15:12:19 $
 */

#include <stdio.h>
#include <stdlib.h>
#define PBESRPRO
#include "spinapi.h"

#define CLOCK_FREQ 400.0  //The value of your clock oscillator in MHz

int main(int argc, const char ** argv)
{
		int start=0,sub=0;
		double clock_freq=CLOCK_FREQ;
		
		//if argument present
		if (argc > 1)
		{
			clock_freq = atof(argv[1]);
		}
				
    printf ("Using spinapi library version %s\n", pb_get_version());	
		if (pb_init() != 0)
		{
			printf("Error Initializing PulseBlaster: %s\n", pb_get_error());
			return -1;
		}

		printf("Using clock frequency:%.2fMHz\n\n", clock_freq);
		printf("BNCs 0-3 will output a train of pulses, each high for 50 ns, followed by ground level for 250 ns.  This pattern will then repeat.\n\n");

		// Tell driver what clock frequency the board uses
		pb_set_clock(clock_freq);

		//Begin pulse program
		pb_start_programming(PULSE_PROGRAM);

		//Instruction format
		//int pb_inst(int flags, int inst, int inst_data, int length)

		// Instruction 0 - continue to inst 1 in 100ns
		start =	pb_inst(0x0,CONTINUE,0,100.0*ns);
		
		// Instruction 1 - Jump to subroutine
		sub = 3;
		pb_inst(0, JSR, sub,50.0*ns);
		
		// Instruction 2 - Branch back to the beginning of the program
		// (instruction 0)
		pb_inst(0x0,BRANCH,start,100.0*ns); 

		// Instruction 3 - This is the start of the subroutine. Turn on
		// the BNC outputs for 50ns, and then return to the instruction
		// immediately after the JSR instruction that called this. In this
		// pulse program, the next instruction that will be executed is
		// instruction 2.
		pb_inst(ON|0xF,RTS,0,50.0*ns);

		// End of programming registers and pulse program
		pb_stop_programming();

    // Trigger the pulse program
		pb_start();

		pb_close();

		system("pause");
		return 0;
}
