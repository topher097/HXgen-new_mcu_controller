#include <MTP.h>
#include <Storage.h>
#include <usb1_mtp.h>


#include <DataLogger.h>
#include <CMDHandler.h>

// Add stuff for MTP
#define DO_DEBUG 0


MTPStorage_SD storage;
MTPD       mtpd(&storage);

SdFs *fsptr;

// instantiate slow and fast datalogger objecta
DataLogger slowdl, fastdl;
CMDHandler mycmds;

uint32_t SpikeCount;

bool MTPLoopOK = true;   // start up with MTP Active---no loop only when logging
bool autostarted = false;

#define LEDON digitalWriteFast(ledpin, HIGH);
#define LEDOFF digitalWriteFast(ledpin, LOW);
const char compileTime [] = " Compiled on " __DATE__ " " __TIME__;


const char *fastname = "Fast";
const char *slowname = "Slow";
const char *extype = "005";

uint32_t eventpacket = 0;   // set to positive value when event occurs

void SlowISR(void) {  // call the data logger collection ISR handler
  slowdl.TimerChore();
}

void FastISR(void) {  // call the data logger collection ISR handler
  fastdl.TimerChore();
}


#define FASTRATE 1000
#define SLOWRATE 1



// use our own buffer on the T3.6
// slow data is 8 bytes/second and we need at two seconds
// the logger will default to a minimum of 5120 bytes.
#define MAXBUFFER 5120
uint8_t slowbuffer[MAXBUFFER];
uint8_t fastbuffer[MAXBUFFER *10];

char fastfilename[64], slowfilename[64];

bool logging = false;
uint64_t fsreserved;

elapsedMillis el_autostart;

// raw data for buffer  16 bytes long

struct slowdat {
  time_t unixtime;
  uint32_t spikecount;
};

// processed data storage structure
struct fastdat {
  uint32_t packetcount;
  float temperature;
};


#define AUTODELAY 120000 // 2 minutes
void setup() {
  uint32_t bufflen;
  TLoggerStat *fasttsp;

  pinMode(ledpin, OUTPUT);

  Serial.begin(9600);
  delay(500);

  Serial.print("\n\nDouble File event Data Logger  ");
  Serial.println(compileTime);
 // fastdl.SetDBPrint(true);
 // slowdl.SetDBPrint(true);
  fsptr = fastdl.InitStorage(NULL);// try starting SD Card and file system
  if (fsptr == NULL) {
    // initializing SD Card failed
    fastBlink();
  }
  slowdl.InitStorage(fsptr);  // use the same file system as fast logger

  // now try to initialize fastdata buffer.  Check that request falls within
  // size of local buffer or if it will fit on heap
  bufflen = fastdl.InitializeBuffer(sizeof(fastdat), FASTRATE, 1200, fastbuffer);
  if ((bufflen == 0) || (bufflen > MAXBUFFER *10)) {
    Serial.println("Not enough buffer space!  Reduce buffer time or sample rate.");
    fastBlink();
  }

  // now try to initializeslow data buffer.  Check that request falls within
  // size of local buffer or if it will fit on heap
  bufflen = slowdl.InitializeBuffer(sizeof(slowdat), SLOWRATE, 1200, slowbuffer);
  if ((bufflen == 0) || (bufflen > MAXBUFFER)) {
    Serial.println("Not enough buffer space!  Reduce buffer time or sample rate.");
    fastBlink();
  }

  LEDON
  delay(1000);  // Need a delay while SDFat checks free clusters for first time
                // This takes a lot longer on FAT32 disks with smaller clusters
  fasttsp =  fastdl.GetStatus();
  LEDOFF

  Serial.printf("Free Space: %6.2f\n", (float)(fasttsp->spaceavailable) / (1024.0 * 1024.0));
  Serial.println("Calculating reserved space.");
  fsreserved = fasttsp->spaceavailable / 20; //reserve the last 5% of space available;

  // Now attach our customized callback functions
  fastdl.AttachCollector(&fastCollector); // specify our fast rate collector callback function
  slowdl.AttachCollector(&slowCollector); // specify our slow rate collector callback function

  // There are no attached writers, since the loggers are saving unaltered binary data

  slowdl.AttachDisplay(&slowBinaryDisplay, 5000); // display written slow data once per 5 seconds
  fastdl.AttachDisplay(&fastBinaryDisplay, 1000); // display written fast data once per second

  slowdl.AutoFile(slowname, extype, 6);// new slow file generated every 6 hours
  fastdl.AutoFile(fastname, extype, 6);// new fast file generated every 6 hours

  StartMTP();
  delay(200);
  el_autostart = 0;
  InitCommands();
}

