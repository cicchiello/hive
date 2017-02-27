#include <sdutils.h>

#include <Arduino.h>

#define NDEBUG
#include <Trace.h>

#include <SdFat.h>

#include <wiring_private.h> // pinPeripheral function

#include <strutils.h>

/* STATIC */
const int SDUtils::SPI_CS = 10;


/* STATIC */
SDUtils::ReadlineStatus SDUtils::readline(SdFile *f, char *buf, int bufsz)
{
    int b = 0, i = 0;
    while ((f->curPosition() < f->fileSize()) && (b = f->read())) {
        if (b < 0) {
	    return ReadEOF;
	} else {
	    char c = (char) (b & 0xff);
	    if (c == '\n') {
                buf[i] = 0;
	        return ReadSuccess;
	    }
	    buf[i++] = c;
	    if (i >= bufsz) {
	        return ReadBufOverflow;
	    }
	}
    }
    return ReadEOF;
}


bool SDUtils::initSd(SdFat &sd)
{
    TF("SDUtils::initSd");
    if (!sd.begin(SPI_CS, SPI_HALF_SPEED)) {
        PL("SDUtils::initSd; initialization failed. Things to check:");
	PL("* is a card inserted?");
	PL("* is your wiring correct?");
	PL("* did you change the chipSelect pin to match your shield or module?");
	return false;
    } else {
        TRACE("SDUtils::initSd; Wiring is correct and a card is present.");
	return true;
    }
}
