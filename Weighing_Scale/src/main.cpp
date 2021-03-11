/* =============================================
 *                  CONFIGURATIONS
 * =============================================
 */

#include <Arduino.h>
 
// ========= 4x3 keypad configurations =========
#include <Keypad.h>

const byte ROWS = 4; // four rows
const byte COLS = 3; // three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {A0, A1, A2, A3}; // connect to the row pinouts of the keypad, A0 to A3 pins (brown, red, orange, yellow)
//byte colPins[COLS] = {A4, A5, 8}; // connect to the column pinouts of the keypad, A4, A5, D8 pins (green, blue, purple)
byte colPins[COLS] = {A4, A5, A6}; // connect to the column pinouts of the keypad, A4, A5, D2 pins (green, blue, purple)
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );


// ========= tm1637 lcd panel configurations =========
#include <Arduino.h>
#include <TM1637Display.h>
const int CLK = 8; // D8 pin (yellow)
const int DIO = 9; // D9 pin (orange)
TM1637Display display(CLK, DIO);

// ========= button configurations =========
const int tarePin = 3; // D4 pin, red button (wire from under resistor)
const int readoutPin = 2; // D5 pin, blue button (wire from under resistor)
const int targetVolPin = 4; // D6 pin, yellow button (wire from under resistor)


// ========= buzzer configurations =========
const int buzzPin = 7; // D7 pin (positive leg)


// ========= speaker configurations =========
// D11 pin, orange wire (TIP)
// GROUND, yellow wire (SLEEVE)
// capacitor between SLEEVE and TIP (ground and digital pin
// default output to D3, cannot change the output digital pin
// default input is D11, LEAVE EMPTY and don't put in other devices to this pin, else code will break

// Go to the Talkie library, and find the C++ header files to get the variable names for the words 
#include "Talkie.h"
#include "Vocab_US_Large.h"
Talkie voice;

// ========= HX711 Load Cell configurations =========
#include "HX711.h"
const int LOADCELL_DOUT_PIN = 10;
const int LOADCELL_SCK_PIN = 12;

HX711 scale;

// ========= variables and constants =========

// helping variables
float targetWeight = -1; // the target volume has to be keyed in through the keypad, if -1 it means no target and we will not be providing help with buzzer sounds

// measured variables
float currentReading = 0; // the currently measured weight 
//float tareDistance = -1; // the current tared zero height, if -1 it means it hasn't been tared yet
//float currentVolume = 0; // the currently measured volume


/* =============================================
 *                  FUNCTIONALITIES
 * =============================================
 */

// ======== Measure function =========
// uses the load cell to take a measurement
int measure(){
  int Reading = int(scale.get_units(10)*100);
  scale.power_down();
  delay(1000);
  scale.power_up();
  return Reading;
}

void tare(){
  scale.tare();
  voice.say(sp3_ZERO);
}

