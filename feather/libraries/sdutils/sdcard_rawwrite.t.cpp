#include <sdcard_rawwrite.t.h>

#define NDEBUG
#include <Trace.h>

#include <strutils.h>
#include <sdutils.h>
#include <SdFat.h>


// number of blocks in the contiguouse file
const uint32_t BLOCK_COUNT = 1000UL; // times 512 = size of file to allocate

// time to produce a block of data
const uint32_t MICROS_PER_BLOCK = 15000; // minimum time that won't trigger an overrun error

// filename
const char *filename = "TEST.WAV";

// file system 
static SdFat *sd = NULL;

// test file
static SdFile *file = NULL;

// file extent
static uint32_t bgnBlock, endBlock;

// file raw cache
static uint8_t *pCache;

// initialize stats
static uint16_t overruns = 0;
static uint32_t maxWriteTime = 0;
static uint16_t minWriteTime = 65000;
static uint32_t t = micros();
static uint32_t b = 0;



static unsigned long timeToAct = 1000*1000l;
static bool success = true;
static int loopCnt = 0;


bool SDCardRawWrite::setup() {
    TF("SDCardRawWrite::setup; ");
    PH("Initializing SD card...");

    // create file system object
    sd = new SdFat();
    file = new SdFile();

    if (!SDUtils::initSd(*sd))
        return false;

    // delete possible existing file
    bool stat = sd->remove(filename);

    // creat a contiguous file
    stat = file->createContiguous(sd->vwd(), filename, 512UL*BLOCK_COUNT);
    assert(stat, "error creating contiguous file");

    // get the location of the file's blocks
    stat = file->contiguousRange(&bgnBlock, &endBlock);
    assert(stat, "error getting contiguous range bounds");

    // initialize stats
    overruns = 0;
    maxWriteTime = 0;
    minWriteTime = 65000;
    b = 0;
    t = micros();

    timeToAct += t;

    return success;
}


bool SDCardRawWrite::loop() {
    TF("SDCardRawWrite::loop; ");
    unsigned long now = micros();
    if (now > timeToAct && !m_didIt && success) {
        //*********************************NOTE**************************************
        // NO SdFile calls are allowed while cache is used for raw writes
        //***************************************************************************

        if (loopCnt == 0) {
	    PH("Start raw write of ");
	    P(file->fileSize());
	    PL(" bytes");

	    // tell card to setup for multiple block write with pre-erase
	    bool stat = sd->card()->erase(bgnBlock, endBlock);
	    assert(stat, "card.erase failed");

	    stat = sd->card()->writeStart(bgnBlock, BLOCK_COUNT);
	    assert(stat, "writeStart failed");
	} else {
	    // clear the cache and use it as a 512 byte buffer
	    pCache = (uint8_t*)sd->vol()->cacheClear();

	    char c = 'A';
	    for (int i = 0; i < 512; i++) {
	        pCache[i] = c++;
		if (c > 'Z') c = 'A';
	    }
	    
	    uint32_t tw = micros(); // time the actual write
	    bool stat = sd->card()->writeData(pCache);
	    assert(stat, "writeData failed");
	    tw = micros() - tw;

	    if (tw > maxWriteTime) { // check for max write time
	        maxWriteTime = tw;
	    }
	    if (tw < minWriteTime) { // check for min write time
	        minWriteTime = tw;
	    }

	    // check for overrun
	    if (tw > MICROS_PER_BLOCK) {
	        PH("Overrun; block: "); 
		P(b); 
		P("; micros: "); 
		PL(tw);
		overruns++;
	    }

	    // increment block counter
	    b++;
	    if (b == BLOCK_COUNT) { // done?
	        m_didIt = true;

		t = micros() - t; // total time
		
		// end multiple block write mode
		stat = sd->card()->writeStop();
		assert(stat, "writeStop failed");

		file->close();
		
		delete sd;
		delete file;

		PH("Done");
		PH("\tElapsed time: "); PL(t);
		PH("\tMax write time: "); PL(maxWriteTime);
		PH("\tMin write time: "); PL(minWriteTime);
		PH("\tOverruns: "); P(overruns); P(" of "); PL(b);
		PL();
	    }
	}

	loopCnt++;
    }
    
    return success;
}


