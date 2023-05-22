/*******************************************************************************
*  Header for data logger object
*  Change Log:
*  6/22/2020   Changes to allow multiple data loggers sharing
                      a common file system
*       Add ponter to global timer chore as parameter to Startlogger
*       Add pointer to file system as parameter to InitStorage
*   6/23/2020  	Remove MTP stuff.   Best handled in main program
******************************************************************************/

#ifndef  DATALOGGER_H
#define DATALOGGER_H

#include <Arduino.h>

#include <SdFat.h>
#include <TimeLib.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MAXNAMELENGTH  64
#define MAXSTRUCTSIZE 1024

// define how close we can write to end of SD card free spaceavailable
// set to 128KB now
#define FSMARGIN   131072

extern void LoggerISR(void);

// Since an interval timer ISR must be at global level, you need to include the 
//  following code in your program :
//   void LoggerISR(void) {  // call the data logger collection ISR handler
//    	myLogger.TimerChore(); // change 'myLogger to the name of your logger  
//    }
//  a pointer to this function is added to the StartLogger function

const int ledpin = 13;

#define LEDON  digitalWriteFast(ledpin, HIGH);
#define LEDOFF digitalWriteFast(ledpin, LOW);


#define LOGGER_VERSION  0.93

#define SD_CONFIG SdioConfig(FIFO_SDIO)

typedef struct TLoggerStat {
	uint64_t		byteswritten;				// number of bytes written to output file
	uint64_t		spaceavailable;			// number of bytes still available on SD card volume

	time_t		nextfiletime;
	uint32_t		collectionstart;
	uint32_t		collectionend;
	float			version;
	uint32_t		collectionrate;			// samples saved per second
	uint32_t		filestartmilli;
	uint32_t		filecurrentmilli;

	uint8_t		*bufferptr;						// location of data buffer
	uint32_t		maxcdelay;					// maximum time to collect data structure in microseconds
	uint32_t		fileinterval;
	float			avgwritetime;				// average time to write to SD card in milliseconds
	float			maxwritetime;				// maximum write delay in milliseconds
	uint32_t 	bufferoverflows;			// if non-zero, there are probably discontinuities in the saved data
	uint16_t		maxchunksready;

	bool      	islogging;						// true if a logging operation is in progress
	char   		filename[MAXNAMELENGTH];   // name of the most recent or current file
	char   		fnamebase[MAXNAMELENGTH];
	char  		fext[10];
	
} TLoggerStat;

typedef struct TLoggerConfig{	
	uint32_t autodelay;
	uint32_t collectionrate;
	uint32_t fileinterval;
	uint16_t serialnumber;
	
	char fnamebase[MAXNAMELENGTH];
	char fext[10];
	
} TLoggerConfig;

class DataLogger
{
	protected:
	private:
	bool OpenDataFile(const char *filename);
	void CheckNewFile(void);
//	uint8_t teststruct[MAXSTRUCTSIZE];
	
	uint8_t fattype;

	SdFs      sdx;   // for fat32/ExFat SD Card
	//SdioCard  sdc;

	FsFile  dataFile; 
	FsFile *fileptr; 
	SdFs *sdptr;

  
	uint32_t cmicrotime, unixseconds;
	uint16_t  displayinterval;
	elapsedMillis eldisplay;  // display interval timer
	elapsedMicros elwrite;    // write time collection
	elapsedMillis elsync;
	elapsedMillis filemillis;   // msec since file was opened
  
	void (*cptr)(void*);  // pointer to collection function-- parameter is pointer to data structure
	uint16_t (*wptr)(void*, void*);  // pointer to write function--parameters are pointer to data structure return data
	void (*pbptr)(void*);  // pointer to playback function.
	void (*dptr)(void *);  // pointer to display function. Parameter is pointer to data structure


	bool buffonheap = false;
	uint32_t fsyncinterval = 1000;
  
	volatile uint32_t bufferoverflows;
	volatile uint32_t maxcdelay;   // maximum time to collect data structure in microseconds