// ========= sayNumber function ========= 
// Say any number between -999,999 and 999,999 
void sayNumber(int n) {
  if (n<0) {
    voice.say(sp2_MINUS);
    sayNumber(-n);
  } else if (n==0) {
    voice.say(sp2_ZERO);
  } else {
    if (n>=1000) {
      int thousands = n / 1000;
      sayNumber(thousands);
      voice.say(sp2_THOUSAND);
      n %= 1000;
      if ((n > 0) && (n<100)) voice.say(sp2_AND);
    }
    if (n>=100) {
      int hundreds = n / 100;
      sayNumber(hundreds);
      voice.say(sp2_HUNDRED);
      n %= 100;
      if (n > 0) voice.say(sp2_AND);
    }
    if (n>19) {
      int tens = n / 10;
      switch (tens) {
        case 2: voice.say(sp2_TWENTY); break;
        case 3: voice.say(sp2_THIR_); voice.say(sp2_T); break;
        case 4: voice.say(sp2_FOUR); voice.say(sp2_T);  break;
        case 5: voice.say(sp2_FIF_);  voice.say(sp2_T); break;
        case 6: voice.say(sp2_SIX);  voice.say(sp2_T); break;
        case 7: voice.say(sp2_SEVEN);  voice.say(sp2_T); break;
        case 8: voice.say(sp2_EIGHT);  voice.say(sp2_T); break;
        case 9: voice.say(sp2_NINE);  voice.say(sp2_T); break;
      }
      n %= 10;
    }
    switch(n) {
      case 1: voice.say(sp2_ONE); break;
      case 2: voice.say(sp2_TWO); break;
      case 3: voice.say(sp2_THREE); break;
      case 4: voice.say(sp2_FOUR); break;
      case 5: voice.say(sp2_FIVE); break;
      case 6: voice.say(sp2_SIX); break;
      case 7: voice.say(sp2_SEVEN); break;
      case 8: voice.say(sp2_EIGHT); break;
      case 9: voice.say(sp2_NINE); break;
      case 10: voice.say(sp2_TEN); break;
      case 11: voice.say(sp2_ELEVEN); break;
      case 12: voice.say(sp2_TWELVE); break;
      case 13: voice.say(sp2_THIR_); voice.say(sp2__TEEN); break;
      case 14: voice.say(sp2_FOUR); voice.say(sp2__TEEN);break;
      case 15: voice.say(sp2_FIF_); voice.say(sp2__TEEN); break;
      case 16: voice.say(sp2_SIX); voice.say(sp2__TEEN); break;
      case 17: voice.say(sp2_SEVEN); voice.say(sp2__TEEN); break;
      case 18: voice.say(sp2_EIGHT); voice.say(sp2__TEEN); break;
      case 19: voice.say(sp2_NINE); voice.say(sp2__TEEN); break;
    }
  }
  return;
}

// ========= readout function ========= 
// performs and interrupt and reads out the current volume at that point in time, once
// BLUE BUTTON
void readout() {
  Serial.println("\nReading out volume... " + String(currentReading) + "\n");
  voice.say(sp3_CURRENT);
  voice.say(sp2_VAL);
  sayNumber((int)currentReading);
}


// ========= readoutTarget function ========= 
// reads out the target volume that is being keyed in right now 
// trigger with valid keypad input 
void readoutTarget(int target) {
  if (target == -1) {
    Serial.println("No target set");
    voice.say(sp4_NO);
    voice.say(sp4_TARGET);
  }
  else {
    Serial.println("Reading out target... " + String(target));
    voice.say(sp5_NEW);
    voice.say(sp4_TARGET);
    sayNumber((int)target);
  }
}


// ========= readoutTargetWeight function ========= 
// reads out the target weight that has been set
void readoutTargetWeight() {
  if (targetWeight == -1) {
    Serial.println("No target weight set");
    voice.say(sp4_NO);
    voice.say(sp4_TARGET);
    voice.say(sp2_VAL);
  }
  else {
    Serial.println("Reading out weight volume... " + String(targetWeight));
    voice.say(sp4_TARGET);
    voice.say(sp2_VAL);
    sayNumber((int)targetWeight);
  }
}



// ========= targetVol function ========= 
// YELLOW BUTTON
void targetVol() {
  readoutTargetWeight();
}


// ========= input function ========= 
// input function for the targetVolume
void input(char firstKey) {
  int target = 0;

  // very first key must be a digit, else exit input mode
  if (firstKey == '*' or firstKey == '#') {
    targetWeight = -1;
    Serial.println("Target weight cleared");
    readoutTarget(targetWeight);
    return;
  } else {
    target = target * 10 + String(firstKey).toInt();
    readoutTarget(target);
  }

  char key = firstKey;

  // # key will be our terminating character and will confirm our targetVolume 
  while (1) {
    key = keypad.getKey();

    if (key) {
      // terminating condition
      if (key == '#') {
        break;
      }
  
      // backspace condition
      else if (key == '*') {
        target /= 10;
        readoutTarget(target);
      }
    
      // digits will add into our input
      else {
        if (target < 1000) {
          target = target * 10 + String(key).toInt();
          readoutTarget(target);
        }
      }
    }
  }

  // set the target volume
  targetWeight = target;
  voice.say(sp5_SET);
  readoutTargetWeight();
  return;
}



