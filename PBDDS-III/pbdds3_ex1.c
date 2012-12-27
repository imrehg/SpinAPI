/* Example 1 for the PulseBlasterDDS
 *
 * This program outputs a 1MHz sine wave on both TX and RX channels.
 * The TX Channel cycles between being on and off with a period
 * of 8us.
 * The TX and RX channels are set to have a 90 degree phase offset.
 */

#include <stdio.h>
#define PBDDS
#include "spinapi.h"

#define CLOCK 100.0

int main(int argc, char **argv)
{
	int start;
	int status;

    printf ("Using spinapi library version %s\n", pb_get_version());
    if(pb_init() != 0) 
		{
        printf ("Error initializing board: %s\n", pb_get_error());    
        return -1;
    }

		printf("Clock frequency: %.2lfMHz\n\n", CLOCK);
		printf("This program outputs a 1MHz sine wave on both TX and RX channels.  The TX Channel cycles between being on and off with a period of 8us.  The TX and RX channels are set to have a 90 degree phase offset. \n\n");

		// Tell the driver what clock frequency the board has
		pb_set_clock(CLOCK);  
	
	// Program the frequency registers
	pb_start_programming(FREQ_REGS);	
	pb_set_freq(1.0*MHz); // Register 0
	pb_set_freq(2.0*MHz); // Register 1
	pb_stop_programming();
	
	// Program RX phase registers (DAC_OUT_1) [Units in degrees]
	pb_start_programming(PHASE_REGS_1);
	pb_set_phase(0.0);  // Register 0
	pb_set_phase(90.0); // Register 1
	pb_stop_programming();
	
	// Program the TX phase registers (DAC_OUT_0 and DAC_OUT_2)
	pb_start_programming(PHASE_REGS_0);
	pb_set_phase(0.0);   // Register 0
	pb_set_phase(180.0); // Register 1
	pb_stop_programming();
	
	// Send the pulse program to the board
	pb_start_programming(PULSE_PROGRAM);

    //For PulseBlasterDDS boards, the pb_inst function is translated to the
    //pb_inst_tworf() function, which is defined as follows:
	//pb_inst_tworf(freq, tx_phase, tx_output_enable, rx_phase, rx_output_enable,
	//				flags, inst, inst_data, length);

	//Instruction 0 - Continue to instruction 1 in 2us
	//Output frequency in freq reg 0 to both TX and RX channels
	//Set all TTL output lines to logical 1
    start = pb_inst(0, 1, TX_ANALOG_ON, 0, RX_ANALOG_ON,
                    0x1FF, CONTINUE, 0, 2.0*us);

	//Instruction 1 - Continue to instruction 2 in 4us
	//Output frequency in freq reg 0 RX channels, TX channel is disabled
	//Set all TTL outputs to logical 0
	pb_inst(0, 1, TX_ANALOG_OFF, 0, RX_ANALOG_ON,
            0x000, CONTINUE, 0, 4.0*us);

	//Instruction 2 - Branch to "start" (Instruction 0) in 2us
	//Output frequency in freq reg 0 to both TX and RX channels
	//Set all TTL output lines to logical 1
	pb_inst(0, 1, TX_ANALOG_ON, 0, RX_ANALOG_ON,
            0x000, BRANCH, start, 2.0*us);

	pb_stop_programming();

	// Trigger the pulse program
	pb_start();					

    // Retreive the status of the current board. This will be 0x04 since
    // the pulse program is a loop and will continue running indefinitely.
    status = pb_read_status();
    printf("status: 0x%.2x\n", status);
	
	// Release Control of the PulseBlasterDDS Board
	pb_close();

	system("pause");
	return 0;
}

