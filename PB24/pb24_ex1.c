/*
 * PulseBlaster example 1
 *
 * This program will cause the outputs to turn on and off with a period
 * of 400ms
 *
 */

#include <stdio.h>
#include <stdlib.h>
#define PB24
#include "spinapi.h"

int main()
{
   int start, status, i;
    //pb_select_board(0);
    // pb_set_debug(1);
    printf ("Using spinapi library version %s\n", pb_get_version());
    if(pb_init() != 0) 
		{
        printf ("Error initializing board: %s\n", pb_get_error());    
        return -1;
    }

		printf("Clock frequency: 100.00MHz\n\n");
		printf("All outputs should now be turning on and off with a period of 400ms and 50 percent duty cycle.\n\n");

  	// Tell the driver what clock frequency the board has (in MHz)
    pb_set_clock(100.0);

    pb_start_programming(PULSE_PROGRAM);
	
    // Instruction 0 - Continue to instruction 1 in 100ms
    // Flags = 0xFFFFFF, OPCODE = CONTINUE
    start = pb_inst(0xFFFFFF, CONTINUE, 0, 200.0*ms);

    // Instruction 1 - Continue to instruction 2 in 100ms
    // Flags = 0x0, OPCODE = CONTINUE
    pb_inst(0x0, CONTINUE, 0, 100.0*ms);

    // Instruction 2 - Branch to "start" (Instruction 0) in 100ms
    // 0x0, OPCODE = BRANCH, Target = start
    pb_inst(0x0, BRANCH, start, 100.0*ms);

	pb_stop_programming();

    // Trigger the pulse program
	pb_start();

	//Read the status register
	status = pb_read_status();
	printf("status: %d \n", status);

	pb_close();

	system("pause");
	return 0;
}

