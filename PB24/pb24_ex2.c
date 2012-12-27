/*
 * PulseBlaster example 2
 *
 * This example makes use of all instructions (except WAIT).
 *
 */

#include <stdio.h>
#define PB24
#include <spinapi.h>

int main(int argc, char **argv)
{
    int start, loop, sub;
    int status;

    pb_set_debug(1);
    printf ("Using spinapi library version %s\n", pb_get_version());
    if(pb_init() != 0) 
		{
        printf ("Error initializing board: %s\n", pb_get_error());    
        return -1;
    }

		printf("Clock frequency: 100.00MHz\n\n");
		printf("Please see pb24_ex2.c for a detailed description of this program's output.\n\n");

  	// Tell the driver what clock frequency the board has (in MHz)
    pb_set_clock(100.0);

    pb_start_programming(PULSE_PROGRAM);

    sub = 5; // Since we are going to jump forward in our program, we need to 
             // define this variable by hand.  Instructions start at 0 and count up

    //Instruction format
    //int pb_inst(int flags, int inst, int inst_data, int length)

    // Instruction 0 - Jump to Subroutine at Instruction 4 in 1s
    start =	pb_inst(0xFFFFFF,JSR, sub, 1000.0 * ms);

    // Loop. Instructions 1 and 2 will be repeated 3 times
    // Instruction 1 - Beginning of Loop (Loop 3 times).  Continue to next instruction in 1s
    loop =	pb_inst(0x0,LOOP,3,150.0 * ms);
    // Instruction 2 - End of Loop.  Return to beginning of loop or continue to next instruction in .5 s
    pb_inst(0xFFFFFF,END_LOOP,loop,150.0 * ms); 

    // Instruction 3 - Stay here for (5*100ms) then continue to Instruction 4
    pb_inst(0x0,LONG_DELAY,5, 100.0 * ms);

    // Instruction 4 - Branch to "start" (Instruction 0) in 1 s
    pb_inst(0x0,BRANCH,start,1000.0*ms);

    // Subroutine
    // Instruction 5 - Continue to next instruction in 1 * s
    pb_inst(0x0,CONTINUE,0,500.0*ms);
    // Instruction 6 - Return from Subroutine to Instruction 1 in .5*s
    pb_inst(0xF0F0F0,RTS,0,500.0*ms);

    // End of pulse program
    pb_stop_programming();

    // Trigger the pulse program
		pb_start();

    //Read the status register
    status = pb_read_status();
    printf("status = %d \n", status);

    pb_close();

		system("pause");	
		return 0;
}
