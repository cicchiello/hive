#include <Arduino.h>
#include <tests.h>

static Tests tests;

// the setup function runs once when you press reset or power the board
void setup() {
    tests.setup();

    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    Serial.println("Connected...");
    pinMode(10, OUTPUT);  // indicates BLE connection
}


// the loop function runs over and over again forever
void loop() {
    tests.loop();
}

