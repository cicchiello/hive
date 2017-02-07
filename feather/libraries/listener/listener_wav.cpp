#include <listener_wav.h>

#define NDEBUG
#include <strutils.h>

#include <listener.h>
#include <sdutils.h>
#include <platformutils.h>

#include <SdFat.h>


// this is done in a separate compilation unit so that I can use the standard Arduino SD
// libraries without worrying about conflicts with the SdFat library that was required
// in order to persist the samples fast enough

#ifndef DISABLE_ASSERTS
#define ENABLE_ASSERTS
#endif

#define regerr(msg) {s_singleton->m_success = false; s_singleton->m_errMsg = msg;}
#define abort() PlatformUtils::nonConstSingleton().resetToBootloader()


#define ENABLE_ASSERTS
#ifdef ENABLE_ASSERTS
#define failtest(t,msg) assert(t,msg)
#else
#define failtest(t,msg) if(!(t)) {regerr(msg);}
#endif

// soudnfile.sapp.org/doc/WaveFormat



class ListenerWavCreatorImp : public ListenerWavCreator {
public:
    ListenerWavCreatorImp(const char *rawFilename, const char *wavFilename,
			  int samplesToWrite, 
			  uint16_t dcBias, uint8_t srcResolution, uint8_t targetResolution);
    ~ListenerWavCreatorImp();
  
    bool writeChunk();
    const char *getErrmsg() const {return m_errMsg;};

    const char *m_rawFilename, *m_wavFilename, *m_errMsg;
    SdFat m_sd;
    SdFile m_rf, m_wf;
    const int m_samplesToWrite;
    int m_samplesRead, m_samplesWritten;
    uint8_t m_srcResolution, m_targetResolution;
    uint16_t m_dcBias;
    bool m_success;
};
  



uint8_t *uint16LittleEndian(uint8_t buf[2], uint16_t ui)
{
    buf[0] = ui & 0xff;
    buf[1] = (ui >> 8) & 0xff;
    return buf;
}

uint8_t *uint32LittleEndian(uint8_t buf[4], uint32_t ui)
{
    buf[0] = ui & 0xff;
    buf[1] = (ui >> 8) & 0xff;
    buf[2] = (ui >> 16) & 0xff;
    buf[3] = (ui >> 24) & 0xff;
    return buf;
}


static ListenerWavCreatorImp *s_singleton = NULL;

ListenerWavCreatorImp::ListenerWavCreatorImp(const char *rawFilename, const char *wavFilename,
					     int samplesToWrite, 
					     uint16_t dcBias, uint8_t srcResol, uint8_t tgtResol)
  : m_rawFilename(rawFilename), m_wavFilename(wavFilename), m_samplesToWrite(samplesToWrite),
    m_samplesRead(0), m_samplesWritten(0), m_success(true), m_errMsg(NULL),
    m_dcBias(dcBias), m_srcResolution(srcResol), m_targetResolution(tgtResol)
{
    PF("ListenerWavCreatorImp::ListenerWavCreatorImp; ");
    
    assert(s_singleton == NULL, "Singleton assumption violated");
    s_singleton = this;

    SDUtils::initSd(m_sd);
    
    PH("Will transfer ");
    P(m_samplesToWrite);
    PL(" samples to the WAV file");

    // rawFilename must exist to begin with
    bool stat = m_sd.exists(m_rawFilename);
    assert(stat, "RAW_FILENAME doesn't exist");

    stat = m_rf.open(m_rawFilename, O_READ);
    failtest(stat, "Couldn't open RAW_FILENAME");
    if (!m_success) return;
 
    // wavFilename must *not* exist to begin with
    if (m_sd.exists(m_wavFilename)) {
        stat = m_sd.remove(m_wavFilename);
	stat &= !m_sd.exists(m_wavFilename);
	assert(stat, "WAV FILE alread exists");
    }

    stat = m_wf.open(m_wavFilename, FILE_WRITE);
    size_t written = m_wf.write("RIFF", 4);
    assert(stat && (written == 4), "unexpected result from SdFile::write");

    int totalSz = 44 + m_rf.fileSize() - 8;
    assert(totalSz > 44-8, "Unexpected raw file size");
    uint32_t chunkSz = totalSz; // total file size in bytes minus 8 goes here
    uint8_t uint32buf[4];
    written = m_wf.write(uint32LittleEndian(uint32buf, chunkSz), 4); 
    assert(written == 4, "unexpected result from SdFile::write");

    written = m_wf.write("WAVE", 4);
    assert(written == 4, "unexpected result from SdFile::write");

    written = m_wf.write("fmt ", 4);
    assert(written == 4, "unexpected result from SdFile::write");

    uint32_t subchunk1sz = 16;
    written = m_wf.write((uint8_t*) &subchunk1sz, 4);
    assert(written == 4, "unexpected result from SdFile::write");

    uint16_t audioFormat = 1; // PCM == 1
    written = m_wf.write((uint8_t*) &audioFormat, 2);
    assert(written == 2, "unexpected result from SdFile::write");

    uint16_t numChannels = 1;
    written = m_wf.write((uint8_t*) &numChannels, 2);
    assert(written == 2, "unexpected result from SdFile::write");

    uint32_t sampleRate = Listener::SAMPLES_PER_SECOND;
    
    written = m_wf.write(uint32LittleEndian(uint32buf, sampleRate), 4);
    assert(written == 4, "unexpected result from SdFile::write");

    uint16_t bitsPerSample = 16; // lets see how left-shifting my 14 bit samples to make 16bit
                                 // samples sounds
    uint32_t byteRate = sampleRate*numChannels*bitsPerSample/8;
    written = m_wf.write(uint32LittleEndian(uint32buf, byteRate), 4);
    assert(written == 4, "unexpected result from SdFile::write");

    uint16_t blockAlign = numChannels*bitsPerSample/8;
    uint8_t uint16buf[2];
    written = m_wf.write(uint16LittleEndian(uint16buf, blockAlign), 2);
    assert(written == 2, "unexpected result from SdFile::write");

    written = m_wf.write(uint16LittleEndian(uint16buf, bitsPerSample), 2);
    assert(written == 2, "unexpected result from SdFile::write");

    written = m_wf.write("data", 4);
    assert(written == 4, "unexpected result from SdFile::write");

    uint32_t numSamples = m_rf.fileSize()/Listener::BYTES_PER_SAMPLE;  // number of samples
    uint32_t subChunk2Sz = numSamples*numChannels*bitsPerSample/8;
    written = m_wf.write(uint32LittleEndian(uint32buf, subChunk2Sz), 4);
    assert(written == 4, "unexpected result from SdFile::write");

    m_samplesRead = m_samplesWritten = 0;
    //... data samples follow!
}


