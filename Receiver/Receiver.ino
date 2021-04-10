#include <SPI.h>
#include "RF24.h"
#include "printf.h"
#include "Transmitter.h"

// Bit packing chart 8 bits
// | extra | State | Zone  | Brightness |
// |  0 0  | ON:1  | 1:001 |   High:11  |
// |  0 0  | OFF:0 | 2:010 |   Med:10   |
// |  0 0  |  x    | 3:100 |    Lo:01   |
// |  0 0  |  x    | A:111 |    NL:00   |



// Zone pins must be set to PWM pins.
#define ZONE1_PIN 3
#define ZONE2_PIN 5
#define ZONE3_PIN 6

// Start state is off with all zones selected on high
uint8_t lightState = 0b00011111;
// Brightness values for each mode 0-255 Night, Low, Medium and High
const uint8_t brightnessLevel[] = {5, 50, 150, 255};


//SPI bus(11,12,13) plus pins 9 & 10 (CE & CS)
RF24 radio(9,10);

void setup(void){
	Serial.begin(9600);
	printf_begin();
	Serial.println("Reciver started");
	// Setup and configure rf radio
	radio.begin();							// Connect to radio
	radio.setPALevel(RF24_PA_LOW );			// Set transmition strength to low.
	radio.setChannel(8);					// Clear channel for my location
	radio.setRetries(0,15);                 // Smallest time between retries, max no. of retries
	radio.setPayloadSize(1);                // 1 Byte payload is expected
	radio.openReadingPipe(1,recieverAddress[RECEIVER_NUMBER]); // Open the reading pipe to the address assigned to Reciver
	radio.startListening();                 // Start listening

	// Set the LED pins to output and off
	pinMode(ZONE1_PIN,OUTPUT);
	pinMode(ZONE2_PIN,OUTPUT);
	pinMode(ZONE3_PIN,OUTPUT);
	analogWrite(ZONE1_PIN, 0);
	analogWrite(ZONE2_PIN, 0);
	analogWrite(ZONE3_PIN, 0);
	Serial.println("Setup done");
}

void loop(void){
	// Constantly checking to see if there is a new message
	if(radio.available()){
		Serial.println("Message received");
		uint8_t len = radio.getPayloadSize();
		radio.read(&lightState,len);
		// If the command is to turn on zones set the value for the selected zones
		if(lightState & 0b00100000){ // on
			if(lightState & 0b00010000){
				Serial.println(lightState);
				analogWrite(ZONE3_PIN, brightnessLevel[lightState & 0b00000011]);
			}
			if(lightState & 0b00001000){
				Serial.println(lightState);
				analogWrite(ZONE2_PIN, brightnessLevel[lightState & 0b00000011]);
			}
			if(lightState & 0b00000100){
				Serial.println(lightState);
				analogWrite(ZONE1_PIN, brightnessLevel[lightState & 0b00000011]);
			}
		}
		// If the command is to turn off zones set the value for the pin to 0
		else{ // off
			if(lightState & 0b00010000){
				Serial.println(lightState);
				analogWrite(ZONE3_PIN, 0);
			}
			if(lightState & 0b00001000){
				Serial.println(lightState);
				analogWrite(ZONE2_PIN, 0);
			}
			if(lightState & 0b00000100){
				Serial.println(lightState);
				analogWrite(ZONE1_PIN, 0);
			}
		}
	}
	// Small delay to slow down the loop
	delay(20);
}