	float     wrtimesum = 0;  // sum of write delays in microseconds
	uint32_t  wravgcount = 0;

	bool dbprint = false;
	IntervalTimer  dlTimer;


	time_t filestarttime;
	TLoggerStat mystatus;
	TLoggerConfig myconfig;
	
	// private variables for buffer management
	// The buffer is broken up into MAXCHUNKS of memory
	//  Each chunk is an integer multiple of the data structure size.
	//  The number of structures per chunk is determined in the InitializeBuffer 
	//  function.
	uint32_t  collectionrate = 1000;
	uint32_t  structsize;  // size of the user-defined data structure in BYTES
	uint8_t   *buffptr = NULL;    
	uint32_t  bufflength = 0;      // length in BYTES
  

	#define MAXCHUNKS 10
	volatile uint8_t *lastptr = NULL;
	uint32_t chunksize;   // number of bytes in each chunk
	uint8_t *chunkPtrs[MAXCHUNKS];
	uint32_t structsperchunk;
	volatile uint16_t readychunks;
	volatile uint16_t collectchunk, writechunk;
	volatile uint32_t collectidx, writeidx; // index to storage location in buffer--which may be LARGE
	volatile uint8_t  *collectPtr;  	// pointer to record storage location in buffer
	uint8_t  *writePtr;		// pointer to record to write to file
	
	
	//  TO DO:  figure out how to handle the increased number of bytes (and increased write times)
	//                 if the user decides to us a writer function that outputs text, instead of binary data.

// default to using USB Serial IO for input and message output
	Stream *iosptr = &Serial;
	

	
	void ClearStatus(void);  // clear all status at start of session
	void WriteBinaryChunks(uint16_t numready); // write binary chunks when no writer function
	void WriteUserChunks(uint16_t numready); // write chunks record-by-record using writer function

	public:
		
		//  Default constructor
		DataLogger(){
			buffptr = NULL;
			cptr = NULL;
			dptr = NULL;
			wptr = NULL;
			pbptr = NULL;
			bufflength = 0;
			mystatus.collectionstart = 0; 
			mystatus.collectionend = 0;
		}
		
		~DataLogger(){
				if(buffonheap){
					if(buffptr) delete[] buffptr;
				}
		}
		
		// this function is called from the global level interval timer ISR.
		//  It has access to alll the class variables and will call the collector function.
		void TimerChore(void);
	
	
		void MakeFileName(const char *fbase, const char *ext);		
		
		//  Tell the data logger to initialize the storage system.  This generally means starting up SD card driver and checking for a working card.
		//  If the input parameter is NULL, a new file system is initialized.  If fptr points to a valid file system, the data logger saves the pointer for its
		//  own use.  In either case, the file system pointer is returned.
		SdFs* InitStorage(SdFs *fptr);

		// Ask data logger to initialize a buffer to hold data while loop() is writing to SD Card
		// crate is the collection rate, number of samples per second
		uint32_t InitializeBuffer(uint16_t stsize, uint32_t crate, uint16_t mSecLen, uint8_t *ibufferptr);
		void xstartlogger(void); // just a marker because Notepad++ function list won't detect following functions 

		// This function must be called from the main loop() function to monitor the collection process 
		//  and write data to the SD Card as necessary.  It should probably be called at least 20 times
		//  per second.  The function will return immediately if no logging is in progress.  If your loop function
		//  does something that takes hundreds of milliseconds (Such as waiting for user input),  that code
		//  should call CheckLogger while waiting if logging is in progress.
		void CheckLogger(void);
		
		// This function takes as input a pointer to a function that will handle the data collection.  The function 
		//  is called from the datalogger timer interrupt handler.   It will pass a pointer to a collection structure
		//  to the collection function.   The function collects whatever data is needed and stores it in the structure.
		//  The user must be sure that all the data written falls within the size of the structure pointed to.
		//   Example:   mylogger.AttachCollector(&mycollector);
		void  AttachCollector(void (*afptr)(void *));
		
