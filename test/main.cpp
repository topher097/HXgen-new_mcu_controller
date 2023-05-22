#include <DataLogger.h>

/*********************************************************
   Example file for datalogger object
   Fast binary logger to test big buffers in T4.1
   PSRAM
   Updated for multiple-logger capability 6/26/2020
   M. Borgerson   5/15/2020
   
 **********************************************************/

#include <Datalogger.h>
#include <Adafruit_HX8357.h>
#include <Adafruit_GFX.h>




// instantiate a datalogger object
DataLogger mydl;


//  The ISR handler for an interval timer must be at global scope,
//  and not in the DataLogger object scope.  THUS,  This function to
//  transfer back to the object so that the buffering can use all
//  the datalogger private variables.
void LoggerISR(void) {  // call the data logger collection ISR handler
  mydl.TimerChore();
}


#define SAMPLERATE 25000     // SAMPLES PER SECOND
#define BUFFERMSEC 100
char logfilename[64];


// use our own buffer on the T4.1
// With one PSRam chip, we get 8MB of buffer--so leave about a MByte
// for other uses
#define MAXBUFFER 7000000
uint8_t *mybuffer = (uint8_t *)0x70000000;

bool logging = false;
bool endplayback = true;
uint32_t pbrecnum, verifyerrors;



// User-define data storage structure.
// A simple structure to hold time stamp #records written, and storage location
// The structure takes up 16 bytes to align millitime on 4-byte boundary
struct dataSave {
  uint32_t microtime;   //  time in microseconds
  uint32_t millitime;   // millis() since collection start at time collection occurred
  uint32_t numrecords;  // number of structures collected
  uint8_t *cptr;        // pointer to last storage location to check out-of-bounds writes
};
const char compileTime [] = "  Compiled on " __DATE__ " " __TIME__;

uint32_t time_now_ms;
TLoggerStat *lsptr;
uint32_t numrecs = 0;
uint8_t *bufferend;
elapsedMillis filemillis;
elapsedMicros filemicros;

void make_file_name(char *filename) {
  sprintf(filename, "HX_%lu.bin", now());
  Serial.printf("File name is:  <%s>\n", filename);
}

void start_logging(void) {
  Serial.println("Starting Logger.");
  logging = true;
  make_file_name(logfilename);      // Create the log file name
  numrecs = 0;
  mydl.StartLogger(logfilename, 1000, &LoggerISR);  // sync once per second
  filemillis = 0; // Opening file takes ~ 40mSec
  filemicros = 0;
  Serial.print("\n");
}


void verify_file(void) {
  Serial.println("Verifying last file");
  endplayback = false;
  pbrecnum = 0;
  verifyerrors = 0;
  mydl.PlaybackFile(logfilename);

  while (!endplayback) {
    delay(2);
  }
  Serial.printf("Verification errors:  %lu\n", verifyerrors);
  Serial.println("Verification complete");
}

void quit_logging(void) {
  Serial.println("Stopping Logger.");
  mydl.StopLogger();
  logging = false;
}



// blink at 500Hz forever to signal unrecoverable error
void fastBlink(void) {
  while (1) {
    LEDON
    delay(100);
    LEDOFF
    delay(100);
  }
}

// can be called before, during, and after logging.
void get_status(void) {
  TLoggerStat *tsp;
  float freembytes, mbytes;
  tsp =  mydl.GetStatus();
  freembytes = tsp->spaceavailable / (1024.0 * 1024.0);
  mbytes = tsp->byteswritten / (1024.0 * 1024.0);
  Serial.println("\nLogger Status:");
  Serial.printf("MBytes Written: %8.2f  ", mbytes);
  Serial.printf(" SdCard free space: %8.2f MBytes\n", freembytes);
  Serial.printf("Collection time: %lu seconds\n", tsp->collectionend - tsp->collectionstart);
  Serial.printf("Max Collection delay: %lu microseconds\n", tsp->maxcdelay);
  Serial.printf("Average Write Time: %6.3f milliseconds\n", tsp->avgwritetime / 1000.0);
  Serial.printf("Maximum Write Time: %6.3f milliseconds\n\n", tsp->maxwritetime / 1000.0);

}

/***************************************************
     Callback function to handle user-defined collection
     and logging.
 ******************************************************/

// called from the datalogger timer handler
//  this version collects only timing data
void write_data_save_struct(  void* vdp) {
  volatile struct dataSave *dp;
  // Note: logger status keeps track of max collection time
  dp = (volatile struct dataSave *)vdp;
  dp->millitime = filemillis; // save mSec wev'e been collecting
  dp->microtime = filemicros;


  dp->numrecords = numrecs;
  dp->cptr = (uint8_t *)dp; // save the address at which data is being stored
  numrecs++;

  // // If first recordings, then print to serial monitor as format microtime, millitime, numrecords, cptr
  // if (numrecs < 20){
  //   Serial.printf("%lu, %lu, %lu, %p\n", dp->microtime, dp->millitime, dp->numrecords, dp->cptr);
  // }
  // else {
  //   quit_logging();
  // }
}