void InitCommands(void) {
  mycmds.AddCommand(&StartLogging, "SL", 0);
  mycmds.AddCommand(&QuitLogging, "QL", 0);
  mycmds.AddCommand(&ShowBothStatus, "SS", 0);
  mycmds.AddCommand(&Directory, "DI", 0);
  mycmds.AddCommand(&FormatDisk, "DF", 0);
}

void FormatDisk(void *cmdline){
  fastdl.FormatSD(false); //use FAT32 for multiple loggers
  fsptr = fastdl.InitStorage(NULL);// try starting SD Card and file system
  if (fsptr == NULL) {
    // initializing SD Card failed
    fastBlink();
  }
  slowdl.InitStorage(fsptr);  // use the same file system as fast logger
}

void Directory(void *cmdline) {
  // slow and fast loggers show files in same directory
  slowdl.ShowDirectory();

}

void loop() {
  // put your main code here, to run repeatedly:

  TLoggerStat *tsp;
  do {
    tsp =  fastdl.GetStatus();
    delay(100);
  } while (tsp->spaceavailable == 0);
 // Serial.println("got fastdle.status");
  if (logging){

    LEDON
    fastdl.CheckLogger(); // check for data to write to SD at regular intervals
    slowdl.CheckLogger();
    delay(50);
    LEDOFF;
  }

    // for demo program, we don't check to see if approaching end of free space.

  if (mycmds.CheckCommandInput()) {
    el_autostart = 0;   // reset autostart timer
  }

  if (MTPLoopOK) {
    mtpd.loop();
  }
  if (!autostarted) {
    if (el_autostart > AUTODELAY) {
      StartLogging(NULL);
    }
  }

}

void StartMTP(void) {
  Serial.println("Starting MTP Responder");
  usb_mtp_configure();
  if (!Storage_init(fsptr)) {
    Serial.println("Could not initialize MTP Storage!");
    fastBlink();
  } else MTPLoopOK = true;
}



void StartLogging(void *cmdline) {
  TLoggerStat *fasttsp, *slowtsp;
  slowtsp =  slowdl.GetStatus();
  fasttsp =  fastdl.GetStatus();
  Serial.println("Starting Loggers.");
  logging = true;
  autostarted = true;
  MTPLoopOK = false;
  SpikeCount = 0;
  fastdl.MakeFileName(fastname, extype);
  strncpy(fastfilename, fasttsp->filename, strlen(fasttsp->filename));
  Serial.printf("Starting fast logger with file name: %s\n", fastfilename);  
  
  if(!fastdl.StartLogger(fastfilename, 1000, &FastISR)){
    Serial.println("Could not start fast logger!");
    fastBlink();
  }
  Serial.println("Started fast logger");
  
  slowdl.MakeFileName(slowname, extype);
  strncpy(slowfilename, slowtsp->filename, strlen(slowtsp->filename));
  Serial.printf("Starting slow logger with file name: %s\n", slowfilename);
  if(!slowdl.StartLogger(slowfilename, 1000, &SlowISR)){
    Serial.println("Could not start slow logger!");
    fastBlink();
  }

  Serial.println("Started slow logger");
  LEDON
  delay(500);
  Serial.print("\n");
  LEDOFF
}

void QuitLogging(void *cmdline) {
  Serial.println("Stopping Logger.");
  slowdl.StopLogger();
  fastdl.StopLogger();
  logging = false;
  MTPLoopOK = true;
}