// ========= buzz function ========= 
// controls the buzzing tones for volume indications (close, overshot, hit)
// 1 - normal. 2 - close (approaching targetVolume). 3 - overshot (went over targetVolume). 4 - hit (on targetVolume).
void buzz() {
  float difference = targetWeight - currentReading;

  // normal mode, no sound
  if (difference >= 50) {
    // pass  
  } 
  // approaching mode, fast beep
  else if (difference >= 15 and difference < 50) {
    tone(buzzPin, 311); //D#
    delay(200);
    noTone(buzzPin);
    delay(100);
  } 
  // hit mode, target volume reached, flatline
  else if (difference >= -15 and difference < 15) {
    tone(buzzPin, 440); //A
    delay(2000);
    noTone(buzzPin);
    delay(500);
  } 
  // overshot mode, offbeat
  else if (difference < -15) {
    tone(buzzPin, 440); //F#
    delay(50);
    tone(buzzPin, 350); //F#
    delay(50);
    tone(buzzPin, 440); //F#
    delay(50);
    tone(buzzPin, 350); //F#
    delay(50);    
    noTone(buzzPin);
    delay(500);
  }
}



/* =============================================
 *                  MAIN BODY
 * =============================================
 */

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // Starts the serial communication
  Serial.println("Weighing scale starting up");
  voice.doNotUseNonInvertedOutput();
  voice.say(sp4_TURN);
  voice.say(sp2_ON);

  // pin settings
  //pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output (ultrasonic sensor)
  //pinMode(echoPin, INPUT); // Sets the echoPin as an Input (ultrasonic sensor)
  pinMode(tarePin, INPUT); // Sets the tarePin as an Input (tare button)
  pinMode(readoutPin, INPUT); // Sets the readoutPin as an Input (readout button)
  pinMode(targetVolPin, INPUT); // Sets the targetVolPin as an Input (targetVol button)
  pinMode(buzzPin, OUTPUT); // Sets the buzzPin as an Output (buzzer)
  attachInterrupt(digitalPinToInterrupt(tarePin), tare, FALLING);
  attachInterrupt(digitalPinToInterrupt(readoutPin), readout, FALLING);

  // lcd settings
  display.setBrightness(0x0f); // Sets the defaults LCD brightness

  //voice.say(sp2_CALIBRATE);
  // reset the device. perform measurements and tare everything
  //tare();
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(52600);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  tare();               // reset the scale to 0
  delay(100);
  voice.say(sp2_READY);
}

void loop() {
  // put your main code here, to run repeatedly:
  currentReading = measure(); // take the current distance
  Serial.println("currentReading" + String(currentReading));

  // ========= tare ========= 
  int tareState = digitalRead(tarePin);
  if (tareState == HIGH) {
    tare();
  }

  // ========= readout ========= 
  int readoutState = digitalRead(readoutPin);
  if (readoutState == HIGH) {
    readout();
  }  

  // // ========= targetVol ========= 
  // int targetVolState = digitalRead(targetVolPin);
  // if (targetVolState == HIGH) {
  //   targetVol();
  // }    

  // ========= input ========= 
  char firstKey = keypad.getKey();
  if (firstKey) {
    input(firstKey);
  }


  // ========= measuring to targetWeight ========= 
  // targetWeight == -1 means that we have no targetWeight set hence no need to bother with the buzzer nonsense
  if (targetWeight != -1) {
    buzz();
  }

  

  // ========= tm1637 lcd panel ========= 
  uint8_t data[] = { 0x0, 0x0, 0x0, 0x0 };
  display.setSegments(data);
  display.showNumberDec(currentReading, false, 4, 0);
  //display.showNumberDec(currentDistance, false, 4, 0);
  delay(500);
}