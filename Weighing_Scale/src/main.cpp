/* =============================================
*       CONFIGURATIONS FOR ARDUINO COMPONENTS
 * =============================================
 */
#include <Arduino.h>


// ========= DFPlayer mini mp3 module and speaker configurations =========
// connect mp3 module's SPK1, SPK2 to TRS Breakout's TIP and SLEEVE. Order does not matter
// connect mp3 module's VCC and GND to the arduino's VCC and GND
// connect a resistor on the RX pin of the mp3 module 
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
static const uint8_t PIN_MP3_TX = 4; // D4 pin, connects to mp3 module's RX 
static const uint8_t PIN_MP3_RX = 5; // D5 pin, connects to mp3 module's TX 
SoftwareSerial softwareSerial(PIN_MP3_RX, PIN_MP3_TX);
DFRobotDFPlayerMini player; // Create the mp3 player object


// ========= 4x3 keypad configurations =========
#include <Keypad.h>
const byte ROWS = 4; // four rows on the keypad
const byte COLS = 3; // three columns on the keypad 
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
// keypad facing upwards, wiring is A0 to A5, D6 from left to right of the keypad
// keypad requires digital pin, but A6, A7 cannot be used as digital (strictly analog) 
byte rowPins[ROWS] = {A0, A1, A2, A3}; // connect to the row pinouts of the keypad on A0-A3 pins
byte colPins[COLS] = {A4, A5, 6}; // connect to the column pinouts of the keypad on A4-A5, D6 pins
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
char keyPressed; // tracking which key was pressed on the keypad

// ========= TM1637 lcd panel configurations =========
#include <Arduino.h>
#include <TM1637Display.h>
const int CLK = 8; // D8 pin 
const int DIO = 9; // D9 pin 
TM1637Display display(CLK, DIO);


// ========= I/O button configurations =========
const int tarePin = 3; // D2 pin, tare button
const int readoutPin = 2; // D3 pin, readout button
const int setTargetPin = 11; // D11 pin, setTarget button
int tareState; // tracking HIGH-LOW state of tarePin
int readoutState; // tracking HIGH-LOW state of readoutPin
int setTargetState; // tracking HIGH-LOW state of setTargetPin


// ========= Piezzobuzzer configurations =========
const int buzzPin = 7; // D7 pin (positive leg)


// ========= HX711 Load Cell configurations =========
#include "HX711.h"
const int LOADCELL_DOUT_PIN = 10; // D10 pin
const int LOADCELL_SCK_PIN = 12; // D12 pin
HX711 scale; // create the scale object


// ========= variables and constants configurations =========
float currentTarget; // the target has to be keyed in through the keypad, if -1 it means no target and we will not be providing help with buzzer sounds
float currentReading; // the currently measured weight 





/* =============================================
 *                  MAIN BODY
 * =============================================
 */

void setup() {
  // ========= Commmunication with computer ========= 
  Serial.begin(115200); // Starts the serial communication
  Serial.println("Weighing scale starting up");  
  

  // ========= DFPlayer mini mp3 module and speaker init =========
  softwareSerial.begin(9600); // Init serial port for DFPlayer Mini
  if (player.begin(softwareSerial)) {
    Serial.println("DFPlayer OK"); // Start communication with DFPlayer Mini
    player.volume(25); // Set volume (0 to 30).
    player.EQ(0); // equalize volume
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }  
  playTrack(18); // say "power up"


  // ========= 4x3 keypad init ========= 


  // ========= TM1637 lcd panel init =========
  display.setBrightness(0x0f); // Sets the defaults LCD brightness
  

  // ========= I/O button init =========
  pinMode(tarePin, INPUT); // Sets the tarePin as an Input (tare button)
  pinMode(readoutPin, INPUT); // Sets the readoutPin as an Input (readout button)
  pinMode(setTargetPin, INPUT); // Sets the setTargetPin as an Input (setTarget button)


  // ========= Piezzobuzzer init =========
  pinMode(buzzPin, OUTPUT); // Sets the buzzPin as an Output (buzzer)


  // ========= HX711 Load Cell init =========
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(52600); // this value is obtained by calibrating the scale with known weights; see the README for details


  // ========= variables and constants init =========
  currentTarget = -1; // no target 
  currentReading = 0; // no reading 


  // ========= program init =========
  tare(); // reset the scale to 0  
}


