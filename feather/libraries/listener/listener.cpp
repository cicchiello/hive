#include <listener.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <strutils.h>
#include <sdutils.h>

#include <adcutils.h>
#include <listener_wav.h>
#include <platformutils.h>

static void fail(const char *errMsg);

#include <SdFat.h>



// size of the contiguous raw file
#define FILE_SZ(ms) (((ms)/1000)*Listener::SAMPLES_PER_SECOND*BYTES_PER_SAMPLE)
#define SD_BLOCK_COUNT(ms) ((int)((FILE_SZ(ms)/SD_BLK_SZ) + 1))
#define SD_BLOCKS_PER_CHUNK (ADCUtils::singleton().numSamplesPerBuf()*BYTES_PER_SAMPLE/SD_BLK_SZ)


const char *RAW_FILENAME = "LISTEN.RAW";

static Listener *s_currListener = NULL;


Listener::Listener(int ADCPIN, int BIASPIN) 
  : m_sd(NULL), m_file(NULL), m_ADCPIN(ADCPIN), m_BIASPIN(BIASPIN), m_errMsg(NULL),
    m_initializedADC(false), m_wavCreator(NULL), 
    m_fifo(4), m_min(0x7fff), m_max(-32766), m_avg(0), m_sampleCnt(0), m_skipCnt(0)
{
    TF("Listener::Listener");
    assert(s_currListener == NULL, "singleton rule violated for Listener");
    s_currListener = this;


    // for SAMD21, A0 is the only pin that can be configured as DAC output
    assert(m_BIASPIN == A0, "DAC is only supported on A0");


    // the following should probably be done only once, when listening is about to start, then disabled
    // when it stops.  But for now, just do it by virtue of the object being in existence
    pinMode(A0, OUTPUT);

    DAC->CTRLA.bit.ENABLE = 0x01;     // Enable DAC
    while (DAC->STATUS.bit.SYNCBUSY == 1)
        ;

    DAC->DATA.reg = 155; // value chosen to produce 0.5V on a 0->3.3V scale, based on 10bits resolution
}

Listener::~Listener()
{
    TF("Listener::~Listener");
    s_currListener = NULL;

    delete m_wavCreator;

    pinMode(A0, INPUT);
    
    DAC->CTRLA.bit.ENABLE = 0x001;     // Disable DAC
    while (DAC->STATUS.bit.SYNCBUSY == 1)
        ;
}


void Listener::stopRecord() 
{
    TF("Listener::stopRecord");
    assert(m_fifo.isEmpty(), "Buffer isn't empty yet");
    if (m_initializedADC)
        ADCUtils::nonConstSingleton().shutdown();
    if (m_sd != NULL) {
        bool stat = m_sd->card()->writeStop();
	assert(stat, "writeStop failed");

	if (m_file != NULL) {
	    m_file->close();
	    delete m_file;
	}

	delete m_sd;

	m_file = NULL;
	m_sd = NULL;
    }
}


void Listener::fail(const char *errMsg)
{
    TF("Listener::fail");
    m_errMsg = errMsg;
    while (!m_fifo.isEmpty()) {
	ADCUtils::nonConstSingleton().freeBuf(m_fifo.pop());
    }
    stopRecord();
}


/* STATIC */
void fail(const char *errMsg)
{
    TF("::fail");
    if (s_currListener == NULL) {
        PHL(errMsg);
    } else {
        s_currListener->fail(errMsg);
    }
    PlatformUtils::nonConstSingleton().resetToBootloader();
}


void persistBuffer(uint16_t *buf)
{
    TF("::persistBuffer");
    assert(!s_currListener->m_fifo.isFull(), "Fifo is full!?!?");
    assert(buf, "buf is NULL");
    if (s_currListener->m_state != Listener::Capturing) {
        if (s_currListener->m_state == Listener::CaptureStopped) {
	    PL("captured a chunk after ADCUtils::nonConstSingleton().stop() called");
	} else {
	    P("captured a chunk while in an unexpected state: ");
	    PL(s_currListener->m_state);
	}
    } else {
        int d;
	assert((d = s_currListener->m_fifo.depth()) >= 0, "always true -- just capturing d");
        s_currListener->m_fifo.push(buf);
	assert(d + 1 == s_currListener->m_fifo.depth(), "re-entrant problem trapped");
    }
}