// blink at 5Hz forever to signal unrecoverable error
void fastBlink(void) {
  while (1) {
    LEDON
    delay(100);
    LEDOFF
    delay(100);
  }
}

// can be called before, during, and after logging.
void ShowBothStatus(void *cmdline) {
  TLoggerStat *tsp;
  tsp =  fastdl.GetStatus();

  Serial.println("\nFast Logger Status:");
  Serial.printf("Bytes Written: %lu\n", tsp->byteswritten);
  
  Serial.printf("Collection time: %lu seconds\n", tsp->collectionend - tsp->collectionstart);
  Serial.printf("Max Collection delay: %lu microseconds\n", tsp->maxcdelay);
  Serial.printf("Average Write Time: %6.3f milliseconds\n", tsp->avgwritetime / 1000.0);
  Serial.printf("Maximum Write Time: %6.3f milliseconds\n\n", tsp->maxwritetime / 1000.0);

  // now do it again for the slow data logger
  tsp =  slowdl.GetStatus();

  Serial.println("\nSlow Logger Status:");
  Serial.printf("Bytes Written: %lu\n", tsp->byteswritten);
  Serial.printf("Collection time: %lu seconds\n", tsp->collectionend - tsp->collectionstart);
  Serial.printf("Max Collection delay: %lu microseconds\n", tsp->maxcdelay);
  Serial.printf("Average Write Time: %6.3f milliseconds\n", tsp->avgwritetime / 1000.0);
  Serial.printf("Maximum Write Time: %6.3f milliseconds\n\n", tsp->maxwritetime / 1000.0);  


}

/**********************************************************
   For the demo, I simulate a temperature signal
    as  T = 20.0 + random(100)/100.0 -0.05;
    or a 20-degree signal with about 0.01 degree P-P of random noise
    and an occasional spike of 0.14 degrees.

 ******************************************************/
float SimTemp(void){
  float simtemp;
  simtemp = 20.0 + random(100)/10000.0 - 0.005; // .01 degrees P-P noise
  if(random(1000) > 995) simtemp+= 0.14; // an occasional spike
  return simtemp;
}

/***************************************************
     Callback function to handle fast logger collection
     and buffering
    struct fastdat {
      uint32_t packetcount;
      float temperature;
    };

*******************************************************/
void fastCollector(  void* vdp) {
  struct fastdat *rp;
  static uint32_t recordnumber = 0;
 
  float simtemp = SimTemp();

  rp =  (struct fastdat *)vdp;
  rp->packetcount = recordnumber;
  rp->temperature = simtemp;
 
  if(simtemp > 20.12){
    SpikeCount++; // increment  global spike counter
  } 
  recordnumber++; 

}

void fastBinaryDisplay(void *vdp){
  struct fastdat *dp;
  dp  = (struct fastdat *)vdp;
  if (!logging) return;
  Serial.printf("Pkt:  %lu  Temp: %6.3f\n", dp->packetcount,dp->temperature);
}

char *TString(time_t tm) {
  static char dstr[12];
  // sprintf(dstr,"%02u/%02u/%04u       ",month(tm),day(tm),year(tm));
  sprintf(dstr, "%02u:%02u:%02u", hour(tm), minute(tm), second(tm));
  return dstr;
}


/************************************
 * 
struct slowdat {
  time_t unixtime;
  uint32_t spikecount;

 }

  The slow collector function is called at 10Hz
  It buffers up the unix time and spike count

// called from the slowlogger CheckLoggger function
 ***********************************************/

void slowCollector(void *vdp){
struct slowdat *rp;
  rp =  (struct slowdat *)vdp;
  
  rp->unixtime = now();
  rp->spikecount = SpikeCount;
  SpikeCount = 0;  // reset count for next interval
  
}
void slowBinaryDisplay( void* vdp) {
  struct slowdat *dp;
  dp  = (struct slowdat *)vdp;
  if (!logging) return;
  Serial.printf("%s\t Spike count: %lu\n", TString(dp->unixtime), dp->spikecount);

}