void loop() {
  // ========= keep checking latest reading ========= 
  currentReading = measure(); // take the current reading from the sensor
  Serial.println("currentReading" + String(currentReading));


  // ========= keep checking if tare button was pressed ========= 
  tareState = digitalRead(tarePin);
  if (tareState == HIGH) {
    tare();
  }


  // ========= keep checking if readout button was pressed ========= 
  readoutState = digitalRead(readoutPin);
  if (readoutState == HIGH) {
    readout();
  }  


  // ========= keep checking if setTarget button was pressed ========= 
  setTargetState = digitalRead(setTargetPin);
  if (setTargetState == HIGH) {
    setTarget();
  }    


  // ========= keep checking if there is a need to sound the buzzer ========= 
  // currentTarget == -1 means that we have no currentTarget set hence no need to bother with the buzzer 
  if (currentTarget != -1) {
    buzz();
  }


  // ========= keep updating the tm1637 lcd panel ========= 
  uint8_t data[] = { 0x0, 0x0, 0x0, 0x0 };
  display.setSegments(data);
  display.showNumberDec(currentReading, false, 4, 0);
  
  
  // ========= time delay before next loop =========   
  delay(200);
}





/* =============================================
 *                  FUNCTIONALITIES
 * =============================================
 */

// ======== playTrack function =========
// uses the DFPlayer mini to play a track
void playTrack(int trackToPlay) {  
  // if player.readState() == 513 then player is playing. if player.readState() == 512 then player has stopped
  while (player.readState() == 513) {
    delay(100);
  }
  player.stop();
  player.play(trackToPlay);

  while (player.readState() == 513) {
    delay(100);
  }  
}
  
  
// ======== measure function =========
// uses the load cell to take a measurement
int measure(){
  int Reading = int(scale.get_units(10)*100);
  // scale.power_down();
  // delay(1000);
  // scale.power_up();
  return Reading;
}


// ======== tare function =========
void tare() {
  playTrack(2); // say "calibrating"
  scale.tare(); // taring
  playTrack(35); playTrack(36); // say "weight", then "zero"
  playTrack(20); // say "ready"
}


// ========= readout function ========= 
// performs and interrupt and reads out the current volume at that point in time, once
// BLUE BUTTON
void readout() {
  Serial.println("\nReading out currentReading... " + String(currentReading) + "\n");
  playTrack(4); playTrack(19); // say "current", then "reading"
  sayNumber((int)currentReading);
}


// ========= sayNumber function ========= 
// Say any number between -999,999 and 999,999 
void sayNumber(int n) {
  if (n<0) {
    playTrack(12); // say "negative"
    sayNumber(-n);
  } else if (n==0) {
    playTrack(36); // say "zero"
  } else {
    if (n>=1000) {
      int thousands = n / 1000;
      sayNumber(thousands);
      playTrack(30); // say "thousand"
      n %= 1000;
      if ((n > 0) && (n<100)) playTrack(1); // say "and"
    }
    if (n>=100) {
      int hundreds = n / 100;
      sayNumber(hundreds);
      playTrack(11); // say "hundred"
      n %= 100;
      if (n > 0) playTrack(1); // say "and"
    }
    if (n>19) {
      int tens = n / 10;
      switch (tens) {
        case 2: playTrack(33); playTrack(24); break; // say "twen", then "t"
        case 3: playTrack(29); playTrack(24); break; // say "thir", then "t"
        case 4: playTrack(9); playTrack(24); break; // say "four", then "t"
        case 5: playTrack(7); playTrack(24); break; // say "fif", then "t"
        case 6: playTrack(23); playTrack(24); break; // say "six", then "t"
        case 7: playTrack(22); playTrack(24); break; // say "seven", then "t"
        case 8: playTrack(5); playTrack(24); break; // say "eight", then "t"
        case 9: playTrack(14); playTrack(24); break; // say "nine", then "t"
      }
      n %= 10;
    }
    switch(n) {
      case 1: playTrack(16); break; // say "one"
      case 2: playTrack(34); break; // say "two"
      case 3: playTrack(31); break; // say "three"
      case 4: playTrack(9); break; // say "four"
      case 5: playTrack(8); break; // say "five"
      case 6: playTrack(23); break; // say "six"
      case 7: playTrack(22); break; // say "seven"
      case 8: playTrack(5); break; // say "eight"
      case 9: playTrack(14); break; // say "nine"
      case 10: playTrack(28); break; // say "ten"
      case 11: playTrack(6); break; // say "eleven"
      case 12: playTrack(32); break; // say "twelve"
      case 13: playTrack(29); playTrack(27); break; // say "thir", then "teen"
      case 14: playTrack(9); playTrack(27); break; // say "four", then "teen"
      case 15: playTrack(8); playTrack(27); break; // say "fif", then "teen"
      case 16: playTrack(23); playTrack(27); break; // say "six", then "teen"
      case 17: playTrack(22); playTrack(27); break; // say "seven", then "teen"
      case 18: playTrack(5); playTrack(27); break; // say "eight", then "teen"
      case 19: playTrack(14); playTrack(27); break; // say "nine", then "teen"
    }
  }
  return;
}


