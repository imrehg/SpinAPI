/**
 * singlepulse_nmr.c
 * This program is used to control the RadioProcessor series of boards.
 *
 * SpinCore Technologies, Inc.
 * www.spincore.com
 * $Date: 2008/05/21 15:06:02 $
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "spinapi.h"
#include "singlepulse_nmr.h"


int main(int argc, char* argv[])
{
    SCANPARAMS* myScan;
    
    int real[RAM_SIZE];         // (16*1024)
    int imag[RAM_SIZE];
    
    char fid_fname[FNAME_SIZE];
	char jcamp_fname[FNAME_SIZE];
	char ascii_fname[FNAME_SIZE];
    
	myScan = (SCANPARAMS*)malloc( sizeof(SCANPARAMS) );
	memset((void*)myScan, 0, sizeof(SCANPARAMS));
	
	if( processArguments(argc,argv,myScan) == 0)
	{
        
        if(pb_count_boards() <= 0)
        {
           if(myScan->verbose) printf("No RadioProcessor boards were detected in your system.\n");
           return BOARD_NOT_DETECTED;
        }
        
        if(myScan->verbose) 
           outputScanParams(myScan);
           
        strncpy(fid_fname, myScan->outputFilename, FNAME_SIZE);
        strncat(fid_fname, ".fid", FNAME_SIZE);
		strncpy(jcamp_fname, myScan->outputFilename, FNAME_SIZE);
		strncat(jcamp_fname, ".jdx", FNAME_SIZE);
		strncpy(ascii_fname, myScan->outputFilename, FNAME_SIZE);
		strncat(ascii_fname, ".txt", FNAME_SIZE);	
        
        pb_select_board( myScan->board_num );
        
        configureBoard(myScan); //Set board defaults.
        
        if(programBoard(myScan)!=0) //Program the board.
        {
           if(myScan->verbose) printf("Error: Failed to program board.\n");
           return PROGRAMMING_FAILED;
        }
        printf("pb_Start()\n");
        pb_start();
            
        if(myScan->verbose) 
            printf("Waiting for the data acquisition to complete.\n");
            
        while(pb_read_status() != BOARD_STATUS_IDLE) //Wait for the board to complete execution.
        {
            pb_sleep_ms(100);
        }
            
    	if(myScan->enable_rx)
		{
			pb_get_data(RAM_SIZE, real, imag);
				
			pb_write_felix(fid_fname, myScan->nPoints , myScan->actualSpectralWidth, myScan->spectrometerFrequency, real, imag);
			pb_write_ascii_verbose(ascii_fname, myScan->nPoints, myScan->actualSpectralWidth, myScan->spectrometerFrequency, real, imag);
			pb_write_jcamp(jcamp_fname, myScan->nPoints, myScan->actualSpectralWidth, myScan->spectrometerFrequency, real, imag);
		}
		
   	    pb_close();
	}

	free(myScan);
	
	return 0;
}


//--------------------------------------------------------------------
int processArguments(int argc, char* argv[], SCANPARAMS* scanParams)
{
	if(argc != NUM_ARGUMENTS+1)
	{
		printProperUse();
		return INVALID_NUM_ARGUMENTS;
	}
	
	scanParams->board_num         = atoi( argv[1] );
	scanParams->nPoints           = atoi( argv[2] );
	scanParams->spectrometerFrequency = atof( argv[3] );
	scanParams->spectralWidth     = atof( argv[4] );
	scanParams->pulseTime         = atof( argv[5] );
	scanParams->transTime         = atof( argv[6] );
	scanParams->repetitionDelay   = atof( argv[7] );
	scanParams->nScans            = atoi( argv[8] );
	scanParams->tx_phase          = atof( argv[9] );
	scanParams->outputFilename    = argv[10];
	scanParams->bypass_fir        = (unsigned short) atoi( argv[11] );
	scanParams->adcFrequency      = atof( argv[12] );
	scanParams->use_shape         = (unsigned short) atoi( argv[13] );
	scanParams->amplitude         = atof( argv[14] );
	scanParams->enable_tx         = atof( argv[15] );
	scanParams->enable_rx         = atof( argv[16] );
	scanParams->verbose           = atof( argv[17] );	
	scanParams->blankingEnable    = atoi( argv[18] );
	scanParams->blankingBit       = atoi( argv[19] );
	scanParams->blankingDelay     = atof( argv[20] );
	
	if( verifyArguments(scanParams, scanParams->verbose) != 0)
	    return INVALID_ARGUMENTS;
	
	return 0;
	
}

int verifyArguments(SCANPARAMS* scanParams, int verbose)
{
    if (pb_count_boards() > 0 && scanParams->board_num > pb_count_boards()-1 ) {
        if(verbose) printf ("Error: Invalid board number. Use (0-%d).\n", pb_count_boards()-1);
        return -1;    
    }
    
     if (scanParams->nPoints > RAM_SIZE || scanParams->nPoints < 1) {
        if(verbose) printf ("Error: Maximum number of points is %d.\n", RAM_SIZE);
        return -1;    
    }
    
    if (scanParams->nScans  < 1) {
        if(verbose) printf ("Error: There must be at least one scan.\n");
        return -1;    
    }
    
    if(scanParams->pulseTime < 0.065) {
        if(verbose) printf("Error: Pulse time is too small to work with board.\n");
        return -1;
    }
    
    if(scanParams->transTime < 0.065) {
        if(verbose) printf("Error: Transient time is too small to work with board.\n");
        return -1;
    }
    
    if(scanParams->amplitude < 0.0 || scanParams->amplitude > 1.0) {
        if(verbose) printf("Error: Amplitude value out of range.\n");
        return -1;
    }	
    
    return 0;
}

static inline void printProperUse()
{
	    printf ("Incorrect number of arguments, there should be %d. Syntax is:\n", NUM_ARGUMENTS);
        printf("--------------------------------------------\n");
        printf("Variable                       Units\n");
        printf("--------------------------------------------\n");
        printf("Board Number...................(0-%d)\n", pb_count_boards()-1);
        printf("Number of Points...............(1-16384)\n");
        printf("Spectrometer Frequency.............MHz\n");
        printf("Spectral Width.................kHz\n");
        printf("Pulse Time.....................us\n");
        printf("Transient Time.................us\n");
        printf("Repetition Delay...............s\n");
        printf("Number of Scans................(1 or greater)\n");
        printf("TX Phase.......................degrees\n");
        printf("Filename.......................Filename to store output\n");
        printf("Bypass FIR.....................(1 to bypass, or 0 to use)\n");
        printf("ADC Frequency..................ADC sample frequency\n");		
		printf("Shaped Pulse...................(1 to output shaped pulse, 0 otherwise)\n");		
		printf("Amplitude......................Amplitude of excitation pulse (0.0 to 1.0)\n");
        printf("Enable Transmitter Stage.......(1 turns transmitter on, 0 turns transmitter off)\n");
		printf("Enable Receiver Stage..........(1 turns receiver on, 0 turns receiver off)\n");
        printf("Enable Verbose Output..........(1 enables verbose output, 0 suppresses output)\n");
        printf("Use TTL Blanking...............(1 enables blanking, 0 disables blanking)\n");
        printf("Blanking TTL Flag Bits.........TTL Flag Bit(s) used in the blanking\n");
        printf("Blanking Delay.................Delay between de-blanking and the TX Pulse (ms)\n");
}
void outputScanParams(SCANPARAMS* myScan)
{
        printf("Filename: %s\n", myScan->outputFilename);
        printf("Board Number: %d\n", myScan->board_num);
        printf("Number of Points: %d\n", myScan->nPoints);
        printf("Number of Scans: %d\n", myScan->nScans);
        printf("Use shape: %d\n", myScan->use_shape);
        printf("Bypass FIR: %d\n", myScan->bypass_fir);
        printf("Amplitude: %lf\n", myScan->amplitude);
        printf("Spectrometer Frequency: %lf\n", myScan->spectrometerFrequency);
        printf("Spectral Width: %lf\n", myScan->spectralWidth);
        printf("TX Phase: %lf\n", myScan->tx_phase);
        printf("Pulse Time: %lf\n", myScan->pulseTime);
        printf("Trans Time: %lf\n", myScan->transTime);
        printf("Repetition Delay: %lf\n", myScan->repetitionDelay);
        printf("ADC Frequency: %lf\n", myScan->adcFrequency);
        printf("Enable Transmitter: %d\n", myScan->enable_tx);
        printf("Enable Receiver: %d\n", myScan->enable_rx);
        printf("Enable Verbose Output: %d\n", myScan->verbose);
        printf("Use TTL Blanking: %d\n", myScan->blankingEnable);
        printf("Blanking TTL Flag Bits: 0x%x\n", myScan->blankingBit);
        printf("Blanking Delay: %lf\n", myScan->blankingDelay);
}

void make_shape_data(float* shape_array, void* arg, void (*shapefnc)(float*,void*) )
{
     shapefnc(shape_array, arg);
}

void shape_sinc(float* shape_data, void* nlobe)
{
    static double pi = 3.1415926;
    int i;
    int lobes = *((int*)nlobe);
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

int configureBoard(SCANPARAMS* myScan) 
{
    float shape_data[1024];
    
    double actual_SW;
    double wait_time;
    double spectralWidth_MHZ = myScan->spectralWidth / 1000.0;
    
    int dec_amount;
    int cmd = 0;
    int num_lobes = 3;
    
    if(pb_init()) {
        printf ("Error initializing board: %s\n", pb_get_error());			
        return -1;
    }
    
    pb_set_defaults();
    pb_set_clock(myScan->adcFrequency);

    pb_zero_ram();    // clear RAM to all zeros
    pb_overflow(1,0); // reset the overflow counters
    pb_scan_count(1); // reset scan counter


	// Load the shape parameters
    make_shape_data(shape_data, (void*)&num_lobes, shape_sinc);
	pb_dds_load(shape_data, DEVICE_SHAPE);
	pb_set_amp(((myScan->enable_tx)?myScan->amplitude:0), 0);
	
    //
    /// Set acquisition parameters
    ///
    if (myScan->bypass_fir) {
        cmd = BYPASS_FIR;   
    }
    
    dec_amount = pb_setup_filters(spectralWidth_MHZ, myScan->nScans, cmd);
    pb_set_num_points(myScan->nPoints);
    
    if(dec_amount <= 0)
    {
       if(myScan->verbose) printf("Error: Invalid data returned from pb_setup_filters(). Please check your board.");
       return INVALID_DATA_FROM_BOARD;
    }
   
    actual_SW = (myScan->adcFrequency*1e6)/(double)dec_amount;
    myScan->actualSpectralWidth = actual_SW;
 
    wait_time = 1000.0 * (((double)myScan->nPoints)/actual_SW); // time in ms
    myScan->wait_time = wait_time;
    
    return 0;
}

int programBoard(SCANPARAMS* myScan)
{
    int start;
    int rx_start;
    int tx_start;
	
    pb_start_programming(FREQ_REGS);
    pb_set_freq(0); //Program Frequency Register 0 to 0
    pb_set_freq(myScan->spectrometerFrequency * MHz); //Register 1
    pb_set_freq( checkUndersampling(myScan, myScan->verbose) ); //Register 2
    pb_stop_programming();


	// control phases for REAL channel
    pb_start_programming(COS_PHASE_REGS);
    pb_set_phase(0.0);
    pb_set_phase(90.0);
    pb_set_phase(180.0);
    pb_set_phase(270.0);
    pb_stop_programming();

    // control phases for IMAG channel
    pb_start_programming(SIN_PHASE_REGS);
    pb_set_phase(0.0);
    pb_set_phase(90.0);
    pb_set_phase(180.0);
    pb_set_phase(270.0);
    pb_stop_programming();

    // control phases for output channel
    pb_start_programming(TX_PHASE_REGS);
    pb_set_phase(myScan->tx_phase);
    pb_stop_programming();

     ///
    /// Specify pulse program
    ///
	
    pb_start_programming(PULSE_PROGRAM);

   //If we have the transmitter enabled, we must include the pulse program to generate the RF pulse.
   if( myScan->enable_tx )
   {
       	// Reset phase initially, so that the phase of the excitation pulse will be the same for every scan.
       pb_inst_radio_shape(1,PHASE090, PHASE000, 0, TX_DISABLE, PHASE_RESET, NO_TRIGGER, myScan->use_shape,  myScan->amplitude,  0x00,
							CONTINUE, 0, 1.0*us);
                   
		//If blanking is enabled, we must add an additional pulse program interval to compesate for the time the power amplifier needs to "warm up" before we can generate the RF pulse.
    	if(myScan->blankingEnable)
    	{
	    	tx_start =    pb_inst_radio_shape(1,PHASE090, PHASE000, 0, TX_DISABLE, NO_PHASE_RESET, NO_TRIGGER,  0, 0, myScan->blankingBit | 0x01,
	                       LOOP, myScan->nScans, myScan->blankingDelay * ms);
	                       
	                       pb_inst_radio_shape(1,PHASE090, PHASE000, 0, TX_ENABLE, NO_PHASE_RESET, NO_TRIGGER,  myScan->use_shape, 0, myScan->blankingBit | 0x01,
	                       CONTINUE, 0, myScan->pulseTime * us);
        }
        else
        {
	        tx_start =     pb_inst_radio_shape(1,PHASE090, PHASE000, 0, TX_ENABLE, NO_PHASE_RESET, NO_TRIGGER,  myScan->use_shape, 0, 0x01,
	                       LOOP, myScan->nScans, myScan->pulseTime * us);
        }
        
    	// Output nothing for the transient time.
        pb_inst_radio_shape(1, PHASE090, PHASE000, 0, TX_DISABLE, NO_PHASE_RESET, NO_TRIGGER, 0, 0, 0x00,
							CONTINUE, 0, myScan->transTime * us);
    }
    
	//If we are enabling the receiver, we must wait for the scan to complete.
    if( myScan->enable_rx ) 
    {            
        rx_start = pb_inst_radio_shape(2,PHASE090, PHASE000, 0, TX_DISABLE, NO_PHASE_RESET, DO_TRIGGER, 0, 0, 0x02,
										CONTINUE, 0, myScan->wait_time * ms);
    }          
    
	//If the transmitter is enabled, we start from the TX section of the pulse program.
    if( myScan->enable_tx)
        start = tx_start;
    else if( myScan->enable_rx)
        start = rx_start;
    else
        return RX_AND_TX_DISABLED;
         
    // Now wait the repetition delay, then loop back to the beginning. Also reset the phase in anticipation of the next scan
    pb_inst_radio_shape(1,PHASE090, PHASE000, 0, TX_DISABLE, PHASE_RESET, NO_TRIGGER, 0, 0, 0x00,
						END_LOOP, start, myScan->repetitionDelay * 1000.0 * ms);

    // Stop execution of program.
    pb_inst_radio_shape(1,PHASE090, PHASE000, 0, TX_DISABLE, NO_PHASE_RESET, NO_TRIGGER, 0, 0, 0x00,
                   CONTINUE, 0, 1.0*us);
    pb_inst_radio_shape(1,PHASE090, PHASE000, 0, TX_DISABLE, NO_PHASE_RESET, NO_TRIGGER, 0, 0, 0x00,
                   STOP, 0, 1.0*us);
    
    pb_stop_programming();

    return 0;
}

double checkUndersampling(SCANPARAMS* myScan, int verbose)
{
    int folding_constant;
    double folded_frequency;
    double adc_frequency = myScan->adcFrequency;
    double spectrometer_frequency = myScan->spectrometerFrequency;
    double nyquist_frequency = adc_frequency / 2.0;
    
    if(verbose) printf("Specified Spectrometer Frequency: %f\n", spectrometer_frequency);
    
	if(spectrometer_frequency >  nyquist_frequency)
	{
		if( ((spectrometer_frequency/adc_frequency) - (int)(spectrometer_frequency / adc_frequency)) >= 0.5 )
			folding_constant = (int)ceil( spectrometer_frequency / adc_frequency ); 
		else
			folding_constant = (int)floor( spectrometer_frequency / adc_frequency );
    
		folded_frequency = fabs( spectrometer_frequency - ((double) folding_constant) * adc_frequency );
		
		if(verbose) printf("Undersampling Detected: Spectrometer Frequency (%.4lf MHz) is greater than Nyquist (%.4lf MHz).\n", spectrometer_frequency, nyquist_frequency);
		
		spectrometer_frequency = folded_frequency;
	}
	
    if(verbose) printf("Using Spectrometer Frequency: %lf MHz.\n", spectrometer_frequency);
	
    return spectrometer_frequency;
}
	
	
	
