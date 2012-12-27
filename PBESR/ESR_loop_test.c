/* PulseBlasterESR example program
 *
 * The following program tests the functionality of the loop instruction.
 * It takes two optional command-line arguments.
 * The first is the clock frequency, the default is 250MHz.  
 * The second is the number of loops, the default is 3 loops.  
 * 
 * BNCs 0-3 will output a pulse train with period 100ns. The number of pulses
 * in this train will be determined by the number of loops used. This pulse
 * train will be followed by ground level for 400ns, after which the entire
 * program will repeat.
 *
 * $Date: 2006/08/25 15:12:19 $
 */

#include <stdio.h>
#include <stdlib.h>
#define PBESR
#include "spinapi.h"

#define CLOCK_FREQ 250.0  //The value of your clock oscillator in MHz
#define NUM_LOOPS 3

int main(int argc, const char ** argv)
{
		
	int start=0,loop=0,num_loops=NUM_LOOPS;
	double clock_freq=CLOCK_FREQ;

	//set num_loops to 1st argument if present
	if (argc > 1)
		clock_freq=atof(argv[1]);
		
	if (argc > 2)
		num_loops=atoi(argv[2]);

  printf ("Using spinapi library version %s\n", pb_get_version());
	if (pb_init() != 0)
		{
			printf("Error Initializing PulseBlaster: %s\n", pb_get_error());
			return -1;
		}

	printf("Clock frequency: %.2lfMHz\n\n", clock_freq);
	printf("BNCs 0-3 will output %d successive pulses, each with a period of 100ns, followed by ground level for 400ns and then the program will repeat\n\n\n", num_loops);

	// Tell driver what clock frequency the board uses
	pb_set_clock(clock_freq); 

	//Begin pulse program
	pb_start_programming(PULSE_PROGRAM);

	//Instruction format
	//int pb_inst(int flags, int inst, int inst_data, int length)

	// Instruction 0 - continue to inst 1 in 200ns
	start =	pb_inst(0x0,CONTINUE,0,200.0*ns);
		
	// Instruction 1 - Beginning of Loop (Loop num_loop times). Continue to
    // next instruction in 50ns
	loop =	pb_inst(0xF, LOOP,num_loops,50.0*ns);
		
	// Instruction 2 - End of Loop.  Return to beginning of loop (instruction 1)
    // or continue to next instruction in 50ns
	pb_inst(0x0,END_LOOP,loop,50.0*ns); 

	// Instruction 3 - branch to start
	pb_inst(0x0,BRANCH,start,200.0*ns);

	// End of programming registers and pulse program
	pb_stop_programming();

	// Trigger the pulse program
	pb_start();

	pb_close();

	system("pause");
	return 0;
}