ListenerWavCreator *Listener::initWavFileCreator(const char *rawFilename, const char *wavFilename,
						 int numSamples, 
						 int dcBias,
						 uint8_t srcResolution, uint8_t targetResolution)
{
    return new ListenerWavCreatorImp(rawFilename, wavFilename, numSamples, 
				     dcBias, srcResolution, targetResolution);
}


bool ListenerWavCreatorImp::writeChunk()

{
    PF("ListnerWavCreatorImp::writeChunk; ");
    
    //... data samples follow!
    if (m_success) { // as long as no error yet
        uint8_t buf[Listener::SD_BLK_SZ];
	int bytesRead = m_rf.read(buf, Listener::SD_BLK_SZ);
	if (bytesRead > 0) {
	    int samplesRead = bytesRead/Listener::BYTES_PER_SAMPLE;
	    for (int i = 0; i < bytesRead; i += 2) {
		uint16_t s = (buf[i+1] << 8) | buf[i];
		s -= m_dcBias;
		s <<= (m_targetResolution-m_srcResolution);
		buf[i] = s & 0xff;
		buf[i+1] = (s >> 8) & 0xff;
	    }
	    m_samplesRead += samplesRead;
	    int bytesWritten;
	    if (m_samplesWritten + samplesRead <= m_samplesToWrite) {
	        bytesWritten = m_wf.write(buf, bytesRead);
		if (bytesWritten != bytesRead) 
		    PHL("Couldn't write the entire buffer to the wav file");
		int samplesWritten = bytesWritten/Listener::BYTES_PER_SAMPLE;
		m_samplesWritten += samplesWritten;
		m_success &= samplesWritten == samplesRead;
		m_success &= m_samplesWritten == m_samplesRead;
		if (m_success) 
		    return false; // say that we're not done yet
	    } else {
	        bytesWritten = m_wf.write(buf,
		     Listener::BYTES_PER_SAMPLE*(m_samplesToWrite - m_samplesWritten));
		int samplesWritten = bytesWritten/Listener::BYTES_PER_SAMPLE;
		PH("Writing partial buffer: "); PL(samplesWritten);
		m_samplesWritten += samplesWritten;
		m_success &= samplesWritten == samplesRead;
		m_success &= m_samplesWritten == m_samplesToWrite;
		m_success &= m_samplesWritten == m_samplesRead;
	    }
	    if (!m_success) {
	        PHL("Just detected failure");
	    }
	} else {
	    PH(m_samplesRead);
	    PL(" samples read from raw file");
	    m_success &= m_samplesRead == m_samplesToWrite;
	}
    }
    
    return true;  // say that we're done
}


ListenerWavCreatorImp::~ListenerWavCreatorImp()
{
    PF("ListenerWavCreatorImp::~ListenerWavCreatorImp; ");
    
    PH("Wrote ");
    P(m_samplesWritten);
    PL(" samples.");

    // close both files
    m_wf.close();
    m_rf.close();

    // if successful, remove the rawfile -- else leave it there for diagnostic purposes
    if (m_success) {
        // delete the raw file so that I can capture another listener call
        m_sd.remove(m_rawFilename);
    } else {
        PH("FAILURE: expected to write ");
	P(m_samplesToWrite);
	PL(" samples");
    }

    s_singleton = NULL;
}


ListenerWavCreator::~ListenerWavCreator()
{
}
