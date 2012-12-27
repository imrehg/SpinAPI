/*
 * Example 2 for the PulseBlasterDDS
 *
 * This program makes use of all available instructions (except WAIT)
 *
 * On the TX channel:
 * There will be a single period of a 1MHz sine wave, followed by a 1us gap.
 * This pattern will be repeated 2 more times followed by a long (5ms) gap,
 * after which the pattern will repeat again.
 *
 * On the RX channel:
 * There will be a 1 MHz sine wave, which will be turned off for short
 * periods of time and have its phase changed several times. See the
 * pulse program below for details.
 *
 */

#include <stdio.h>

#define PBDDS
#include "spinapi.h"

#define CLOCK 100.0

int main(void)
{
	int start, loop, sub;

    printf ("Using spinapi library version %s\n", pb_get_version());
    if(pb_init() != 0) {
        printf ("Error initializing board: %s\n", pb_get_error());    
        return -1;
    }

		printf("Clock frequency: %.2lfMHz\n\n", CLOCK);
		printf("On the TX channel: \n There will be a single period of a 1MHz sine wave, followed by a 1us gap.  This pattern will be repeated 2 more times followed by a long (5ms) gap, after which the pattern will repeat again. \n\nOn the RX channel: \n There will be a 1 MHz sine wave, which will be turned off for short periods of time and have its phase changed several times. See pbdds2_ex2.c for details.\n\n"); 
	
    // Tell driver what clock frequency the board has
    pb_set_clock(CLOCK);

    //Program the frequency registers
    pb_start_programming(FREQ_REGS);
    pb_set_freq(1.0*MHz);  // Set register 0
    pb_set_freq(2.0*MHz);  // Set register 1
    pb_set_freq(3.0*MHz);  // Set register 2
    pb_set_freq(4.0*MHz);  // Set register 3
    pb_set_freq(5.0*MHz);  // Set register 4 
    pb_set_freq(6.0*MHz);  // Set register 5
    pb_set_freq(7.0*MHz);  // Set register 6
    pb_set_freq(8.0*MHz);  // Set register 7
    pb_set_freq(9.0*MHz);  // Set register 8
    pb_set_freq(10.0*MHz); // Set register 9
    pb_set_freq(11.0*MHz); // Set register 10
    pb_set_freq(12.0*MHz); // Set register 11
    pb_set_freq(13.0*MHz); // Set register 12
    pb_set_freq(14.0*MHz); // Set register 13
    pb_set_freq(15.0*MHz); // Set register 14
    pb_set_freq(16.0*MHz); // Set register 15
    pb_stop_programming();
		
    //Program the RX phase registers (DAC_OUT_1) [Units in degrees]
    pb_start_programming(PHASE_REGS_1);
    pb_set_phase(0.0);   // Set register 0
    pb_set_phase(22.5);  // Set register 1
    pb_set_phase(45.0);  // Set register 2
    pb_set_phase(67.5);  // Set register 3
    pb_set_phase(90.0);  // Set register 4
    pb_set_phase(112.5); // Set register 5
    pb_set_phase(135.0); // Set register 6
    pb_set_phase(157.5); // Set register 7
    pb_set_phase(180.0); // Set register 8
    pb_set_phase(202.5); // Set register 9
    pb_set_phase(225.0); // Set register 10
    pb_set_phase(247.5); // Set register 11
    pb_set_phase(270.0); // Set register 12
    pb_set_phase(292.5); // Set register 13
    pb_set_phase(315.0); // Set register 14
    pb_set_phase(337.5); // Set register 15
    pb_stop_programming();

    //Program the TX phase registers (DAC_OUT_0 and DAC_OUT_2)
    pb_start_programming(PHASE_REGS_0);
    pb_set_phase(0.0);   // Set register 0
    pb_set_phase(22.5);  // Set register 1
    pb_set_phase(45.0);  // Set register 2
    pb_set_phase(67.5);  // Set register 3
    pb_set_phase(90.0);  // Set register 4
    pb_set_phase(112.5); // Set register 5
    pb_set_phase(135.0); // Set register 6
    pb_set_phase(157.5); // Set register 7
    pb_set_phase(180.0); // Set register 8
    pb_set_phase(202.5); // Set register 9
    pb_set_phase(225.0); // Set register 10
    pb_set_phase(247.5); // Set register 11
    pb_set_phase(270.0); // Set register 12
    pb_set_phase(292.5); // Set register 13
    pb_set_phase(315.0); // Set register 14
    pb_set_phase(337.5); // Set register 15
    pb_stop_programming();


    //Begin pulse program
    pb_start_programming(PULSE_PROGRAM);

    //For PulseBlasterDDS boards, the pb_inst function is translated to the
    //pb_inst_tworf() function, which is defined as follows:
	//pb_inst_tworf(freq, tx_phase, tx_output_enable, rx_phase, rx_output_enable,
	//				flags, inst, inst_data, length);
	
    sub = 5; // Since we are going to jump forward in our program, we need to 
             // define this variable by hand.  Instructions start at 0 and count up

    // Instruction 0 - Jump to Subroutine at Instruction 5 in 1us
    start =	
    pb_inst(0,0,TX_ANALOG_OFF,0,RX_ANALOG_ON,0x1FF,JSR,sub,1*us);

    // Loop. The next two instructions make up a loop which will be repeated
    // 3 times.
    // Instruction 1 - Beginning of Loop (Loop 3 times).  Continue to next instruction in 1us
    loop = 
    pb_inst(0,0,TX_ANALOG_OFF,0,RX_ANALOG_OFF,0x0,LOOP,3,1*us);
    // Instruction 2 - End of Loop.  Return to beginning of loop or continue to next instruction in 1us
    pb_inst(0,0,TX_ANALOG_ON,0,RX_ANALOG_OFF,0x0,END_LOOP,loop,1*us); 

    // Instruction 3 - Stay here for (5*1ms) then continue to Instruction 4
    pb_inst(0,0,TX_ANALOG_OFF,0,RX_ANALOG_ON,0x0,LONG_DELAY,5,1*ms);

    // Instruction 4 - Branch to "start" (Instruction 0) after 1us
    // This has the effect of looping the entire program indefinitely.
    pb_inst(0,0,TX_ANALOG_OFF,0,RX_ANALOG_ON,0x0,BRANCH,start,1*us);

    // Subroutine. This subroutine is called by using the JSR instruction
    // and specifying the address of instruction 5. (As is done in instruction
    // 1)
    // Instruction 5 - Reset phase and continue to next instruction in 2us
    pb_inst(0,4,TX_ANALOG_OFF,8,RX_ANALOG_ON,0x200,CONTINUE,0,2*us);
		
    // Instruction 6 - Return from Subroutine after 2us
    pb_inst(0,8,TX_ANALOG_OFF,8,RX_ANALOG_ON,0x0,RTS,0,2*us);

    pb_stop_programming();

    // Trigger the pulse program
    pb_start();

    // Release control of the PulseBlasterDDS board
    pb_close();

		system("pause");
    return 0;
}

