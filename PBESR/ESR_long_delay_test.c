/* 
 * PulseBlasterESR  example program
 *
 * The following program tests the functionality of the long delay opcode. The
 * long delay opcode determines what the delay value is by multipling the given
 * delay value by the data field. Thus, this program will output a pulse train
 * with period (100ns*long_delay)
 *
 * This program takes two optional command-line arguments.
 * The first is the clock frequency, the default is 250MHz.  
 * The second is the number of delay loops in the pulse train, the default is 5.  
 *
 * $Date: 2006/08/25 15:12:19 $
 */

#include <stdio.h>
#include <stdlib.h>

#define PBESR
#include "spinapi.h"

#define CLOCK_FREQ 250.0  //The value of your clock oscillator in MHz
#define DELAY_LOOP 5

int main(int argc, const char ** argv)
{
		int start=0, delay_loop=DELAY_LOOP;
		double clock_freq=CLOCK_FREQ;

		//set num_loops to 1st argument if present
		if (argc > 1)
			clock_freq=atof(argv[1]);
		
		if (argc > 2)
			delay_loop=atoi(argv[2]);

    printf ("Using spinapi library version %s\n", pb_get_version());
    if (pb_init() != 0)
		{
			printf("Error Initializing PulseBlaster: %s\n", pb_get_error());
			return -1;
		}
			
		printf("Clock frequency: %.2fMHz\n\n", clock_freq);
		printf("BNCs 0-3 should output a pulse train with period %uns\n\n\n", delay_loop*100*2);

    // Tell driver what clock frequency the board uses
		pb_set_clock(clock_freq);

		//Begin pulse program
		pb_start_programming(PULSE_PROGRAM);

		//Instruction format
		//int pb_inst(int flags, int inst, int inst_data, int length)

		// Instruction 0 - continue to inst 1 in (100ns*delay_loop)
		start =	pb_inst(0xF,LONG_DELAY,delay_loop,100.0*ns);
			
		// Instruction 1 - continue to instruction 2 in (100ns*(delay_loop-1))
		pb_inst(0x0,LONG_DELAY,delay_loop-1,100.0*ns);

		// Instruction 2 - branch to start
		pb_inst(0x0, BRANCH, start, 100.0*ns);

		// End of programming registers and pulse program
		pb_stop_programming();

		// Trigger pulse program
		pb_start();

		pb_close();

		system("pause");
		return 0;
}