bool Listener::record(unsigned int duration_ms, const char *wavFilename, bool verbose) 
{
    TF("Listener::record");
    if (verbose) {
        TRACE2("Listener::record ", millis());
    }
    
    m_wavFilename = wavFilename;
    m_duration_ms = duration_ms;
    m_min = 0x7fff;
    m_max = -32766;
    m_avg = 0;
    m_sampleCnt = 0;
  
    // create file system object
    m_sd = new SdFat();
    m_file = new SdFile();
  
    // we'll use the initialization code from the utility libraries
    bool exists = SDUtils::initSd(*m_sd);
    assert(exists, "Couldn't initialize sd");
    
    // delete possible existing file
    exists = m_sd->exists(RAW_FILENAME);
    if (exists) {
        PL("Deleting old RAW_FILENAME");
        m_sd->remove(RAW_FILENAME);
	exists = m_sd->exists(RAW_FILENAME);
	assert(!exists, "Couldn't delete old RAW_FILENAME");
    }

    // creat a contiguous file
    bool stat = m_file->createContiguous(m_sd->vwd(), RAW_FILENAME,
					 SD_BLOCK_COUNT(m_duration_ms)*SD_BLK_SZ);
    assert(stat, "error creating contiguous file");

    // get the location of the file's blocks
    uint32_t bgnBlock, endBlock;
    stat = m_file->contiguousRange(&bgnBlock, &endBlock);
    assert(stat, "error getting contiguous range bounds");

    // tell card to setup for multiple block write with pre-erase
    stat = m_sd->card()->erase(bgnBlock, endBlock);
    assert(stat, "card.erase failed");

    if (verbose) {
        TRACE2("Recording for (ms): ", m_duration_ms);
	TRACE3("Setting up raw file to accommodate ", SD_BLOCK_COUNT(m_duration_ms), " SD blocks");
    }
    
    stat = m_sd->card()->writeStart(bgnBlock, SD_BLOCK_COUNT(m_duration_ms));
    assert(stat, "writeStart failed");
 
    // clear the cache and use it as a 512 byte buffer
    m_cache = (uint8_t*)m_sd->vol()->cacheClear();
    m_icache = 0;
    m_chunkCnt = m_skipCnt = 0;

    // setup ADC bufferring
    ADCUtils::nonConstSingleton().init(m_ADCPIN, SAMPLES_PER_CHUNK, Listener::SAMPLES_PER_SECOND,
				       persistBuffer);
    m_initializedADC = true;

    m_captureStartTime_us = micros();
    m_endTime = m_captureStartTime_us + m_duration_ms*1000;

    m_state = Capturing;

    return !hasError();
}


void Listener::writeSD(volatile const uint16_t *buf) 
{
    TF("Listener::writeSD");
    
    //*********************************NOTE**************************************
    // NO SdFile calls are allowed while cache is used for raw writes
    //***************************************************************************
  
    int isamp = 0;
    bool dataIsQueued = false;
    while ((isamp < SAMPLES_PER_CHUNK) && !hasError()) {
        while ((m_icache < SD_BLK_SZ) && (isamp < SAMPLES_PER_CHUNK)) {
	    uint16_t uis = buf[isamp++];
	    if (uis > m_max) m_max = uis;
	    if (uis < m_min) m_min = uis;
	    m_avg += uis;
	    m_sampleCnt++;

	    // little-endian file format
	    m_cache[m_icache++] = uis & 0xff;
	    m_cache[m_icache++] = (uis >> 8) & 0xff;
	    dataIsQueued = true;
	}
	if (m_icache == SD_BLK_SZ && !hasError()) {
	    bool stat = m_sd->card()->writeData(m_cache);
	    assert(stat, "writeData failed");
	    dataIsQueued = false;
	    
	    m_icache = 0;
	    m_chunkCnt++;
	} else {
	    PL("Finished block but didn't write to file!?! (1)");
	}
    }
    assert(!dataIsQueued, "Not all queued data has been flushed");
}


