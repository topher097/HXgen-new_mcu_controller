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
const char *defname = "LOG";
const char *defext = "dat";
/*************************************************************************
  Public methods
************************************************************************/
// Initialize the SD Card.  Uses Fat32 of ExFat, depending on type of card.
// Requires SDFat 2.0.
// With change in SdFatConfig.h, SDFat should also handle ExFat file system
SdFs* DataLogger::InitStorage(SdFs *fptr) {
	
  if(fptr == NULL){  // initialize a new file system
	if (!sdx.begin(SdioConfig(FIFO_SDIO))) {
		if (dbprint)iosptr->printf("\nSD File  system initialization failed.\n");
		return NULL;
	}
	fattype = sdx.fatType();
	if (fattype == FAT_TYPE_EXFAT) {
		if (dbprint)iosptr->printf("Card Type is exFAT\n");
	} else {
		if (dbprint)iosptr->printf("  Card Type is FAT%u\n", fattype);
	}
	if (dbprint)iosptr->printf("File system initialization done.\n");
	sdptr = &sdx;
	} else {
	  sdptr = fptr;
	}
  mystatus.spaceavailable = (uint64_t) sdptr->vol()->sectorsPerCluster() * sdptr->vol()->freeClusterCount() * 512;

  delay(50);
  SdFile::dateTimeCallback(dateTime);
  mystatus.version = LOGGER_VERSION;
  if (timeStatus() != timeSet) {
    if (dbprint)iosptr->println("Unable to sync with the RTC");
  } else {
    if (dbprint)iosptr->println("RTC has set the system time");
    unixseconds = now();
  } 
   setSyncProvider(mygetTeensy3Time);
  return sdptr;
  

}

void DataLogger::MakeFileName(const char *fbase, const char *ext){
	time_t nn;
	nn = now();
	uint8_t mo = month(nn);
	uint8_t dd = day(nn);
	uint8_t hh = hour(nn);
	uint8_t mn = minute(nn);
	sprintf(mystatus.filename, "%s_%02d%02d%02d%02d.%s",fbase,mo,dd,hh,mn, ext);	
}


void DataLogger::ShowDirectory(void) {
uint64_t freespace;
float ffreespace;
  iosptr->print("\nData Logger Directory\n");
  iosptr->print("Date       Time     Size(B)  Name\n");
  sdptr->vol()->ls(iosptr, LS_SIZE | LS_DATE |  LS_R);
  
 freespace = (uint64_t) sdptr->vol()->sectorsPerCluster() * sdptr->vol()->freeClusterCount() * 512;
 ffreespace = (float)freespace/(1024.0*1024.0);
 iosptr->printf("Card free space = %6.0f MBytes\n",ffreespace);
}

