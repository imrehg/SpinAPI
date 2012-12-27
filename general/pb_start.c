/*
 * This program sends a software trigger to the board. It will work with 
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

	if(pb_start()) 
	{
    printf("Error occurred trying to trigger board: %s\n\n", pb_get_error());
	}
  else 
	{
    printf("Board triggered successfully\n\n");
  }

	pb_close();

	system("pause");
	return 0;
}

