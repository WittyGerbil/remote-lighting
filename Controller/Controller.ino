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

#define DEBUGGING_ENABLED 0

// Delay value to filter out button noise after press
#define DEBOUNCE_DELAY  50
// Button input pins can be connected to any input pins
#define ON_PIN  7
#define OFF_PIN 8
#define HI_PIN  A0
#define MED_PIN A1
#define LO_PIN  A2
#define NL_PIN  6
#define Z1_PIN  5
#define Z2_PIN  4
#define Z3_PIN  3
#define AZ_PIN  2

const uint8_t button_pins[] ={ON_PIN, OFF_PIN, HI_PIN, MED_PIN, LO_PIN, NL_PIN, Z1_PIN, Z2_PIN, Z3_PIN, AZ_PIN};

// Start state is off with all zones selected on high
uint8_t lightState = 0b00011111;
// Tracks if a zone change is taking place to allow proper function
uint8_t zoneChange = 0;

RF24 radio(9,10);

void setup(void)
{
	Serial.begin(9600);
	printf_begin();
	#if DEBUGGING_ENABLED
		Serial.println("Remote started");
	#endif
	// Setup and configure rf radio
	radio.begin();
	radio.setPALevel(RF24_PA_LOW);
	radio.setChannel(8);
	radio.setRetries(0,15);                 // Smallest time between retries, max no. of retries
	radio.setPayloadSize(1);                // Here we are sending 1-byte
	radio.openReadingPipe(1,controllerAddress);
	// Set pull-up resistors for all buttons
	// When pressed the button is grounded
	for(int i = 0; i++; i < 9){
		pinMode(button_pins[i],INPUT);
		digitalWrite(button_pins[i],LOW);
	}
}

void loop(void)
{
	// Poll each pin to see if the button attached is being pressed
	// On
	if(digitalRead(ON_PIN)){
		#if DEBUGGING_ENABLED
		    Serial.println(F("On"));
		#endif
		if((lightState | 0b00100000) == lightState) lightState |= 0b00111100; // If already on turn all zones on
		else lightState |= 0b00100000; // If not on turn on
		sendChanges();
		while(digitalRead(ON_PIN)){} // Wait untill the button is released by the user
		delay(DEBOUNCE_DELAY); // Delay a small amount of time to allow the voltage to stableize
	}
	// Off
	if(digitalRead(OFF_PIN)){
		#if DEBUGGING_ENABLED
		    Serial.println(F("Off"));
		#endif
		if((lightState & 0b00011111) == lightState){ // If already off turn all zones off
			lightState &= 0b00011111;
			lightState |= 0b00011100;
		}
		else lightState &= 0b00011111; // If on turn off
		sendChanges();
		while(digitalRead(OFF_PIN)){}
		delay(DEBOUNCE_DELAY);
	}
	// High
	if(digitalRead(HI_PIN)){
		#if DEBUGGING_ENABLED
		    Serial.println(F("High"));
		#endif
		if(((lightState | 0b00100011) == lightState) & !zoneChange) lightState = 0b00111111; // If already on high all zones hi
		else lightState |= 0b00100011; // If not set high
		sendChanges();
		while(digitalRead(HI_PIN)){}
		delay(DEBOUNCE_DELAY);
	}
	// Medium
	if(digitalRead(MED_PIN)){
		#if DEBUGGING_ENABLED
		    Serial.println(F("Medium"));
		#endif
		if((((lightState & 0b00111110) | 0b00100010) == lightState) & !zoneChange) lightState = 0b00111110; // If already on mMedium all zones medium
		else lightState = (lightState & 0b00111110) | 0b00100010; // If not set medium
		sendChanges();
		while(digitalRead(MED_PIN)){}
		delay(DEBOUNCE_DELAY);
	}
	if(digitalRead(LO_PIN)){ // Low
		#if DEBUGGING_ENABLED
		    Serial.println(F("Lo"));
		#endif
		if((((lightState & 0b00111101) | 0b00100001) == lightState) & !zoneChange) lightState = 0b00111101; // If already on low all zones low
		else lightState = (lightState & 0b00111101) | 0b00100001;// If not set low
		sendChanges();
		while(digitalRead(LO_PIN)){}
		delay(DEBOUNCE_DELAY);
	}
	if(digitalRead(NL_PIN)){ // Night
		#if DEBUGGING_ENABLED
		    Serial.println(F("Night"));
		#endif
		if((((lightState & 0b00111100) | 0b00100000) == lightState) & !zoneChange) lightState = 0b00111100; // If already on night all zones night
		else lightState = (lightState & 0b00111100) | 0b00100000; // If not set night
		sendChanges();
		while(digitalRead(NL_PIN)){}
		delay(DEBOUNCE_DELAY);
	}
	if(digitalRead(Z1_PIN)){ // Zone 1
		#if DEBUGGING_ENABLED
		    Serial.println(F("Z1"));
		#endif
		zoneChange = 1;
		if((lightState & 0b00011100) == 0b00011100){ // If all zones selected just select one
			lightState &= 0b00100011;
			lightState |= 0b00000100;
		}
		else lightState ^= 0b00000100; // If not toggle
		while(digitalRead(Z1_PIN)){}
		delay(DEBOUNCE_DELAY);
	}
	if(digitalRead(Z2_PIN)){ // Zone 2
		#if DEBUGGING_ENABLED
		    Serial.println(F("Z2"));
		#endif
		zoneChange = 1;
		if((lightState & 0b00011100) == 0b00011100){ // If all zones selected just select one
			lightState &= 0b00100011;
			lightState |= 0b00001000;
		}
		else lightState ^= 0b00001000; // If not toggle
		while(digitalRead(Z2_PIN)){}
		delay(DEBOUNCE_DELAY);
	}
	if(digitalRead(Z3_PIN)){ // Zone 3
		#if DEBUGGING_ENABLED
		    Serial.println(F("Z3"));
		#endif
		zoneChange = 1;
		if((lightState & 0b00011100) == 0b00011100){ // If all zones selected just select one
			lightState &= 0b00100011;
			lightState |= 0b00010000;
		}
		else lightState ^= 0b00010000; // If not toggle
		while(digitalRead(Z3_PIN)){}
		delay(DEBOUNCE_DELAY);
	}
	if(digitalRead(AZ_PIN)){ // All zones
		#if DEBUGGING_ENABLED
		    Serial.println(F("AZ"));
		#endif
		lightState |= 0b00011100; // If not on
		while(digitalRead(AZ_PIN)){}
		delay(DEBOUNCE_DELAY);
	}
}

void sendChanges(void){
	zoneChange = 0;
	for (uint8_t i = 0; i < RECEIVER_COUNT; i++) {
		radio.stopListening();
		radio.openWritingPipe(recieverAddress[i]);
		radio.write(&lightState, 1);
		radio.stopListening();
	}
	#if DEBUGGING_ENABLED
	  Serial.println("Sent");
	#endif
}
