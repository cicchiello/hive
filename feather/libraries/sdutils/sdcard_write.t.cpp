#include <sdcard_write.t.h>

#include <Arduino.h>

#include <Trace.h>

#include <sdutils.h>
#include <strutils.h>

#include <SdFat.h>

 

#define FILENAME "/TEST.TXT"

bool SDCardWrite::setup() {
    TF("SDCardWrite::setup; ");
    
    PH("Initializing SD card...");

    SdFat sd;
    if (!SDUtils::initSd(sd))
        return false;
	
    // print the type of card
    PH("Card type: ");

    // sd.card()->type(): return 0 - SD V1, 1 - SD V2, or 3 - SDHC.
    switch (sd.card()->type()) {
    case 0: 
        PL("SD1");
	break;
    case 1: 
        PL("SD2");
	break;
    case 3:
        PL("SDHC");
	break;
    default:
        PL("Unknown card type");
    }

    // print the type and size of the first FAT-type volume
    uint32_t volumesize;
    PH("Volume type is FAT");
    Serial.println(sd.fatType(), DEC);

    volumesize = sd.blocksPerCluster();    // clusters are collections of blocks
    volumesize *= sd.clusterCount();       // we'll have a lot of clusters
    volumesize *= 512;                     // SD card blocks are always 512 bytes
    PH("Volume size (bytes): ");
    PL(volumesize);
    PH("Volume size (Kbytes): ");
    volumesize /= 1024;
    PL(volumesize);
    PH("Volume size (Mbytes): ");
    volumesize /= 1024;
    PL(volumesize);

    PH("Files found on the card (name, date and size in bytes): ");
    
    // list all files in the card with date and size
    sd.ls("/", LS_A | LS_DATE | LS_SIZE | LS_R);

    return true;
}


/* STATIC */
bool SDCardWrite::makeFile(const char *filename)
{
    TF("SDCardWrite::makeFile; ");
    SdFile f;
    if (!f.open(filename, O_CREAT | O_WRITE)) {
        PH("Couldn't open file");
	return false;
    } else {
        P("Created file "); PL(filename);
    }

    const char *line1 = "This is a test!\n";
    const char *line2 = "Line2 of the test!\n";
    const char *line3 = "This is a long-ish line to test the readline function's ability to deal with a buffer that is not initially large enough\n";
	
    f.write(line1, strlen(line1));
    f.write(line2, strlen(line2));
    f.write(line3, strlen(line3));

    f.close();

    return true;
}

static unsigned long timeToAct = 1000l;
static bool success = true;

bool SDCardWrite::loop() {
    TF("SDCardWrite::loop; ");
    unsigned long now = millis();
    if (now > timeToAct && !m_didIt) {
	m_didIt = true;
        PH(testName());

	SdFat sd;
	SDUtils::initSd(sd);
	
	// file mustn't exist to begin with
	bool exists = sd.exists(FILENAME);
	if (exists) {
	    sd.remove(FILENAME);
	    exists = sd.exists(FILENAME);
	}

	if (exists) {
	    PH("file that I'm supposed to create already exists and I couldn't delete it");
	    success = false;
	    return false;
	}

	makeFile(FILENAME);
    }
    
    return success;
}

