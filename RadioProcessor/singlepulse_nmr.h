/**
 * singlepulse_nmr.h
 * This program is used to control the RadioProcessor series of boards.
 * This file contains constants and data structures used by spinnmr.c. 
 *
 * SpinCore Technologies, Inc.
 * www.spincore.com
 * $Date: 2008/05/21 15:04:05 $
 */

#define NUM_ARGUMENTS 20
#define RAM_SIZE (16*1024)
#define FNAME_SIZE 256

#define BOARD_STATUS_IDLE 0x3
#define BOARD_STATUS_BUSY 0x6


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
#define AMP0 0

// Error return values for spinnmr
#define INVALID_ARGUMENTS      -1
#define BOARD_NOT_DETECTED     -2
#define INVALID_NUM_ARGUMENTS  -3
#define RX_AND_TX_DISABLED     -4
#define INVALID_DATA_FROM_BOARD -5
#define PROGRAMMING_FAILED     -6


typedef struct SCANPARAMS
{
	char* outputFilename;
	
	int board_num;
	
	unsigned int nPoints;
	unsigned int nScans;
	unsigned short bypass_fir;
	unsigned short use_shape;
	unsigned short enable_rx;
	unsigned short enable_tx;
	unsigned short verbose;
	
	double spectrometerFrequency;
	double spectralWidth;
	double actualSpectralWidth;
	double pulseTime;
	double transTime;
	double repetitionDelay;
	double adcFrequency;
	double amplitude;
    double tx_phase;
    
    double wait_time;
    
    char blankingEnable;
    char blankingBit;
    double blankingDelay;
    
} SCANPARAMS;

//--------------------------------------------------------------------	

void make_shape_data(float* shape_array, void* arg, void (*shapefnc)(float*,void*) ); //This function will generate the shape data for the board using the shapefnc argument.

void shape_sinc(float* shape_array, void* nlobe); //Creates shape data. To be used with make_shape_data

int processArguments(int argc, char* argv[], SCANPARAMS* scanParams); //Process argc and argv and filles in the scanParams structure.

int verifyArguments(SCANPARAMS* scanParams, int verbose); //CHecks the boundary conditions of the arguments.

void outputScanParams(SCANPARAMS* myScan); //Prints the SCANPARAMS structure.

int configureBoard(SCANPARAMS*); //Sets the board defaults.

int programBoard(SCANPARAMS*); //Programs the phase and frequency registers as well as the pulse program.

double checkUndersampling(SCANPARAMS*,int); //Checks to see if undersampling is needed. (Spectral Frequency > Nyquist)

static inline void printProperUse(); //Prints out the usage data for the executable

//--------------------------------------------------------------------
