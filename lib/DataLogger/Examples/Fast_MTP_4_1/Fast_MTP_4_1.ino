#include <DataLogger.h>

/*********************************************************
   Example file for datalogger object
   Fast binary logger to test big buffers in T4.1
   PSRAM

   M. Borgerson   5/15/2020
 **********************************************************/

#include <DataLogger.h>
// instantiate a datalogger object
DataLogger mydl;

// Add stuff for MTP
#define DO_DEBUG 1
#include "MTP.h"
#include "usb1_mtp.h"

MTPStorage_SD storage;
MTPD       mtpd(&storage);
bool MTPLoopOK = true;   // start up with MTP Active---no loop only when logging

//  The ISR handler for an interval timer must be at global scope,
//  and not in the DataLogger object scope.  THUS,  This function to
//  transfer back to the object so that the buffering can use all
//  the datalogger private variables.
void LoggerISR(void) {  // call the data logger collection ISR handler
  mydl.TimerChore();
}


#define SAMPLERATE 200000
#define BUFFERMSEC 1000
char logfilename[64];


// use our own buffer on the T4.1
// With one PSRam chip, we get 8MB of buffer--so leave about a MByte
// for other uses
#define MAXBUFFER 7000000
uint8_t *mybuffer = (uint8_t *)0x70000000;

bool logging = false;
bool endplayback = true;
uint32_t pbrecnum, verifyerrors;
uint64_t fsreserved;
// User-define data storage structure.
// A simple structure to hold time stamp #records written, and storage location
// The structure takes up 16 bytes to align millitime on 4-byte boundary
struct datrec {
  uint32_t microtime;    //  time in microseconds
  uint32_t millitime;   // millis() since collection start at time collection occurred
  uint32_t numrecords;  // number of structures collected
  uint8_t *cptr;        // pointer to last storage location to check out-of-bounds writes
};
const char compileTime [] = "  Compiled on " __DATE__ " " __TIME__;

SdFs *fsptr;
TLoggerStat *lsptr;
uint32_t numrecs = 0;
uint8_t *bufferend;
elapsedMillis filemillis;
elapsedMicros filemicros;

void setup() {
  uint32_t bufflen;
  TLoggerStat *tsp;
  const char *autoname = "fast";
  const char *extype = "001";


  pinMode(ledpin, OUTPUT);
  Serial.begin(9600);

 // while(!Serial);
  delay(500);
  Serial.print("\n\nData Logger Timing Example ");
  Serial.println(compileTime);

  mydl.SetDBPrint(true);  // turn on debug output
  fsptr = mydl.InitStorage(NULL);// try starting SD Card and file system
  if (fsptr == NULL) { 
    // initializing SD Card failed
    fastBlink();
  }

  LEDON
  delay(1000);  // Need a delay while SDFat checks free clusers for first time
  tsp =  mydl.GetStatus();
  LEDOFF

  Serial.printf("Free Space: %6.2f\n", (float)(tsp->spaceavailable) / (1024.0 * 1024.0));
  Serial.println("Calculating reserved space.");
  fsreserved = tsp->spaceavailable / 20; //reserve the last 5% of space available;


  StartMTP();
  /************************************************
   * Initialize MTP  
   * Needs USB TYPE MTP + Serial
   *********************************************
  usb_mtp_configure();
  if(!Storage_init(fsptr)) {
    Serial.println("Could not initialize MTP Storage!"); 
    fastBlink();
   }
*************************************************/
  // now try to initialize buffer.  Check that request falls within
  // size of local buffer or if it will fit on heap
  bufflen = mydl.InitializeBuffer(sizeof(datrec), SAMPLERATE, BUFFERMSEC, mybuffer);
  bufferend = mybuffer + bufflen;
  Serial.printf("End of buffer at %p\n", bufferend);
  if ((bufflen == 0) || (bufflen > MAXBUFFER)) {
    Serial.println("Not enough buffer space!  Reduce buffer time or sample rate.");
    fastBlink();
  }

  LEDON

  Serial.printf("reserved SD Card space = %6.2fMBytes\n", (float)fsreserved / 1000000.0);
  // Now attach our customized callback functions

    mydl.AttachCollector(&myCollector); // specify our collector callback function

    // If you attach no writer or a NULL pointer, by default all binary data is written
    mydl.AttachDisplay(&myBinaryDisplay, 30000); // display written data once per 30 seconds
    mydl.AttachPlayback(&myVerify);  // check for missing records
    mydl.AutoFile(autoname, extype, 300);

  delay(200);
}


