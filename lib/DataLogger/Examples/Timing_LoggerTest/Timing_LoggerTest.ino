#include <DataLogger.h>

/*********************************************************
   Example file for datalogger object
   Fast binary logger

   M. Borgerson   5/13/2020
 **********************************************************/

#include <Datalogger.h>
// instantiate a datalogger object
DataLogger mydl;


//  The ISR handler for an interval timer must be at global scope,
//  and not in the DataLogger object scope.  THUS,  This function to
//  transfer back to the object so that the buffering can use all
//  the datalogger private variables.
void LoggerISR(void) {  // call the data logger collection ISR handler
  mydl.TimerChore();
}


#define SAMPLERATE 10000
#define BUFFERMSEC 800
char logfilename[64];

// use our own buffer on the T3.6
#define MAXBUFFER 200000
uint8_t mybuffer[MAXBUFFER];

bool logging = false;

// User-define data storage structure.
// A simple structure to hold time stamp #records written, and storage location
// The structure takes up 16 bytes to align millitime on 4-byte boundary
struct datrec {
  uint32_t unixtime;    // 1-second time/date stamp
  uint32_t millitime;   // millis() since collection start at time collection occurred
  uint32_t numrecords;  // number of structures collected
  uint8_t *cptr;        // pointer to last storage location to check out-of-bounds writes
};
const char compileTime [] = "  Compiled on " __DATE__ " " __TIME__;


TLoggerStat *lsptr;
uint32_t numrecs = 0;
elapsedMillis filemillis;

void setup() {
  uint32_t bufflen;

  pinMode(ledpin, OUTPUT);
  Serial.begin(9600);
  delay(100);

  Serial.print("\n\nData Logger Timing Example ");
  Serial.println(compileTime);

  mydl.SetDBPrint(true);  // turn on debug output
  if (!mydl.InitStorage()) { // try starting SD Card and file system
    // initialize SD Card failed
    fastBlink();
  }
  // now try to initialize buffer.  Check that request falls within
  // size of local buffer or if it will fit on heap
  bufflen = mydl.InitializeBuffer(sizeof(datrec), SAMPLERATE, BUFFERMSEC, mybuffer);
  if ((bufflen == 0) || (bufflen > MAXBUFFER)) {
    Serial.println("Not enough buffer space!  Reduce buffer time or sample rate.");
    fastBlink();
  }
  // Now attach our customized callback functions
  mydl.AttachCollector(&myCollector); // specify our collector callback function

  // If you attach no writer or a NULL pointer, by default all binary data is written
  
  mydl.AttachDisplay(&myBinaryDisplay, 5000); // display written data once per 5 seconds
}

void loop() {
  // put your main code here, to run repeatedly:
  char ch;

  mydl.CheckLogger();  // check for data to write to SD at regular intervals

  if (Serial.available()) {
    ch = Serial.read();
    if (ch == 'r')  StartLogging();
    if (ch == 'q') QuitLogging();
    if (ch == 's') GetStatus();
    if (ch == 'f') mydl.FormatSD(true); // format exFat for all sizes
    if (ch == 'd') mydl.ShowDirectory();
  }
  delay(2); // check 500 times/ second
}

void StartLogging(void) {
  Serial.println("Starting Logger.");
  logging = true;
  MakeFileName(logfilename);
  mydl.StartLogger(logfilename, 1000);  // sync once per second
  filemillis = 40; // Opening file takes ~ 40mSec
  Serial.print("\n");
}

void QuitLogging(void) {
  Serial.println("Stopping Logger.");
  mydl.StopLogger();
  logging = false;
}

void MakeFileName(char *filename) {
  sprintf(filename, "LOG_%02lu%02lu.bin", (now() % 86400) / 3600, (now() % 3600) / 60);
  Serial.printf("File name is:  <%s>\n", filename);
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
void GetStatus(void) {
  TLoggerStat *tsp;
  float mbytes;
  tsp =  mydl.GetStatus();

  mbytes = tsp->byteswritten / (1024.0 * 1024.0);
  Serial.println("\nLogger Status:");
  Serial.printf("MBytes Written: %8.2f\n", mbytes);
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
void myCollector(  void* vdp) {
  volatile struct datrec *dp;
  // Note: logger status keeps track of max collection time
  dp = (volatile struct datrec *)vdp;
  dp->millitime = filemillis; // save mSec wev'e been collecting
  dp->unixtime = now();

  dp->numrecords = numrecs;
  dp->cptr = (uint8_t *)dp; // save the address at which data is being stored
  numrecs++;
}

// called from the datalogger CheckLoggger function
void myBinaryDisplay( void* vdp) {
  struct datrec *dp;
  dp = (struct datrec *)vdp;
  TLoggerStat *tsp;
  tsp =  mydl.GetStatus(); // updates values collected at interrupt time

  if (!logging) return;
  Serial.printf("%8.3f,  ", dp->millitime / 1000.0);
  Serial.printf(" Records:%8lu  ", dp->numrecords);
  Serial.printf(" Overflows: %4lu\n", tsp->bufferoverflows);
  if ( (uint32_t )dp->cptr > (uint32_t)mybuffer + MAXBUFFER) {
    Serial.printf("Saved data outside buffer at %p\n", dp->cptr);
  }
}