		// This function takes as input a pointer to a function that will handle writing data to the SD Card.  The 
		//  function will be called from the CheckLogger function---which is called from the loop() function.
		//  The Attachwriter function will be called with a pointer to a data structure and a pointer to a File.  
		//  Your function can then convert the binary structure data to strings which are written to the file.
		//   If you never call this function or if you call it with a NULL pointer, the logger writes binary 
		//   structure data by default.  returns number of bytes written in user function
		//   Example:   mylogger.AttachWriter(&myASCIIWriter);
		void AttachWriter(uint16_t (*afptr)(void *, void *));
		
		// This function takes as input a pointer to a function that will handle playback of data on the SD Card.  
		// The  function will be called from the CheckLogger function---which is called from the loop() function.
		//  The Attached callback function will be called with a pointer to a data structure.
		//   
		//   Example:   mylogger.AttachPlayback(&myPlayback);
		void AttachPlayback(void(*afptr)(void *));
		
		 // This function takes as input a pointer to a function that will handle display of the data while
		 //  collection is in progress.  The   function will be called from the CheckLogger function---which is called
		 //  from the loop() function.  The display function will be called with a pointer to a data structure.  
		//  Your function can then convert the binary structure data to strings and write them to the console,
		//   plot them on a display,   etc, etc.  The function is called every <displayinterval> milliseconds.
		//   If you never call this function or if you call it with a NULL pointer, the display function is not called;
		//   Example:   mylogger.AttachDisplay(&myDisplay, 1000); // show data once per second.
		void AttachDisplay(void (*afptr)(void *), uint16_t dispinterval);
		
		
		// Set up automatic new file generation with interval in hours.  A new file
	    // have a name of the form namebase_YYMMDDHH.ext. where the date
		// and time become part of the name so that duplicate file names are avoided.
		// New files are started at even multiples of the interval.
		void AutoFile(const char *namebase,  const char *ext, uint16_t interval);
		
		//  This function starts the logging operation, storing the data in the specified SD file.
		//  The data logger will call the sync() function every <syncinterval> seconds
		//  to update the file directory.  That will guarantee that the file is readable up to the
		//  point of the last sync if power is lost or the SD card fills up
		//  The function returns true if the file is opened and logging has started
		//  We now pass in a pointer to the global-level timer chore----which will call our
		//  own object-level timer chore
		bool StartLogger(const char *filename, uint16_t syncInterval, void(*afptr)());
		//  This function stops the data logging and closes the file.
		bool  StopLogger(void);
		
		//  This function plays back a recorded file.  This only works when data logging is not in progress.
		//  If the file name is empty or the pointer is NULL, the last recorded file from the current logging
		//  session will be used.  If the file cannot be found, the function returns false.  The attached playback
		//  function has to know the data structure when playing binary files.  ASCII files will be sent to
		//  the playback function in segments the size of the current data record.
		bool  PlaybackFile(char  *filename);
		
		// return a pointer to a loggerstatus record
		TLoggerStat *GetStatus(void);
		
	    //  Resets the average and maximum write times and collection times
		//  total bytes written and buffer overflows are not cleared
		void ResetStatus(void);	
		
		// Verbose printout of logger status record
		void ShowStatus(void);

		void SetDBPrint(bool dbpval);
		
		void SetStream(Stream *sptr);


		void ShowDirectory(void);
		
		//  This function will format the SD Card, using EXFat if requested.    
		//
		//   USE EXTREME CARE  TO ONLY CALL THIS FUNCTION IF YOU ARE WILLING 
		//   TO LOSE THE DATA ON THE 
		//
		bool  FormatSD(bool useEXFat);
	
	// The configuration stuff is still a work in progress as of 6/24/2020
		void LoadConfig(void);
		void SaveConfig(void);
		void ShowConfig(void);

}; // end of class header



#ifdef __cplusplus
}
#endif


#endif // DATALOGGER_H