// ========= setTarget function ========= 
void setTarget() {
  readoutCurrentTarget();
  
  // ========= keep checking if keypad was pressed ========= 
  keyPressed = keypad.getKey();
  while (!keyPressed) {
    keyPressed = keypad.getKey();
  }  
  input(keyPressed);

  // reset the keyPressed
  keyPressed = NULL;
}


// ========= readoutTarget function ========= 
void readoutCurrentTarget() {
  if (currentTarget == -1) {
    Serial.println("no current target");
    playTrack(15); playTrack(4); playTrack(26); // say "no", then "current", then "target"
  } else {
    Serial.println("current target... " + String(currentTarget));
    playTrack(4); playTrack(26); // say "current", then "target"
    sayNumber((int)currentTarget);
  }
}


// ========= readoutTarget function ========= 
// reads out the target volume that is being keyed in right now 
// trigger with valid keypad input 
void readoutTarget(int target) {
  if (target == -1) {
    Serial.println("No target set");
    playTrack(15); playTrack(26); playTrack(35); // say "no", then "target", then "weight"
  }
  else {
    Serial.println("Reading out target... " + String(target));
    playTrack(13); playTrack(26); playTrack(35); // say "new", then "target", then "weight"
    sayNumber((int)target);
  }
}


// ========= input function ========= 
// input function for the targetWeight
void input(char keyPressed) {
  Serial.print("keyPressed: ");
  Serial.println(keyPressed);
  int target = 0; // assign temporary target

  // very first key must be a digit, else exit setTarget mode
  if (keyPressed == '#') {
    // simply exit the setTarget mode with no change
    Serial.println("no new target");
    playTrack(15); playTrack(13); playTrack(26); // say "no", then "new", then "target"
    return;
  } else if (keyPressed == '*') {
    // delete the currentTarget and exit setTarget mode
    Serial.println("setting zero target");
    currentTarget = -1; 
    playTrack(21); playTrack(36); playTrack(26); // say "setting", then "zero", then "target"
    return;
  } else {
    target = target * 10 + String(keyPressed).toInt();
    readoutTarget(target);
  }


  // loop and keep reading new input until new target is confirmed
  char key = keyPressed;

  while (1) {
    key = keypad.getKey();

    if (key) {
      // terminating condition: # key will be our terminating character and will confirm our targetWeight 
      if (key == '#') {
        break;
      }
  
      // backspace condition: * key will be our backspace character
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

  // set the currentTarget to the confirmed target 
  currentTarget = target;
  playTrack(21); // say "setting"
  readoutCurrentTarget();
  return;
}


// ========= buzz function ========= 
// controls the buzzing tones for volume indications (close, overshot, hit)
// 1 - normal. 2 - close (approaching targetWeight). 3 - overshot (went over targetWeight). 4 - hit (on targetWeight).
void buzz() {
  float difference = currentTarget - currentReading;

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