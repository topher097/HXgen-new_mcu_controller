#include <DataLogger.h>

/*********************************************************
   Example file for datalogger object
   medium speed Analog  logger with ASCII file output

   M. Borgerson   5/13/2020
   Update 5/18/2020

   Updated 6/26/2020 for multiple-logger version
 **********************************************************/

#include <DataLogger.h>
// instantiate a datalogger object
DataLogger mydl;


//  The ISR handler for an interval timer must be at global scope,
//  and not in the DataLogger object scope.  THUS,  This function to
//  transfer back to the object so that the buffering can use all
//  the datalogger private variables.
void LoggerISR(void) {  // call the data logger collection ISR handler
  mydl.TimerChore();
}


#define SAMPLERATE 4000
#define BUFFERMSEC 1200
char logfilename[64];

// use our own buffer on the T3.6
#define MAXBUFFER 220000
uint8_t mybuffer[MAXBUFFER];

bool logging = false;

// User-define data storage structure.
// A simple structure to hold time stamp #records written, and storage location
// The structure takes up 16 bytes to align millitime on 4-byte boundary
struct datrec {
  uint32_t unixtime;    // 1-second time/date stamp
  uint32_t millitime;   // millis() since collection start when collection occurred
  uint16_t avals[4];    // i6 byte record length
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

  Serial.print("\n\nData Logger Example ");
  Serial.println(compileTime);
  analogReadResolution(12);
  // Setting pinMode to INPUT_DISABLE  turns off weak digital keeper resistors that
  // can affect inputs when connected to a moderately high output impedance device.
  // Thanks to @JBeale for discovering this.
  pinMode(A0, INPUT_DISABLE);  // 4 ADC inputs without digital "keeper" drive on pin
  pinMode(A1, INPUT_DISABLE);
  pinMode(A2, INPUT_DISABLE);
  pinMode(A3, INPUT_DISABLE);
  mydl.SetDBPrint(true);  // turn on debug output
  if (!mydl.InitStorage(NULL)) { // try starting SD Card and file system
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

  mydl.AttachWriter(&myASCIIWriter); // logger saves Ascii data that you generate
  mydl.AttachDisplay(&myASCIIDisplay, 5000); // display collected data once per 5 seconds
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
  mydl.StartLogger(logfilename, 1000 &LoggerISR);  // sync once per second
  filemillis = 40; // Opening file takes ~ 40mSec

  Serial.print("\n");
}

void QuitLogging(void) {
  Serial.println("Stopping Logger.");
  mydl.StopLogger();
  logging = false;
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
  float mbytes;
  tsp =  mydl.GetStatus();

  mbytes = tsp->byteswritten / (1024.0 * 1024.0);
  Serial.println("\nLogger Status:");
  Serial.printf("Collection time: %lu seconds  ", tsp->collectionend - tsp->collectionstart);
  Serial.printf("MBytes Written: %8.2f\n", mbytes);
  Serial.printf("Max chunks ready: %u ",tsp->maxchunksready);
  Serial.printf("Overflows: %lu ", tsp->bufferoverflows);
  Serial.printf("Max Collect delay: %lu uSecs\n", tsp->maxcdelay);
  Serial.printf("Avg Write Time: %6.3f mSec   ", tsp->avgwritetime / 1000.0);
  Serial.printf("Max Write Time: %6.3f mSec \n\n", tsp->maxwritetime / 1000.0);

}

/***************************************************
     Callback function to handle user-defined collection
     and logging.
 ******************************************************/

// called from the datalogger timer handler
//  this version collects  4 analog signals with basic Arduino analog input
void myCollector(  void* vdp) {
  volatile struct datrec *dp;
  // Note: logger status keeps track of max collection time
  dp = (volatile struct datrec *)vdp;
  dp->millitime = filemillis; // save mSec wev'e been collecting
  dp->unixtime = now();
  dp->avals[0] = analogRead(A0);
  dp->avals[1] = analogRead(A1);
  dp->avals[2] = analogRead(A2);
  dp->avals[3] = analogRead(A3);
}

void myASCIIDisplay( void* vdp) {
  char *dp;
  dp = (char *)vdp;
  if (!logging) return;
  Serial.printf("%s\n",  dp);
}


// Used to write ASCII data.  If you attach this
// function it will write data to the file in the  ASCII format
// you specify.  It is called once for each record saved.
uint16_t myASCIIWriter(void *bdp, void* rdp) {
  static char dstr[80];
  char** sptr;    // pointer to the address where we need to store our string pointer
  sptr = (char**) rdp;
  struct datrec *dp;

  dp = (struct datrec *)bdp;  //type cast once to save typing many times
  // rdp is the ADDRESS of a pointer object
  // simple ascii format--could use a complex date/time output format
  sprintf(dstr, "%8lu, %6lu, %4u, %4u, %4u, %4u", dp->unixtime, dp->millitime, dp->avals[0],
                        dp->avals[1], dp->avals[2],dp->avals[3]);

  *sptr = &dstr[0]; // pass back address of our string
  return strlen(dstr);  // Datalogger will write the string to the output file.
}
