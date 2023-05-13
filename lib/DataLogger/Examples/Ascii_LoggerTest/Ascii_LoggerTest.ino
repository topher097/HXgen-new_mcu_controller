#include <DataLogger.h>

/*********************************************************
   Example file for datalogger object
   M. Borgerson   5/1/2020

   Updated to play back file  5/15/2020
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


#define SAMPLERATE 100
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
  uint32_t millitime;   // millis() since collection start when collection occurred
  uint32_t numrecords;  // number of structures collected
  uint8_t *cptr;        // Will get a pointer to last storage location
};
const char compileTime [] = "  Compiled on " __DATE__ " " __TIME__;


TLoggerStat *lsptr;
uint32_t numrecs = 0;
uint32_t fstartmilli;
bool endplayback = true;
void setup() {
  uint32_t bufflen;

  pinMode(ledpin, OUTPUT);
  Serial.begin(9600);
  delay(100);

  Serial.print("\n\nData Logger Example ");
  Serial.println(compileTime);
  //Storage_init();  // MTP not ready yet
  mydl.SetDBPrint(true);  // turn on debug output
  if (!mydl.InitStorage()) { // try starting SD Card and file system
    // initialize SD Card failed
    fastBlink();
  }
  // now try to initialize buffer.  Check that request falls within
  // size of local buffer or if it will fit on heap
  bufflen = mydl.InitializeBuffer(sizeof(datrec), SAMPLERATE, 800, mybuffer);
  if ((bufflen == 0) || (bufflen > MAXBUFFER)) {
    Serial.println("Not enough buffer space!  Reduce buffer time or sample rate.");
    fastBlink();
  }
  // Now attach our customized callback functions
  mydl.AttachCollector(&myCollector); // specify our collector callback function

  // If you attach no writer or a NULL pointer, by default all binary data is written

  mydl.AttachWriter(&myASCIIWriter); // logger saves Ascii data that you generate
  mydl.AttachDisplay(&myASCIIDisplay, 5000); // display collected data once per 5 seconds

  mydl.AttachPlayback(&myASCIIPlayback); // Play back ascii file
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
    if (ch == 'p') Playbackfile();
    if (ch == 'd') mydl.ShowDirectory();
  }
  delay(50); // check 20 times per second
}

void StartLogging(void) {
  Serial.println("Starting Logger.");
  logging = true;
  MakeFileName(logfilename);
  mydl.StartLogger(logfilename, 1000);  // sync once per second
  lsptr = mydl.GetStatus();
  fstartmilli = lsptr->filestartmilli; // millis() at file open
  Serial.printf("File Start Millis() = %lu \n", fstartmilli);
  Serial.print("\n");
}

void QuitLogging(void) {
  Serial.println("Stopping Logger.");
  mydl.StopLogger();
  logging = false;
}

void Playbackfile(void) {
  Serial.println("File Playback");
  endplayback = false;
  mydl.PlaybackFile(logfilename);
  while (!endplayback) {
    delay(2);
  }
}


void MakeFileName(char *filename) {
  sprintf(filename, "LOG_%02lu%02lu.csv", (now() % 86400) / 3600, (now() % 3600) / 60);
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
  tsp =  mydl.GetStatus();
  Serial.println("\nLogger Status:");
  Serial.printf("Bytes Written: %lu\n", tsp->byteswritten);
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
  dp->unixtime = now();
  dp->millitime = millis() - fstartmilli; // save mSec into collection
  dp->numrecords = numrecs;
  dp->cptr = (uint8_t *)dp; // save the address at which data is being stored
  numrecs++;
}


void myASCIIDisplay( void* vdp) {
  char *dp;
  dp = (char *)vdp;
  if (!logging) return;
  Serial.printf("disp ptr: %p  Data: %s\n", dp, dp);
}

// display playbck data.  Don't bother with unixtime
// assumes that newline "\n" is in file
void myASCIIPlayback( void* vdp) {
  char *dp;
  dp = (char *)vdp;
  if(vdp == NULL){
    endplayback = true;
    Serial.println("File playback complete.");  
  } else {
    Serial.write(dp, sizeof(datrec));
  }
}


// Used to write ASCII data.  If you attach this
// function it will write data to the file in the  ASCII format
// you specify.  It is called once for each record saved.
uint16_t myASCIIWriter(void *bdp, void* rdp) {
  static char dstr[64];
  char** sptr;    // pointer to the address where we need to store our string pointer
  sptr = (char**) rdp;
  struct datrec *dp;

  dp = (struct datrec *)bdp;  //type cast once to save typing many times
  // rdp is the ADDRESS of a pointer object
  // simple ascii format--could use a complex date/time output format
  sprintf(dstr, "%8lu, %8lu, %8lu, %p\n", dp->unixtime, dp->millitime, dp->numrecords, dp->cptr);

  *sptr = &dstr[0]; // pass back address of our string
  return strlen(dstr);  // Datalogger will write the string to the output file.
}