bool DataLogger::OpenDataFile(const char *filename) {

  if (! dataFile.open(filename, O_WRITE | O_CREAT | O_TRUNC)) {
    if (dbprint)iosptr->printf("Open File failed for %s\n", filename);
    return false;
  }
  // pre-allocate 1GB
  /*
  uint64_t alloclength = (uint64_t)(1024l * 1024l * 1024l);
  if (dbprint)iosptr->print("Pre-allocating 1GB file space \n");
  if (!dataFile.preAllocate(alloclength)) {
    if (dbprint)iosptr->println("Allocation failed. Proceeding anyway.");
  } else {
    if (dbprint)iosptr->println("Allocation succeeded.");
  }
  */
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
  
  // need to make sure this works as intended
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
  if(wptr == NULL)  lastptr = collectPtr;   // lastptr used in display functions
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
  //  lastptr = writePtr;
    writechunk++;
    if (writechunk >= MAXCHUNKS) writechunk = 0;
    mystatus.byteswritten += (uint64_t)chunksize;
   
    //Serial.printf("bytes written: %" PRIu64 "\n",mystatus.byteswritten);
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
      mystatus.byteswritten += (uint64_t)nbytes;

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

void DataLogger::CheckNewFile(void){
	uint32_t stmilli;
		if(mystatus.fileinterval == 0) return;
		if(!mystatus.islogging) return;
		stmilli = millis();
		if(now() >= mystatus.nextfiletime){// start a new file
		mystatus.nextfiletime += mystatus.fileinterval;
		if (dbprint)iosptr->printf("Opening new file at:   %lu   Next time %lu\n ",
	                           now(), mystatus.nextfiletime);
		if(fileptr != NULL){
			fileptr->truncate();
			fileptr->close();
		}
		MakeFileName(mystatus.fnamebase,mystatus.fext);
		if(!OpenDataFile(mystatus.filename)){
			fileptr = NULL;
			iosptr->println("ERROR: Unable to open next file");
		}
		stmilli = millis()-stmilli;
		if (dbprint)iosptr->printf("Closing old and opening new file took %lu milliseconds\n",stmilli);
	}
}

//  CheckLogger is called from loop() to save buffered data to file and do display
//  Also checks for automatic new file
void DataLogger::CheckLogger(void) {
  uint16_t numready;
  volatile  uint8_t *dispptr;
  uint32_t logger_safe_read;
  uint16_t readyHold;

	//if (dbprint)iosptr->println("Checking for sync");  
	if (elsync > fsyncinterval) { // sync file on a regular basis	
		//if (dbprint)iosptr->printf("File Sync for file at %p\n ",dataFile);  
		dataFile.sync();
		elsync -= fsyncinterval;
	}
	numready = 0;

 // Note1:   This alternative from @defragster avoids masking interrupts
 // The LDREXW STREXW pair change things in a way I'm still reseaching
 //  if an interrupt occurs between the pair.
 //  Note2:  You have to include "arm_math.h" to get this to compile
	//if (dbprint)iosptr->println("Updating volatile variables. "); 
	do { // account for interrupts that may alter vars during read.
		__LDREXW(&logger_safe_read);
		readyHold = numready;
		numready += readychunks;
		if(wptr == NULL) dispptr = lastptr;  // lastptr changeable in timer interrupt handler
		readychunks -= (numready - readyHold);  // account for number found
		mystatus.bufferoverflows = bufferoverflows;
		mystatus.maxcdelay = maxcdelay;

	} while ( __STREXW(1, &logger_safe_read));


	if(numready > mystatus.maxchunksready) mystatus.maxchunksready = 	numready;
  
  // check amount to write against free space to prevent writing past end of SDC
	if(((uint32_t)numready* chunksize) > (mystatus.spaceavailable-FSMARGIN)){
		if (dbprint)iosptr->println("Reached end of SD Card ");  
		mystatus.islogging = false;
		return;
	} else {  
  // adjust free space count
	mystatus.spaceavailable -= (uint32_t)numready* chunksize ;
  // pull data from queue of  chunks and write to file
		//if (dbprint)iosptr->println("Writing chunks ");  
		if (wptr != NULL) {
			if (numready > 0)  WriteUserChunks(numready);
		}  else {
			if (numready > 0) WriteBinaryChunks(numready);
		}
	}
		//if (dbprint)iosptr->println("Checking for display");  
	if ((eldisplay > displayinterval) && (lastptr != NULL)){ // display last written record
	//	if(wptr != NULL)
		dispptr = lastptr;
		eldisplay -= displayinterval;	
		if(dptr != NULL)dptr((void*)dispptr); // call user-written display function can take some msec!
	}
  
	CheckNewFile();
}

// OPen a file on the SD Card and play back for uploading or verification
bool  DataLogger::PlaybackFile(char *filename){
FsFile pbFile;
uint16_t bytesread;
	if (! pbFile.open((const  char *)filename, O_RDONLY )) {
    if (dbprint) {
      iosptr->println("Could not open file for playback ");
    }
    pbptr(NULL);  // send NULL pointer to indicate end of file
		return false;
	}
	if(mystatus.islogging){
		if (dbprint)iosptr->println("Can't play back while logging");  
    pbptr(NULL);  // send NULL pointer to indicate end of file
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

void DataLogger::AutoFile(const char *namebase, const char *ext, uint16_t interval){

	time_t ntime;
	uint16_t slen;
	if(strlen(namebase) != 0){
		strncpy(mystatus.fnamebase, namebase,MAXNAMELENGTH-20);
	} else {
		strncpy(mystatus.fnamebase,defname, 3);
	}
	if(strlen(ext) != 0){
		slen = strlen(ext);
		if(slen > 10) slen = 10;
		strncpy(mystatus.fext,ext, slen);
	}  else  strncpy(mystatus.fext, defext, 3);
	// quick mod for checking code with shorter intervals
	if(interval >= 120 ){ // use seconds only for inputs > 120
		mystatus.fileinterval = (uint32_t)interval; 
	} else     mystatus.fileinterval = (uint32_t)interval * 3600;  //convert from hours to seconds
	ntime = now();
	ntime = ntime - (ntime % mystatus.fileinterval) + mystatus.fileinterval;
	mystatus.nextfiletime = ntime;

}

void DataLogger::xstartlogger(void){}  // just a marker because Notepad++ function list won't detect following functions with function parameters;

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


//  Open the file, using EXFat if SD Card is formatted that way
bool DataLogger::StartLogger(const char *filename, uint16_t syncInterval, void(*afptr)()){
  const char *defext = "DAT";
  time_t ntime;
  ClearStatus();
  mystatus.collectionrate = collectionrate;
  elsync = 0;
  fsyncinterval = syncInterval;
  if(strlen(filename) ==0){
		if(strlen(mystatus.fnamebase) ==0){ //no defined base and ext
			MakeFileName(defname, defext); // set default base and ext
		}  else {
			MakeFileName(mystatus.fnamebase,mystatus.fext);
		}
		filename = mystatus.filename;
  }
  if(mystatus.fileinterval != 0){
	ntime = now();
	ntime = ntime - (ntime % mystatus.fileinterval) + mystatus.fileinterval;
	mystatus.nextfiletime = ntime;
  }
  // Open the file, pointed to by fileptr.  can be either exfile or standard file
  if (!OpenDataFile(filename)) {
    if (dbprint)iosptr->print("Could not open data file.\n");
    return false;
  }
  filemillis = 2;

  eldisplay = 20;  // to better sync display time with main program file time
    if (dbprint)iosptr->print("Calling initial CheckLogger.\n"); 
	delay(3);
  CheckLogger();  // perform an initial CheckLogger to init some variables
   if (dbprint)iosptr->print("Called initial CheckLogger.\n");
    
  bufferoverflows = 0;
  maxcdelay = 0;
  filestarttime = now();
  mystatus.collectionstart = now();
  mystatus.collectionend = now();

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
  // we start it with a pointer to the timer chore that was sent by the main program
  dlTimer.begin(afptr, 1000000 / collectionrate); // start at collectionrate
  if (dbprint)iosptr->print("Started collection timer.\n");
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
uint64_t freespace;
	freespace = (uint64_t) sdptr->vol()->sectorsPerCluster() * sdptr->vol()->freeClusterCount() * 512;
	mystatus.spaceavailable = freespace;

	if(mystatus.collectionend != 0) mystatus.collectionend = now();
	mystatus.filecurrentmilli = filemillis;

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

void DataLogger::LoadConfig(void){
	
}

void DataLogger::SaveConfig(void){
	
	
}


void DataLogger::ShowConfig(void){
	

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
    yield();
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
	 fatFormatter.format(m_card, sectorBuffer, iosptr);	
  }

   iosptr->print("\n");
   
   // Removed InitStorage here for compatibility with multiple logger objects.
   //  It is now the responsibility of the main program to re-initialize storage after
   //  a disk format operation
   // initStorage(NULL);   // Set up a new file system

	return true;
}


