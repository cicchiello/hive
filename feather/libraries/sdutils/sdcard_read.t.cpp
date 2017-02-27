#include <sdcard_read.t.h>

#include <sdcard_write.t.h>

#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <sdutils.h>

#include <SdFat.h>



#define FILENAME "/TEST.TXT"

bool SDCardRead::setup()
{
    TF("SDCardRead::setup");
    PH("Initializing SD card...");

    return true;
}


/* STATIC */
bool SDCardRead::testFile(const char *filename)
{
    TF("SDCardRead::testFile");
    
    SdFat sd;
    
    const char *line1 = "This is a test!";
    const char *line2 = "Line2 of the test!";
    const char *line3 = "This is a long-ish line to test the readline function's ability to deal with a buffer that is not initially large enough";

    SdFile f;
    if (!f.open(filename, O_READ)) {
        PH("Couldn't open file");
	return false;
    }

    int bufsz = 40;
    char *buf = new char[bufsz];
    SDUtils::ReadlineStatus stat;
    int linecnt = 0;
    while ((stat = SDUtils::readline(&f, buf, bufsz)) != SDUtils::ReadEOF) {
        while (stat == SDUtils::ReadBufOverflow) {
	    char *newBuf = new char[bufsz*2];
	    memcpy(newBuf, buf, bufsz);
	    delete [] buf;
	    buf = newBuf;
	    newBuf = buf + bufsz;
	    stat = SDUtils::readline(&f, newBuf, bufsz);
	    bufsz *= 2;
	}
	PH(buf);
		
	if (linecnt == 0) {
	    if (strcmp(buf, line1) != 0) {
	        f.close();
		PH("SDCardRead: failed at test 1");
		return false;
	    }
	} else if (linecnt == 1) {
	    if (strcmp(buf, line2) != 0) {
	        f.close();
		PH("SDCardRead: failed at test 2");
		return false;
	    }
	} else if (linecnt == 2) {
	    if (strcmp(buf, line3) != 0) {
	        f.close();
		PH("SDCardRead: failed at test 3");
		return false;
	    }
	} else {
	    f.close();
	    PH("SDCardRead: failed at test 4");
	    return false;
	}
	linecnt++;
    }
    f.close();
    delete [] buf;
    return true;
}

static unsigned long timeToAct = 1000l;
static bool success = true;

bool SDCardRead::loop() {
    TF("SDCardRead::loop");
    
    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
        PH(testName());

	m_didIt = true;

	SdFat sd;
	SDUtils::initSd(sd);

	// file must exist to begin with
	bool exists = sd.exists(FILENAME);
	if (!exists) {
	    PH("TEST.TXT doesn't exist; I'll try to create it.");
	    SDCardWrite::makeFile(FILENAME);
	}

	exists = sd.exists(FILENAME);
	if (exists) {
	    success = testFile(FILENAME);
	} else {
	    PH("failed at test 5");
	    success = false;
	}
    }
    
    return success;
}


