/*
 * DirectCapture.c
 *
 * This program demonstrates how to use the Direct Ram Capture feature of the 
 * RadioProcessor.
 * NOTE: This feature is only enabled in the PCI RadioProcessor with Firmware Revisions
 * 10-14 and up and USB RadioProcessors with Firmware Revisions 12-5 and up.
 * This program is compatible with SpinAPI Versions 20080521 and up.
*/

#include <stdio.h>
#include <stdlib.h>
#include "spinapi.h"

//Defines for easy reading.
#define RAM_SIZE (16*1024)
#define BOARD_STATUS_IDLE 0x3
#define DO_TRIGGER 1
#define NO_TRIGGER 0
#define ADC_FREQUENCY 75.0

//This is the board you wish to use.
#define BOARD_NUMBER 1

//The number of points your board will acquire.
//For PCI boards, the full ram size is 16384 points.  For USB boards it is 4*16384 points.
#define NUMBER_POINTS 4*16384 

//The number of scans to perform (with signal averaging enabled.)
//If you use signal averaging, please make sure that there is phase coherence between scans.
#define NUMBER_SCANS 1 

int main(void)
{  
   int i;
   short data[NUMBER_POINTS], data_imag[NUMBER_POINTS];
   int 	 idata[NUMBER_POINTS],idata_imag[NUMBER_POINTS];
   
   printf("Using SpinAPI Version: %s\n",pb_get_version());
   printf("Number of boards detected in your system: %d\n", (i=pb_count_boards()));
   
   if(i<=0)
   {
      printf("No boards were detected in your system. Please check all connections.\n");
      system("pause");
      return -1;
   }
   
   pb_select_board(BOARD_NUMBER);
   
   printf("This program demonstrates the direct ram capture feature of the RadioProcessor.\n");
   
   if(!pb_init())
   {
     printf("Error initializing board #%d. Please check that this is a valid board number and that all connections are correct.\n");
     return -1;
   }

   pb_set_defaults();
   
   pb_set_clock(ADC_FREQUENCY); //Set the ADC frequency.
   pb_zero_ram();    // Clear RAM to all zeros (PCI only).
   pb_overflow(1,0); // Reset the overflow counters.
   pb_scan_count(1); // Reset scan counter.
   
   pb_set_radio_control(RAM_DIRECT); //Enable direct ram capture.

   pb_set_num_points(NUMBER_POINTS); //This is the number of points that we are going to be capturing.
   
   double wait_time = 1000.0 * (NUMBER_POINTS)/(ADC_FREQUENCY*1e6); // Time in ms
   
   pb_start_programming(PULSE_PROGRAM);
   int start = pb_inst_radio_shape(0,0, 0, 0, 0, 0, NO_TRIGGER, 0, 0, 0x02, LOOP    , NUMBER_SCANS, 1.0*us);
               pb_inst_radio_shape(0,0, 0, 0, 0, 0, DO_TRIGGER, 0, 0, 0x01, END_LOOP, start       , wait_time * ms);                   
               pb_inst_radio_shape(0,0, 0, 0, 0, 0, NO_TRIGGER, 0, 0, 0x02, STOP    , 0           , 1.0*us);
   pb_stop_programming();
   
   printf("Performing direct ram capture...\n");
   
   pb_start();
   
   printf("Waiting for the data acquisition to complete.\n");
            
   while(pb_read_status() != BOARD_STATUS_IDLE) //Wait for the board to complete execution.
   {
      pb_sleep_ms(100);
   }
   
   pb_get_data_direct(NUMBER_POINTS,data);
   
   pb_unset_radio_control(RAM_DIRECT); //Disable direct ram capture.
   
   pb_close();
   
   FILE* fp_ascii = fopen("direct_data.txt","w");
   
   for(i=0;i<NUMBER_POINTS;++i)
   {
    fprintf(fp_ascii,"%d\n",data[i]);
   } 
   fclose(fp_ascii);
   
   for(i=0;i<NUMBER_POINTS; ++i) 
   {
       idata[i] = data[i];
       idata_imag[i] = data_imag[i];
   }   
   
   //The spectrometer frequency does not matter in a direct ram capture. Using 1.0 MHz for
   //proper file format.
  
   pb_write_felix("direct_data.fid", NUMBER_POINTS,ADC_FREQUENCY*1e6, 1.0, idata, idata_imag);
    
   system("PAUSE");
   return 0;
}
