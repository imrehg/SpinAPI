/*
 * This program outputs the firmware # on the board
 *
 * SpinCore Technologies, Inc.
 * www.spincore.com
 * $Date: 2007/12/06 16:45:14 $
 */

#include <stdio.h>
#include <math.h>
#include "spinapi.h"

#define CLOCK_FREQ 75.0

int main(int argc, char **argv)
{
    pb_set_debug(1);
    printf ("Using spinapi library version %s\n", pb_get_version());
    if(pb_init()) {
        printf ("Error initializing board: %s\n", pb_get_error());  
        while (getchar() != '\n');
        return -1;
    }
    int i;
    for(i = 0; i < pb_count_boards(); i++)
    {
        pb_select_board(i);
        printf("Board %d Firmware ID: 0x%x\n", i, pb_get_firmware_id());
    }
	printf("\n\n");
	while (getchar() != '\n');
	pb_close();
	return 0;
}