void Listener::flushSD() 
{
    TF("Listener::flushSD");
    
    //*********************************NOTE**************************************
    // NO SdFile calls are allowed while cache is used for raw writes
    //***************************************************************************

    int16_t dcBias= m_avg/m_sampleCnt;
    while (m_chunkCnt < SD_BLOCK_COUNT(m_duration_ms) && !hasError()) {
        while (m_icache < SD_BLK_SZ) {
	    // pad with dcBias's
	  
	    int16_t sample = dcBias;

	    // little-endian file format
	    m_cache[m_icache++] = sample & 0xff;
	    m_cache[m_icache++] = (sample >> 8) & 0xff;
	    m_sampleCnt++;
	    PL("Wrote dcBias for final block");
	}
	if (m_icache == SD_BLK_SZ && !hasError()) {
	    PL("Wrote partially empty final block");
	    bool stat = m_sd->card()->writeData(m_cache);
	    assert(stat, "writeData failed");
	    
	    m_icache = 0;
	    m_chunkCnt++;
	} else {
	    PL("Finished block but didn't write to file!?!");
	}
    }

}


void Listener::processBuffer()
{
    TF("Listener::processBuffer");
    
    // should have a complete buffer in the fifo
    int d;
    assert((d = m_fifo.depth()) >= 0, "always true; just capturing d for later assert");
    uint16_t *buf = m_fifo.pop();
    assert(d-1 == m_fifo.depth(), "re-entrant problem trapped");
    assert(buf != NULL, "Received NULL from popFifo");
    if (m_chunkCnt < SD_BLOCK_COUNT(m_duration_ms)) 
        writeSD(buf);
    else
        m_skipCnt++;
    ADCUtils::nonConstSingleton().freeBuf(buf);
}


bool Listener::loop(bool verbose) 
{
    TF("Listener::loop");
    
    unsigned long now = micros();
    switch (m_state) {
    case Uninitialized:
        assert(false, "Shouldn't get here; initialization should happen before ::loop");
	break;
    case Capturing: {
        if (now >= m_endTime) {
	    if (verbose) PHL("Capturing done");
	    // we're done recording
	    ADCUtils::nonConstSingleton().stop();
	    m_state = CaptureStopped;
	} else if (!m_fifo.isEmpty()) {
	    processBuffer();
	}
    }
      break;
    case CaptureStopped: {
        if (now <= m_endTime + 20000l) { // wait for another 20ms for any remaining buffer 
	    if (!m_fifo.isEmpty()) {
	        // have a complete buffer 
	        processBuffer();
		assert(m_fifo.isEmpty(), "Buffer isn't empty yet");
		// there should never be more than one pending afer ::stop()
	    }
	} else {
            assert(m_fifo.isEmpty(), "Buffer isn't empty yet");
	    flushSD();

	    stopRecord();
	    
	    if (!hasError()) {
		if (verbose) {
		    TRACE2("Generating ", m_wavFilename);
		    TRACE2(" from raw file ", RAW_FILENAME);

		    TRACE2("m_chunkCnt: ", m_chunkCnt);
		    TRACE2("Max: ", m_max);
		    TRACE2("Min: ", m_min); 
		    TRACE2("Avg: ", m_avg/m_sampleCnt);
		    TRACE2("# Samples: ", m_sampleCnt);
		}

		assert(m_chunkCnt == SD_BLOCK_COUNT(m_duration_ms), "Wrong # of buffers were saved");
		if (!hasError()) {
		    assert(SAMPLES_PER_CHUNK == 256, "SAMPLES_PER_CHUNK is unexpected value");
                    m_wavCreator = initWavFileCreator(RAW_FILENAME, m_wavFilename,
						      m_chunkCnt*SAMPLES_PER_CHUNK, 
						      m_avg/m_sampleCnt,
						      Listener::SAMPLE_RESOLUTION,
						      ListenerWavCreator::WAV_RESOLUTION);
		    m_state = GenWAV;
		} else m_state = Done;
	    } else m_state = Done;
	}
    }
      break;
    case GenWAV: {
        bool doneWriting = m_wavCreator->writeChunk();
	if (doneWriting) {
	    if (!hasError() && m_wavCreator->hasError()) {
	        m_errMsg = m_wavCreator->getErrmsg();
	    } else {
		m_state = Done;

		if (verbose) 
		    PHL("Done");
	    }
	    delete m_wavCreator;
	    m_wavCreator = NULL;
	}
    }
      break;
    case Done:
      break;
    default:
      assert(false, "Shouldn't get here; unhandled m_state");
      break;
    }

    return !hasError();
}