// called from playback
void my_verify(void *vdp) {
  struct dataSave *dp;
  dp = (struct dataSave *)vdp;
  if (dp == NULL) {
    Serial.println("End of file playback.");
    endplayback = true;
    return;
  }

  if (dp->numrecords != pbrecnum) {
    Serial.printf("Record number error at %lu", dp->numrecords);
    Serial.printf("  File: %lu  local: %lu\n", dp->numrecords, pbrecnum);
    pbrecnum = dp->numrecords;
    verifyerrors++;
  }
  pbrecnum++;
  if ((pbrecnum % 1000000) == 0)Serial.printf("Recnum: %lu\n", pbrecnum);

}

// called from the datalogger CheckLoggger function
void my_binary_display( void* vdp) {
  struct dataSave *dp;
  dp = (struct dataSave *)vdp;
  TLoggerStat *tsp;
  tsp =  mydl.GetStatus(); // updates values collected at interrupt time

  if (!logging) return;
  Serial.printf("%8.3f,  ", dp->millitime / 1000.0);
  Serial.printf(" Records:%10lu  ", dp->numrecords);
  Serial.printf(" Overflows: %4lu\n", tsp->bufferoverflows);
  if ( dp->cptr > bufferend) {
    Serial.printf("Saved data outside buffer at %p\n", dp->cptr);
  }
}

void setup() {
  uint32_t bufflen;

  pinMode(ledpin, OUTPUT);
  Serial.begin(115200);
  // Wait for serial port to connect. Needed for native USB port only
  while (!Serial) {}

  delay(500);

  Serial.print("\n\nData Logger Timing Example ");
  Serial.println(compileTime);

  mydl.SetDBPrint(true);  // turn on debug output
  // Add NULL parameter for new multiple-logger version
  if (!mydl.InitStorage(NULL)) { // try starting SD Card and file system
    // initialize SD Card failed
    fastBlink();
  }
  // now try to initialize buffer.  Check that request falls within
  // size of local buffer or if it will fit on heap
  bufflen = mydl.InitializeBuffer(sizeof(dataSave), SAMPLERATE, BUFFERMSEC, mybuffer);
  bufferend = mybuffer + bufflen;
  Serial.printf("End of buffer at %p\n", bufferend);
  if ((bufflen == 0) || (bufflen > MAXBUFFER)) {
    Serial.println("Not enough buffer space!  Reduce buffer time or sample rate.");
    fastBlink();
  }
  // Now attach our customized callback functions
  mydl.AttachCollector(&write_data_save_struct
); // specify our collector callback function

  // If you attach no writer or a NULL pointer, by default all binary data is written
  mydl.AttachDisplay(&my_binary_display, 5000); // display written data once per 5 seconds
  mydl.AttachPlayback(&my_verify);  // check for missing records

  Serial.println("Can press 'h' for a help menu at any time.");
  // make_file_name(logfilename);
  // Serial.println(logfilename);
  // delay(1000); // wait for serial monitor to start up
  // make_file_name(logfilename);
  // Serial.println(logfilename);
  // delay(1000); // wait for serial monitor to start up
  // make_file_name(logfilename);
  // Serial.println(logfilename);
  // delay(1000); // wait for serial monitor to start up

  // Clear the serial input buffer
  while (Serial.available()) {
    Serial.read();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  char ch;
  TLoggerStat *tsp;

  tsp =  mydl.GetStatus();
  mydl.CheckLogger();       // check for data to write to SD at regular intervals

  // Check for end of SD Card
  if (logging) {
    if (tsp->spaceavailable < (10240l * 1024)) { //we are within 10MB of end of card
      Serial.println("Halting logging.  Getting near end of SD Card.");
      quit_logging();
      get_status();
    }
  }

  // Take manual control of the logger
  if (Serial.available()) {
    ch = Serial.read();
    if (ch == 'r') start_logging();
    if (ch == 'q') quit_logging();
    if (ch == 's') get_status();
    if (ch == 'f') mydl.FormatSD(true); // format exFat for all sizes
    if (ch == 'd') mydl.ShowDirectory();
    if (ch == 'v') verify_file();
    if (ch == 'h') {
      // Display the help, with the cuurent available commands
      Serial.println("Available commands:");
      Serial.println("  r - Start logging");
      Serial.println("  q - Quit logging");
      Serial.println("  s - Get status");
      Serial.println("  f - Format SD Card");
      Serial.println("  d - Show directory");
      Serial.println("  v - Verify last file");
    }
  }
  delay(2); // check 500 times/ second
  // at max, we write chunks of data only 10 times in the interval defined
  // by the buffer storage time.  For 1 second of buffer, that means
  // we write about once every 100mSec.

}


