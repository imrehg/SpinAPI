/* 
 * PulseBlasterESR  example program from product manual
 *
 * This program will create an infinite loop consisting of three intervals during 
 *  which a) all 24 output bits will be ON for 40ns, b) bit #0 will be OFF and the
 *  remaining 23 output bits will be ON for 80 ns, and c) all output bits will be
 *  OFF for 1us.
 *
 * $ Date: 2006/08/23 $
 */

//

#include <stdio.h>
#define PBESR
#include "spinapi.h"

#define CLOCK 250.0  //The value of your clock oscillator in MHz

int main(void)
{

  int start;
	
	printf ("Using spinapi library version %s\n", pb_get_version());
  if (pb_init() != 0)
  {
    printf("--- Error Initializing PulseBlaster ---\n");
    return -1;
  }
  	
	printf("Using clock frequency:%.2fMHz\n\n", CLOCK);
	printf("You should now see a pulsetrain where: \n\n a) all 24 output bits will be ON for 40ns, then \n\n b) bit #0 will be OFF and the remaining 23 output bits will be ON for 80 ns, and then \n\n c) all output bits will be OFF for 1us. \n\n");

  // Tell driver what clock frequency the board uses
  pb_set_clock(CLOCK);
	
  // Prepare the board to receive Pulse Program instructions
  pb_start_programming(PULSE_PROGRAM);	

  // Instruction #0 - All outputs ON, continue to Instruction #1 after 40ns
  // Flags = 0xFFFFFF (all outputs ON), 
	// OPCODE = CONTINUE (proceed to next instruction after specified delay)
  // Data Field = empty.  This field is ignored for CONTINUE instructions.
  // Delay Count = 40*ns (other valid units are *us, *ms)
  start = pb_inst(0xFFFFFF, CONTINUE, 0, 40*ns);

  // Instruction #1 – Output bit 0 OFF, continue to instruction 2 after 80ns
  // Flags = 0xFFFFFE (all outputs ON except output bit 0) 
	// OPCODE = CONTINUE (proceed to next instruction after specified delay)
  // Data Field = empty.  This field is ignored for CONTINUE instructions.
  // Delay Count = 80*ns (other valid units are *us, *ms)
  pb_inst(0xFFFFFE, CONTINUE, 0, 80*ns);

  // Instruction #2 - Branch to "start" (Instruction #0) after 1us
  // Flags = 0x000000 (all output bits are OFF), 
  // OPCODE = BRANCH
  // Data Field = start (the target branch address)
  // Delay Count = 1*us (other valid units are *ns, *ms)
  pb_inst(0x000000, BRANCH, start, 1*us);

  pb_stop_programming();// Finished Sending Instructions
  pb_start();//  Run the Program

  pb_close();  // Release Control of the PulseBlasterESR board
	
	system("pause");
  return 0;
}
