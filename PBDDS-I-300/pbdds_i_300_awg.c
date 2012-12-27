/*
 * This program demonstrates the use of the shaped pulse feature of the 
 * PulseBlasterDDS-300
 *
 * SpinCore Technologies, Inc.
 * www.spincore.com
 * $Date: 2007/07/17 17:54:22 $
 */

#include <stdio.h>
#include <math.h>
#include "spinapi.h"

#define CLOCK_FREQ 75.0

// User friendly names for the control bits
#define TX_ENABLE 1
#define TX_DISABLE 0
#define PHASE_RESET 1
#define NO_PHASE_RESET 0
#define DO_TRIGGER 1
#define NO_TRIGGER 0
#define USE_SHAPE 1
#define NO_SHAPE 0

void program_and_start(float amp1, float amp2, float freq);

void shape_make_ramp(float *shape_data);
void shape_make_sin(float *shape_data);
void shape_make_sinc(float *shape_data, int lobes);

int main(int argc, char **argv)
{
    float amp1, amp2, freq;

    // These arrays will hold the value of the shapes to be used for the
    // shaped pulse feature.
    float dds_data[1024];   // Waveform for the DDS signal itself (normally a sinewave)
    float shape_data[1024]; // Waveform for the pulse shape

    printf ("Using spinapi library version %s\n", pb_get_version());
    if(pb_init()) {
        printf ("Error initializing board: %s\n", pb_get_error());    
        system("pause");
        return -1;
    }
    pb_set_clock(CLOCK_FREQ);
    pb_set_defaults();
   
    // Set the contents of the arrays to the desired waveform
    // (see below for the definition of these functions)
    shape_make_sinc(shape_data, 3);
    shape_make_sin(dds_data);

    // And then load them on the board
    pb_dds_load(shape_data, DEVICE_SHAPE);
    pb_dds_load(dds_data, DEVICE_DDS);

    printf("\n");
    printf("This program demonstrates the shaped pulse feature of the RadioProcessor. It "
           "will output two sinc shaped pulses, with the amplitude you specify. You can "
           "also specify the RF frequency used. The TTL outputs are enabled during the "
           "pulse, and can be used to trigger your oscilloscope.\n\n"
           "This is is only the basics of what you can do. Please see the PulseBlasterDDS300 "
           "documentation and source code of this program for more details.\n\n");

    printf("Press CTRL-C at any time to quit\n\n");

    // Loop continuously, gathering parameters for the demo, and the programming
    // the board appropriately.
    while (1) {
        printf("Enter amplitude for pulse 1 (value from 0.0 to 1.0): ");
  	    scanf("%f",&amp1);

        printf("Enter amplitude for pulse 2 (value from 0.0 to 1.0): ");
  	    scanf("%f",&amp2);

        printf("Enter RF frequency (in MHz): ");
      	scanf("%f",&freq);

        program_and_start(amp1, amp2, freq);
    }
  
	// Release control of the board
	pb_close();

	printf("\n\n");
	system("pause");
	return 0;
}

// Program the PulseBlaster-I-300 board to run a simple demo of the shaped pulse
// feature, and then trigger the program. It will output two shaped pulses,
// as described above.
void program_and_start(float amp1, float amp2, float freq)
{
    int start;
    int ret;

    pb_set_amp(amp1, 0);
    pb_set_amp(amp2, 1);
    // There are two more amplitude registers that can be programmed, but are
    // not used for this demo program.
    //pb_set_amp(0.5, 2);
    //pb_set_amp(0.3, 3);

    // set the frequency for the sine wave
	pb_start_programming(FREQ_REGS);	
	pb_set_freq(freq*MHz);
	pb_stop_programming();

    // Set the TX phase to 0.0
	pb_start_programming(TX_PHASE_REGS);
	pb_set_phase(0.0); // in degrees
	pb_stop_programming();

    printf("Amplitude 1: %f\n", amp1);
    printf("Amplitude 2: %f\n", amp2);
    printf("Freqency:    %fMHz\n", freq);

	pb_start_programming(PULSE_PROGRAM);

    //pb_inst_dds_shape(freq, tx_phase, tx_enable, phase_reset, use_shape, amp, flags,
    //              inst, inst_data, length)

    // 10us shaped pulse, with amplitude set by register 0. TTL outputs on
	start =
    pb_inst_dds_shape(0,0, TX_ENABLE, NO_PHASE_RESET, USE_SHAPE, 0, 0x1FF, CONTINUE, 0, 10.0*us);

    if(start < 0) {
        printf("Error with first instruction: %s\n\n", pb_get_error());
        return;
    }

    // 20us shaped pulse, with amplitude set by register 1. TTL outputs on
	ret =
    pb_inst_dds_shape(0,0, TX_ENABLE, NO_PHASE_RESET, USE_SHAPE, 1, 0x1FF, CONTINUE, 0, 20.0*us);

    if(ret < 0) {
        printf("Error with second instruction: %s\n\n", pb_get_error());
        return;
    }

    // Output no pulse for 1ms. reset the phase. TTL outputs off. Execution
    // branches back to the beginning of the pulse program.
    ret =
    pb_inst_dds_shape(0,0, TX_DISABLE, PHASE_RESET, NO_SHAPE, 0, 0x00, BRANCH, start, 1.0*ms);

    if(ret < 0) {
        printf("Error with third instruction: %s\n\n", pb_get_error());
        return;
    }

	pb_stop_programming();

	pb_start();
	
    printf("Board Programmed Successfully!\n\n");
}

// The following functions show how to build up arrays with different shapes
// for use with the pb_dds_load() function.

double pi = 3.1415926;

// Make a sinc shape, for use in generating soft pulses.
void shape_make_sinc(float *shape_data, int lobes)
{
    int i;

    double x;
    double scale = (double)lobes*(2.0*pi);
    for(i=0; i < 1024; i++) {
        x = (double)(i-512)*scale/1024.0;
        shape_data[i] = sin(x)/x;
        if((x) == 0.0) {
            shape_data[i] = 1.0;
        }
    }    
}

// Make one period of a sinewave
void shape_make_sin(float *shape_data)
{
    int i;

    for(i=0; i < 1024; i++) {
        shape_data[i] = sin(2.0*pi*((float)i/1024.0));
    }    
}

// Make a ramp function. This is an example of a different kind of shape you
// could potentially for a shaped pulse
void shape_make_ramp(float *shape_data)
{
    int i;

    for(i=0; i < 1024; i++) {
        shape_data[i] = (float)i/1024.0;        
    }    
}
