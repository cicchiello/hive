#include <sdcard_ls.t.h>

#define NDEBUG
#include <strutils.h>
#include <sdutils.h>

#include <SdFat.h>

bool SDCardLS::setup() {
    PF("SDCardLS::setup; ");
    
    PHL("Initializing SD card...");

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
    PLC(sd.fatType(), DEC);

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
    PL();

    PHL("Files found on the card (name, date and size in bytes): ");
    
    // list all files in the card with date and size
    sd.ls("/", LS_A | LS_DATE | LS_SIZE | LS_R);

    m_didIt = true;
    
    return true;
}


bool SDCardLS::loop() {
    return true;
}

