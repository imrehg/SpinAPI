/*
 * This program stops execution of a pulse program. The pulse program can then
 * be restarted at any time with a hardware of software trigger.It will work with 
 * all SpinCore products.
 */

#include <stdio.h>
#include "spinapi.h"

int main(int argc, char **argv)
{
    printf ("Using spinapi library version %s\n", pb_get_version());
    if(pb_init()) 
		{
      printf ("Error initializing board: %s\n", pb_get_error());    
      return -1;
    }

		if(pb_stop()) 
		{
      printf("Error occurred trying to stop board: %s\n\n", pb_get_error());
    }
    else 
		{
      printf("Board stopped successfully\n\n");
    }

	pb_close();

	system("pause");
	return 0;
}

