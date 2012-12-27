/**
 *
 * CPMG program
 *
 * $Date: 2008/07/11 15:17:02 $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spinapi.h"

#define RAM_SIZE (8*1024) //Max RAM size for USB board = 256*1024

int main (int argc, char **argv)
{
    pb_select_board(1);
    int real[RAM_SIZE];
    int imag[RAM_SIZE];

    int start;
    PB_OVERFLOW_STRUCT of;

    char txt_fname[1024];
    char fid_fname[1024];

    // pulse program parameters
    int echo_points;
    double SF;
    double SW;
    double P2_time;
    double P2_phase;
    double ringdown_time;

    double P1_time;
    double P1_phase;
    double tau;
    double P1_after1;
    double P1_after2;
    double top_time;

    int echo_loop_label;
    int scan_loop_label;
                   
    int echo_loops;
    int num_scans;

    double wait_time;

    char *fname;
    int bypass_fir;
    double adc_freq;

    int cmd;
    int do_top_trigger;

    if (argc < 14) {
        printf ("Invalid number of arguments, please see the documentation for proper syntax\n");    
        exit(1);
    }

    SF =        atof(argv[1]); // MHz
    SW =        atof(argv[2]); // kHz
    P2_time =   atof(argv[3]); // us
    ringdown_time =  atof(argv[4]); // us
    P2_phase =  atof(argv[5]); // degrees
    P1_phase =  atof(argv[6]); // degrees
    tau =       atof(argv[7]); // us
    echo_points = atoi(argv[8]);
    echo_loops  = atoi(argv[9]);
    num_scans   = atoi(argv[10]);
    fname       = argv[11];
    bypass_fir  = atoi(argv[12]);
    adc_freq    = atof(argv[13]); // MHz
    wait_time   = atof(argv[14]);

    strncpy(txt_fname, fname,  1024);
    strncat(txt_fname, ".txt", 1024);
    strncpy(fid_fname, fname,  1024);
    strncat(fid_fname, ".fid", 1024);

    printf ("SF=            %f MHz\n", SF);
    printf ("SW=            %f kHz\n", SW);
    printf ("P2_time=       %f us\n",  P2_time);
    printf ("ringdown_time= %f us\n", ringdown_time);
    printf ("P2_phase=      %f us\n", P2_phase);
    printf ("P1_phase=      %f us\n", P1_phase);
    printf ("tau=           %f us\n", tau);
    printf ("echo_points=   %d\n", echo_points);
    printf ("echo_loops=    %d\n", echo_loops);
    printf ("num_scans=     %d\n", num_scans);
    printf ("filename=      %s\n", fname);
    printf ("bypass fir=    %d\n", bypass_fir);
    printf ("adc_freq=      %f MHz\n",adc_freq);
    printf ("wait_time=     %f s\n", wait_time);

    // convert to MHz
    SW /= 1000.0;

    printf ("Using spinapi library version %s\n", pb_get_version());
    if(pb_init()) {
        printf ("Error initializing board: %s\n", pb_get_error());    
        return -1;
    }
    
    int num_points = echo_points*echo_loops;
    int top_real[num_points];
    int top_imag[num_points];
    
		
    pb_set_defaults();
    pb_set_clock(adc_freq);

    pb_zero_ram();    // clear RAM to all zeros
    pb_overflow(1,0); // reset the overflow counters
    pb_scan_count(1); // reset scan counter

    ///
    /// Program frequency and phase registers
    ///
    pb_start_programming(FREQ_REGS);
    pb_set_freq(SF);
    pb_stop_programming();

    pb_start_programming(COS_PHASE_REGS);
    pb_set_phase(0.0);
    pb_set_phase(90.0);
    pb_set_phase(180.0);
    pb_set_phase(270.0);
    pb_stop_programming();

    pb_start_programming(SIN_PHASE_REGS);
    pb_set_phase(0.0);
    pb_set_phase(90.0);
    pb_set_phase(180.0);
    pb_set_phase(270.0);
    pb_stop_programming();

    pb_start_programming(TX_PHASE_REGS);
    pb_set_phase(P1_phase);
    pb_set_phase(P2_phase);
    pb_stop_programming();

// User friendly names for the phase registers of the cos and sin channels
#define PHASE000 0
#define PHASE090 1
#define PHASE180 2
#define PHASE270 3

// User friendly names for the control bits
#define TX_ENABLE 1
#define TX_DISABLE 0
#define PHASE_RESET 1
#define NO_PHASE_RESET 0
#define DO_TRIGGER 1
#define NO_TRIGGER 0

    ///
    /// Set acquisition parameters
    ///
    int dec_amount;
    if (bypass_fir) {
        cmd = BYPASS_FIR;   
    }
    dec_amount = pb_setup_filters(SW, num_scans, cmd);
    double actual_SW = (adc_freq*1e6)/(double)dec_amount;
    printf("1e6 is %f\n", 1e6);
    printf ("Actual SW used is %f kHz (desired was %f kHz)\n", actual_SW/1000.0, SW*1000.0);

    P1_time   = 2.0*P2_time; // us

    top_time = ((double)echo_points)/(actual_SW/1e6); // top time in us
    printf("top_time = %f \n", top_time);
    
    P1_after1 = tau - top_time/2.0; printf("P1_after1 = %f \n", P1_after1);
    P1_after2 = tau + top_time/2.0; printf("P1_after2 = %f \n", P1_after2);

    if (echo_points == 0) {
        pb_set_num_points(RAM_SIZE);
        pb_set_scan_segments(1);
        do_top_trigger = NO_TRIGGER;
        printf ("Acquiring data in a single, continuous scan\n");
    }
    else {
        pb_set_num_points(echo_points);
        pb_set_scan_segments(echo_loops + 1); // +1 because 1st segment
        do_top_trigger = DO_TRIGGER;
        printf ("Acquiring only the tops of the echos\n");
    }

    ///
    /// Specify pulse program
    ///

//pb_inst_radio (freq, cos_phase, sin_phase, tx_phase, tx_enable, phase_reset, trigger_scan, flags,
//                inst, inst_data, length)

    pb_start_programming(PULSE_PROGRAM);

	// Reset phase initially, so that the phase of the excitation pulse will be
	// the same for every scan. This is the beginning of the scan loop
    scan_loop_label =
    pb_inst_radio(0,PHASE000, PHASE090, 0, TX_DISABLE, PHASE_RESET, NO_TRIGGER, 0x04,
                   LOOP, num_scans, 5.0*ms);

	// 90 degree pulse (P2)
    pb_inst_radio(0,PHASE000, PHASE090, 1, TX_ENABLE, NO_PHASE_RESET, NO_TRIGGER, 0x04,
                   CONTINUE, 0, P2_time * us);
    // wait for for the transient to subside...
    pb_inst_radio(0,PHASE000, PHASE090, 1, TX_DISABLE, NO_PHASE_RESET, NO_TRIGGER, 0x04,
                   CONTINUE, 0, ringdown_time * us);
    // ... then trigger the acquisition. 
    pb_inst_radio(0,PHASE000, PHASE090, 1, TX_DISABLE, NO_PHASE_RESET, DO_TRIGGER, 0x04,
                   CONTINUE, 0, tau * us);

    // These next three instructions will loop echo_loops times. 
    // 180 degree pulse (P1)    
    echo_loop_label =
    pb_inst_radio(0,PHASE000, PHASE090, 0, TX_ENABLE, NO_PHASE_RESET, NO_TRIGGER, 0x04,
                   LOOP, echo_loops, P1_time*us);
    // wait through half of the echo ...
    pb_inst_radio(0, PHASE000, PHASE090, 0, TX_DISABLE, NO_PHASE_RESET, NO_TRIGGER, 0x04,
                  CONTINUE, 0, P1_after1*us);
    // ... then trigger acquisition and wait through the other half. (If this is to be
    // a singlue continuous acquisition, do_top_trigger is 0 so a trigger will not
    // occur here)
    pb_inst_radio(0, PHASE000, PHASE090, 0, TX_DISABLE, NO_PHASE_RESET, do_top_trigger, 0x04,
                  END_LOOP, echo_loop_label, P1_after2*us);

    // Allow sample to relax before acuiring another scan
    pb_inst_radio(0,PHASE000, PHASE090, 0, TX_DISABLE, NO_PHASE_RESET, NO_TRIGGER, 0x00,
                   CONTINUE, 0, wait_time*ms);

    // Loop back and do scan again. This will occur num_scans times
    pb_inst_radio(0,PHASE000, PHASE090, 0, TX_DISABLE, PHASE_RESET, NO_TRIGGER, 0x00,
                   END_LOOP, scan_loop_label, 1.0*us);
    
    // Then stop the pulse program
    pb_inst_radio(0,PHASE000, PHASE090, 0, TX_DISABLE, NO_PHASE_RESET, NO_TRIGGER, 0x00,
                   STOP, echo_loop_label, 1.0*us);
    
    pb_stop_programming();

    ///
    /// Trigger pulse program
    ///
    pb_start();

    // Wait until scan is complete.
    printf ("Waiting for scan to complete...\n");
    int status;
    do {
        status=pb_read_status();
        printf ("status=0x%.2x (experiment in progress)\n", status);
        pb_sleep_ms(400);
    } while (status != 0x03); // 0x03 is STOPPED and RESET, this means the scan and pulse program are complete

    // Find out if overflows occurred in the ADC while capturing data
    pb_overflow(0, &of);
    printf ("adc overflow: %d\n", of.adc);

    ///
    /// Read data out of the RAM
    ///
    if(echo_points == 0){
       pb_get_data(RAM_SIZE, real, imag);
       // Write the data to an ASCII file
	   pb_write_ascii(txt_fname, RAM_SIZE, actual_SW, real, imag);
       // Write the data to a Felix file
       pb_write_felix(fid_fname, RAM_SIZE, actual_SW, SF, real, imag);
    }
    
    // For acquiring tops of loops, only write acquired points to files.
    else{ 
       pb_get_data(RAM_SIZE, real, imag);  //For USB Boards smallest read size is 8K
       // Read in RAM_SIZE, then copy the acquired points to file.
       int i;
       for(i=0; i<num_points; i++)
           top_real[i] = real[i];
       for(i=0; i<num_points; i++)
           top_imag[i] = imag[i];
       
       // Write the data to an ASCII file
	   pb_write_ascii(txt_fname, num_points, actual_SW, top_real, top_imag);
       // Write the data to a Felix file
       pb_write_felix(fid_fname, (num_points), actual_SW, SF, top_real, top_imag);
    }
    pb_stop();
    pb_close();

    return 0;
}