void loop() {
  // put your main code here, to run repeatedly:
  char ch;
  TLoggerStat *tsp;
  do{
    tsp =  mydl.GetStatus();
    delay(1);
  } while(tsp->spaceavailable == 0);

  // can't depend on tsp->islogging---it doesn't quit until just a few MB left!
  if (logging)mydl.CheckLogger(); // check for data to write to SD at regular intervals
  if ((tsp->spaceavailable < fsreserved) && logging) { //we are near end of card
    Serial.println("Halting logging.  Getting near end of SD Card.");
    Serial.printf("Space Available = %6.1f Space reserved = %6.1f\n", 
                             (float)tsp->spaceavailable / (1024.0 * 1024.0), (float)fsreserved / (1024.0 * 1024.0));
    QuitLogging();
    ShowStatus();
    logging = false;
  }
  if (Serial.available()) {
    ch = Serial.read();
    if (ch == 'r') StartLogging();
    if (ch == 'q') QuitLogging();
    if (ch == 's') ShowStatus();
    if (ch == 'f') mydl.FormatSD(true); // format exFat for > 32GB
    if (ch == 'd') mydl.ShowDirectory();
    if (ch == 'm') StartMTP();
    if (ch == 'v') VerifyFile();
    if (ch == 'x') LogAndVerify();
  }
 // delay(2); // check 500 times/ second
  // at max, we write chunks of data only 10 times in the interval defined
  // by the buffer storage time.  For 1 second of buffer, that means
  // we write about once every 100mSec.
  if(MTPLoopOK){
    mtpd.loop();
  }

}

void StartMTP(void){
  Serial.println("Starting MTP Responder");
  usb_mtp_configure();
  if(!Storage_init(fsptr)) {
    Serial.println("Could not initialize MTP Storage!"); 
    fastBlink();
   } else MTPLoopOK = true;
}

void LogAndVerify(void) {
  TLoggerStat *tsp;

  MTPLoopOK = false;
  StartLogging();
  while (logging) {
    delay(5);
    tsp =  mydl.GetStatus();
    if (tsp->spaceavailable < fsreserved) { //we are within 10MB of end of card
      Serial.println("Halting logging.  Getting near end of SD Card.");
      QuitLogging();
      ShowStatus();
    }
    mydl.CheckLogger();  // check for data to write to SD at regular intervals

  }
  VerifyFile();
  MTPLoopOK = true;
}

void StartLogging(void) {
  Serial.println("Starting Logger.");
  logging = true;
  MTPLoopOK = false;
  numrecs = 0;
  mydl.StartLogger("", 1000, &LoggerISR);  // sync once per second
  filemillis = 0; // Opening file takes ~ 40mSec
  filemicros = 0;
  Serial.print("\n");

}


void VerifyFile(void) {
  TLoggerStat *tsp;
  tsp =  mydl.GetStatus();
  MTPLoopOK = false;
  Serial.printf("Verifying last file: %s\n", tsp->filename);
  endplayback = false;
  pbrecnum = 0;
  verifyerrors = 0;
  mydl.PlaybackFile(tsp->filename);
  while (!endplayback) {
    delay(2);
  }
  Serial.printf("Verification errors:  %lu\n", verifyerrors);
  Serial.println("Verification complete");
  MTPLoopOK = false;
}

void QuitLogging(void) {
  Serial.println("Stopping Logger.");
  mydl.StopLogger();
  logging = false;
  MTPLoopOK = true;
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
void ShowStatus(void) {
  TLoggerStat *tsp;
  float freembytes, mbytes;
  tsp =  mydl.GetStatus();
  freembytes = (float)tsp->spaceavailable / (1024.0 * 1024.0);
  mbytes = (float)tsp->byteswritten / (1024.0 * 1024.0);
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
void myCollector(  void* vdp) {
  volatile struct datrec *dp;
  // Note: logger status keeps track of max collection time
  dp = (volatile struct datrec *)vdp;
  dp->millitime = filemillis; // save mSec wev'e been collecting
  dp->microtime = filemicros;


  dp->numrecords = numrecs;
  dp->cptr = (uint8_t *)dp; // save the address at which data is being stored
  numrecs++;
}

// called from playback
void myVerify(void *vdp) {
  struct datrec *dp;
  dp = (struct datrec *)vdp;
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
void myBinaryDisplay( void* vdp) {
  struct datrec *dp;
  dp = (struct datrec *)vdp;
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
