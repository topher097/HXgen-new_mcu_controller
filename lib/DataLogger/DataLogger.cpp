/*************************************************************
   C++ code for data logger object

************************************************************/
#include "DataLogger.h"

//#include "SdFat.h"
//#include "sdios.h"

//#include "ExFatlib\ExFatLib.h"
#include "arm_math.h"

time_t mygetTeensy3Time() {
  return Teensy3Clock.get();
}


void  dateTime(uint16_t* date, uint16_t* time) {
  // use the year(), month() day() etc. functions from timelib

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(year(), month(), day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(hour(), minute(), second());
}


/*************************************************************************
  Public methods
************************************************************************/
// Initialize the SD Card.  Uses Fat32 of ExFat, depending on type of card.
// Requires SDFat 2.0.
// With change in SdFatConfig.h, SDFat should also handle ExFat file system
bool DataLogger::InitStorage(void) {
  if (!sdx.begin(SdioConfig(FIFO_SDIO))) {
    if (dbprint)iosptr->printf("\nSD File  system initialization failed.\n");
    return false;
  }
  fattype = sdx.fatType();
  if (fattype == FAT_TYPE_EXFAT) {
    if (dbprint)iosptr->printf("Card Type is exFAT\n");
  } else {
    if (dbprint)iosptr->printf("  Card Type is FAT%u\n", fattype);
  }
  if (dbprint)iosptr->printf("File system initialization done.\n");

  setSyncProvider(mygetTeensy3Time);
  delay(50);
  if (timeStatus() != timeSet) {
    if (dbprint)iosptr->println("Unable to sync with the RTC");
  } else {
    if (dbprint)iosptr->println("RTC has set the system time");
    unixseconds = now();
  }

  sdptr = &sdx;
  SdFile::dateTimeCallback(dateTime);
  mystatus.version = VERSION;
  mystatus.spaceavailable = (uint64_t) sdx.vol()->sectorsPerCluster() * sdx.vol()->freeClusterCount() * 512;
  return true;

}


void DataLogger::ShowDirectory(void) {
  iosptr->print("\nData Logger Directory\n");
  sdx.vol()->ls(iosptr, LS_SIZE | LS_DATE |  LS_R);
  iosptr->print("\n");
}

bool DataLogger::OpenDataFile(const char *filename) {

  if (! dataFile.open(filename, O_WRITE | O_CREAT | O_TRUNC)) {
    if (dbprint)iosptr->println("Open File failed");
    return false;
  }
  // pre-allocate 1GB
  uint64_t alloclength = (uint64_t)(1024l * 1024l * 1024l);
  if (dbprint)iosptr->print("Pre-allocating 1GB file space \n");
  if (!dataFile.preAllocate(alloclength)) {
    if (dbprint)iosptr->println("Allocation failed. Proceeding anyway.");
  } else {
    if (dbprint)iosptr->println("Allocation succeeded.");
  }
  if (dbprint)iosptr->printf("Opened  File %s\n ", filename);

  return true;
}

// don't allow sd card write rates above 20MB/sec
#define MAXSDCRATE  1024l*1024l*20l
// At 256K samples/second, you only have ~4 microseconds to collect data
#define MAXCOLLECTIONRATE 256000

// Ask data logger to initialize a buffer to hold data while loop() is writing to SD Card
// The requested size of the buffer will be (stsize * collectionrate * mSecLen)/1024 bytes.
//  If buffptr is NULL, the data logger will allocate the buffer on the heap,  otherwise it will
//  use the storage pointed  to by ibuffptr.   The function returns the number of BYTES
//  occupied by the buffer.   It is the responsibility of the user to verify the validity of this
//  size.  If heap allocation failed, the return size is zero.  If not on the heap, the user
//  must check the returned size against the memory available in the input ibufferptr.
uint32_t DataLogger::InitializeBuffer(uint16_t stsize, uint32_t crate, uint16_t mSecLen, uint8_t *ibufferptr) {
  uint32_t bufferneeded;
  uint32_t   sdcrate, i;
  structsize = stsize;
  collectionrate = crate;
  mystatus.collectionrate = crate;
  uint64_t buffneeded64;
  // calculate memory needed based on structure size, collection rate, and desired buffer time

	if(mSecLen > 4096){
		if (dbprint)iosptr->println("Buffer time limited to 4.096 Seconds");  
		return 0;
	}
  	sdcrate = structsize * crate;
	if(sdcrate  >   MAXSDCRATE){
		if (dbprint)iosptr->println("SC Write rate needed is too high!");
	return 0;
	}
  
	if(crate > 256000){
		if (dbprint)iosptr->println("Collection rate is too high.");
		return 0;	  
	}
// added type cast on structsize to make sure compiler doesn't use 32-bit shortcuts
  buffneeded64 = ((uint64_t)structsize * collectionrate * mSecLen);
  
  // nee to make sure this works as intended
  if(buffneeded64 > 0x0FFFFFF00){ // calculation will overflow in uint32_t
		if (dbprint)iosptr->println("Possible overflow in buffer size calculation");
		return 0;
  }
	// if we get this far, we shouldn't have overflow or impossible-to-meet requirements
  bufferneeded = (structsize * collectionrate * mSecLen)/ 1000;
  // for small records and slow logging, we default to 10 chunks of 512 bytes
  if(bufferneeded <5120) bufferneeded = 5120;
  if (ibufferptr != NULL) {
    buffptr = (uint8_t *)ibufferptr;
    mystatus.bufferptr = (uint8_t*)ibufferptr;
  } else {  // allocate buffer on heap
    buffptr = new uint8_t[bufferneeded];
    // Check for heap allocation error
    if (dbprint)iosptr->printf("Buffer allocation on heap returned %p.\n", buffptr);
    if (buffptr == NULL) {
      if (dbprint)iosptr->println("Buffer allocation on heap failed.");
      return 0;
    }
  }
  // now divide the buffer into chunks and initialize pointers to the chunks
  // ensure chunksize is even multiple of structure size
  chunksize = ((bufferneeded / MAXCHUNKS) / structsize) * structsize;
  structsperchunk = chunksize / structsize;
  bufflength = chunksize * MAXCHUNKS;

  // now initialize the various chunk management variables
  for (i = 0; i < MAXCHUNKS; i++) {
    chunkPtrs[i] = buffptr + i * chunksize;			
	//if(dbprint)iosptr->printf("Chunk [%lu] at %p\n", i, chunkPtrs[i]);
  }
  lastptr = buffptr;
  collectchunk = 0;
  writechunk = 0;
  readychunks = 0;
  collectidx = 0;

  if (dbprint)iosptr->println("Buffer initialization complete.");
  if (dbprint)iosptr->printf("buffer at: %p with length %lu \n", buffptr, bufflength);
  if (dbprint)iosptr->printf("chunk size is %lu   or %lu data records\n", chunksize, structsperchunk);

  return bufflength;
}


// TimerChore is called regularly when dlTimer is active
void DataLogger::TimerChore(void) {
  uint32_t cmicrostart, cmicrodelay;

  cmicrostart = micros();
  
  cptr((void*)collectPtr); // call data collection function

  collectPtr += structsize;
  collectidx++;
  if (collectidx >= structsperchunk) { // time to move to next chunk
    collectidx = 0;  // start at beginning of next chunk
    readychunks++;   // signal to writer in checklogger
    if (readychunks >= MAXCHUNKS) { // buffer overflow
      bufferoverflows++;
      // collectchunk stays the same, so data is overwrtten
    } else { // move to next chunk for collection
      collectchunk++;
      if (collectchunk >= MAXCHUNKS) { // wrap around at buffer end
        collectchunk = 0;
      }
      collectPtr = chunkPtrs[collectchunk];  // get pointer to start of new chunk
    }
  }

  cmicrodelay = micros() - cmicrostart;  // see how long collection took
  if (cmicrodelay > mystatus.maxcdelay)maxcdelay = cmicrodelay;
}


// write data chunks as binary data  writechunk has index
// of first ready-to-write chunk
void DataLogger::WriteBinaryChunks(uint16_t numready) {
  uint16_t i;
  uint32_t etime;
  float wravg;

  LEDON
  for (i = 0; i < numready; i++) {
    etime = micros();
    writePtr = chunkPtrs[writechunk];
    dataFile.write((uint8_t *)writePtr, chunksize); // write the data to file
    lastptr = writePtr;
    writechunk++;
    if (writechunk >= MAXCHUNKS) writechunk = 0;
    mystatus.byteswritten += chunksize;
    etime = micros() - etime;
    if (etime > mystatus.maxwritetime) mystatus.maxwritetime = etime;
    wrtimesum += etime;
    wravgcount++;
    wravg = wrtimesum / wravgcount;
    mystatus.avgwritetime = wravg;
  }  // end of for loop
  LEDOFF

}

// write data chunks record by record using wptr function
// this is used when averaging, triggered logging, or ASCII file output
void DataLogger::WriteUserChunks(uint16_t numready) {
  uint32_t i, j, nbytes;
  uint32_t etime;
  float wravg;
  uint8_t *rptr;
  LEDON
  for (i = 0; i < numready; i++) {
    etime = micros();
    writePtr = chunkPtrs[writechunk]; //get pointer to start of next chunk
    // write out data record-by record
    for (j = 0; j < structsperchunk; j++) {
      rptr = NULL;
	  // user write function returns size of data written
	  // this can be larger or smaller than the collection record size
      nbytes = wptr(writePtr, &rptr); // send the ADDRESS of rptr
      if(rptr == NULL){
        //Serial.print("NULL "); 
      }else {
        if(dataFile) dataFile.write(rptr, nbytes);
        lastptr = rptr;  // point to last saved data
      }
      writePtr += structsize;  // move write pointer to next data structure
      mystatus.byteswritten += nbytes;
    }

    writechunk++;  // move to next chunk, with wrap around if needed
    if (writechunk >= MAXCHUNKS) writechunk = 0;

    etime = micros() - etime;
	// now update our status record
    if (etime > mystatus.maxwritetime) mystatus.maxwritetime = etime;
    wrtimesum += etime;
    wravgcount++;
    wravg = wrtimesum / wravgcount;
    mystatus.avgwritetime = wravg;
  }  // end of for(i... loop
  LEDOFF

}


//  CheckLogger is called from loop() to save buffered data to file and do display
void DataLogger::CheckLogger(void) {
  uint16_t numready;
  uint8_t *dispptr;
  uint32_t logger_safe_read;
  uint16_t readyHold;
  if (eldisplay > displayinterval) { // display last written record
    dispptr = lastptr;
    eldisplay -= displayinterval;	
    dptr((void*)dispptr); // call user-written display function can take some msec!
  }
  if (elsync > fsyncinterval) { // sync file on a regular basis	
	dataFile.sync();
    elsync -= fsyncinterval;
  }
  numready = 0;
 // Note1:   This alternative from @defragster avoids masking interrupts
 // The LDREXW STREXW pair change things in a way I'm still reseaching
 //  if an interrupt occurs between the pair.
 //  Note2:  You have to include "arm_math.h" to get this to compile
  do { // account for interrupts that may alter vars during read.
    __LDREXW(&logger_safe_read);
    readyHold = numready;
    numready += readychunks;
    readychunks -= (numready - readyHold);  // account for number found
    mystatus.bufferoverflows = bufferoverflows;
    mystatus.maxcdelay = maxcdelay;
  } while ( __STREXW(1, &logger_safe_read));

  
  if(numready > mystatus.maxchunksready) mystatus.maxchunksready= numready;
  
  // check amount to write against free space to prevent writing past end of SDC
  if(((uint32_t)numready* chunksize) > (mystatus.spaceavailable-FSMARGIN)){
	if (dbprint)iosptr->println("Reached end of SD Card ");  
	return;
  } else {  
  // adjust free space count
  mystatus.spaceavailable -= (uint32_t)numready* chunksize ;
  // pull data from queue of  chunks and write to file
	if (wptr != NULL) {
		if (numready > 0)  WriteUserChunks(numready);
	}  else {
		if (numready > 0) WriteBinaryChunks(numready);
	}
  }
}

void  DataLogger::AttachCollector(void (*afptr)(void *)) {
  cptr = afptr;
}

void  DataLogger::AttachDisplay(void (*afptr)(void *), uint16_t dispinterval) {
  dptr = afptr;
  displayinterval = dispinterval;  // set display interval in mSec.
}

void DataLogger::AttachWriter(uint16_t(*afptr)(void *, void*)) {
  wptr = afptr;
}

void DataLogger::AttachPlayback(void(*afptr)(void *)) {
  pbptr = afptr;
}

// OPen a file on the SD Card and play back for uploading or verification
bool  DataLogger::PlaybackFile(uint8_t *filename){
FsFile pbFile;
uint16_t bytesread;
	if (! pbFile.open((const  char *)filename, O_RDONLY )) {
    if (dbprint)iosptr->println("Could not open file for playback ");
		return false;
	}
	if(mystatus.islogging){
		if (dbprint)iosptr->println("Can't play back while logging");  
		return false;
	}
	if(pbptr == NULL) return false;
// file is open, and we're not logging,   read in structsize variables to bufferptr
	do {
		bytesread = pbFile.read( buffptr, structsize);
		if(bytesread > 0){
			pbptr(buffptr); // send to user playback function				
		}
	}while(bytesread == structsize);
	pbFile.close();
	pbptr(NULL);  // send NULL pointer to indicate end of file
return true;	
}

// Set the IO Stream used for console output.  iosptr defaults to &Serial, the USB serial port
//  For loggers without a USB connection, it could be set to a hardware serial port.
void DataLogger::SetStream(Stream * sptr) {
  iosptr = sptr;
}


//  Open the file, using EXFat if SD Card is formatted that way
bool DataLogger::StartLogger(const char *filename, uint16_t syncInterval) {

  ClearStatus();
  mystatus.collectionrate = collectionrate;
  fsyncinterval = syncInterval;
  // Open the file, pointed to by fileptr.  can be either exfile or standard file
  if (!OpenDataFile(filename)) {
    if (dbprint)iosptr->print("Could not open data file.\n");
    return false;
  }
  filemillis = 2;

  eldisplay = 20;  // to better sync display time with main program file time
  CheckLogger();  // perform an initial CheckLogger to init some variables
  bufferoverflows = 0;
  maxcdelay = 0;
  filestarttime = now();
  mystatus.collectionstart = now();
  strncpy((char *)mystatus.filename, filename, MAXNAMELENGTH-1); 
  mystatus.maxchunksready = 0;
  mystatus.islogging = true;
  wrtimesum = 0.0;
  wravgcount = 0;
  writechunk = 0;
  readychunks = 0;
  collectchunk = 0;
  collectidx = 0;
  writeidx = 0;
  collectPtr =  chunkPtrs[collectchunk];
  writePtr = chunkPtrs[writechunk];
  // now that everything is set up, we can start the collection timer
  dlTimer.begin(&LoggerISR, 1000000 / collectionrate); // start at collectionrate
  mystatus.filestartmilli = millis();

  return true;
}


// Close the file and stop the timer
bool DataLogger::StopLogger(void) {
  dlTimer.end();
  mystatus.collectionend = now();
  delay(10);

  dataFile.truncate(); //Shrink to amount actually written
  dataFile.close();

  mystatus.islogging = false;
  return true;
}

// return a pointer to a loggerstatus record.  The parts of loggerstatus that are updated in the
// collection ISR are saved in volatile variable and protected by noInterrupts/interrupts
// when status is updated in the CheckLogger function.

TLoggerStat * DataLogger::GetStatus(void) {
  if (mystatus.islogging) {
	 mystatus.collectionend = now();
	 mystatus.filecurrentmilli = filemillis;
  }
  return &mystatus;
}

// reset only the average and maximum write times and max collection time
void DataLogger::ResetStatus(void) {
  cli();
  mystatus.maxcdelay =  0;
  mystatus.avgwritetime = 0.0;
  mystatus.maxwritetime = 0.0;
  wrtimesum = 0.0;
  wravgcount = 0;
  sei();

}

void DataLogger::SetDBPrint(bool dbpval) {
  dbprint = dbpval;
}

// Send a vervose display of status to the iosptr stream
void DataLogger::ShowStatus(void) {
  // copy to externstatus  and display results

  iosptr->println("Data Logger Status:");
 // Still a work in progress
}


// clear max times and averages, bytes written and overflows
void DataLogger::ClearStatus(void) {
  mystatus.byteswritten = 0;
  mystatus.maxcdelay = 0.0;
  mystatus.avgwritetime = 0.0;
  mystatus.maxwritetime = 0.0;
  mystatus.bufferoverflows = 0;
  mystatus.maxchunksready = 0;

  wrtimesum = 0.0;
  wravgcount = 0;
}

bool  DataLogger::FormatSD(bool useEXFat) {

//	SdCardFactory constructs and initializes the 
	SdCardFactory cardFactory;
	SdCard* m_card = nullptr;
	ExFatFormatter exFatFormatter;
	FatFormatter fatFormatter;
	uint32_t cardSectorCount = 0;
	uint8_t  sectorBuffer[512];

  // Discard any extra characters.
  do {
    delay(10);
  } while (Serial.available() && Serial.read() >= 0);
  
  	if(mystatus.islogging) {
		iosptr->print("Cannot format while logging!\n");
		return false;
	}

  iosptr->print( "Warning, all data on the card will be erased.\n");
 iosptr->print( "Enter 'Y' to continue: ");
  while (!iosptr->available()) {
    SysCall::yield();
  }
  char c = iosptr->read();

  if (c != 'Y') {
    iosptr->print("\nQuitting, you did not enter 'Y'.\n");
    return false;
  }
  // Read any existing Serial data.
  do {
    delay(10);
  } while (iosptr->available() && iosptr->read() >= 0);

  // Select and initialize proper card driver.
  m_card = cardFactory.newCard(SD_CONFIG);
  if (!m_card || m_card->errorCode()) {
    iosptr->print("card init failed.\n");
    return false;
  }

  cardSectorCount = m_card->sectorCount();
  if (!cardSectorCount) {
    iosptr->print("Get sector count failedn");
    return false;
  }

  
   iosptr->printf("\nCard size: %8.3f ", cardSectorCount/2097152.0);
   iosptr->printf(" GiB (GiB = 2^30 bytes)\n");

   iosptr->print("Card will be formated ");
  if ((cardSectorCount >  67108864) || useEXFat) {
     iosptr->print("exFAT\n");
	 exFatFormatter.format(m_card, sectorBuffer, &Serial) ;
  } else{
     iosptr->print("FAT32\n");
	 fatFormatter.format(m_card, sectorBuffer, &Serial);	
  }

   iosptr->print("\n");
   InitStorage(); 

	return true;
}



// try to activate MTP Mode to allow
bool DataLogger::DoMTP(void) {
  // char ch;
  //Serial.print("Enter any character to exit MTP mode\n");
  // do{
  //   mtpd.loop();
  // }while(!Serial.available());
  // ch = Serial.read();
  Serial.println("MTP transfer not yet implemented.");
  return false;
}
