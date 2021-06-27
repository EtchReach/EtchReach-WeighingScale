/* =============================================
*       CONFIGURATIONS FOR ARDUINO COMPONENTS
 * =============================================
 */
#include <Arduino.h>


// ========= Vocabulary =========
#include "TTS_AUDIO.h"


// ========= DFPlayer mini mp3 module and speaker configurations =========
// connect mp3 module's SPK1, SPK2 to TRS Breakout's TIP and SLEEVE. Order does not matter
// connect mp3 module's VCC and GND to the arduino's VCC and GND
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
static const uint8_t mp3TxPin = 4; // D4 pin, connects to mp3 module's RX 
static const uint8_t mp3RxPin = 5; // D5 pin, connects to mp3 module's TX 
SoftwareSerial softwareSerial(mp3RxPin, mp3TxPin);
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
int tareState; // tracking HIGH-LOW state of tarePin
int readoutState; // tracking HIGH-LOW state of readoutPin


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
  Serial.begin(115200); // starts the serial communication
  Serial.println("Weighing scale starting up");  
  

  // ========= DFPlayer mini mp3 module and speaker init =========
  softwareSerial.begin(9600); // initialize serial port for DFPlayer Mini
  if (player.begin(softwareSerial)) {
    Serial.println("DFPlayer OK"); // start communication with DFPlayer Mini
    player.volume(15); // set volume (0 to 30).
    player.EQ(0); // equalize volume
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }  
  playTrack(mp3_POWER_UP); 


  // ========= 4x3 keypad init ========= 


  // ========= TM1637 lcd panel init =========
  display.setBrightness(0x0f); // sets the defaults LCD brightness
  

  // ========= I/O button init =========
  pinMode(tarePin, INPUT); // sets the tarePin as an Input (tare button)
  pinMode(readoutPin, INPUT); // sets the readoutPin as an Input (readout button)


  // ========= Piezzobuzzer init =========
  pinMode(buzzPin, OUTPUT); // sets the buzzPin as an Output (buzzer)


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


  // ========= keep checking if the keypad was pressed ========= 
  keyPressed = keypad.getKey();
  if (keyPressed) {
    target(keyPressed);
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
  Serial.println("\nplayTrack() - track to play: " + String(trackToPlay));

  // if player.readState() == 513 then player is playing. if player.readState() == 512 then player has stopped
  while (player.readState() == 513) {
    Serial.println("playTrack() - player busy"); 
    delay(100);
  }

  Serial.println("playTrack() - playing now: " + String(trackToPlay)); 
  player.stop();
  player.play(trackToPlay);

  while (player.readState() == 513) {
    Serial.println("playTrack() - player busy");
    delay(100);
  }  

  Serial.print("\nplayTrack() - finished playing\n");
}
  
  
// ======== measure function =========
// uses the load cell to take a measurement
int measure(){
  int Reading = int(scale.get_units(10)*100);
  Serial.println("measure() - currentReading: " + String(currentReading));
  return Reading;
}


// ======== tare function =========
void tare() {
  Serial.println("\ntare() - start taring");
  playTrack(mp3_CALIBRATING); 
  scale.tare(); // taring
  playTrack(mp3_WEIGHT); playTrack(mp3_ZERO); 
  playTrack(mp3_READY); 
  Serial.println("tare() - finish taring\n");
}


// ========= readout function ========= 
// performs and interrupt and reads out the current reading at that point in time, once
// BLUE BUTTON
void readout() {
  Serial.println("\nreadout() - current reading: " + String(currentReading) + "\n");
  playTrack(mp3_CURRENT); playTrack(mp3_READING); 
  sayNumber((int)currentReading);
}


// ========= sayNumber function ========= 
// Say any number between -999,999 and 999,999 
void sayNumber(int n) {
  if (n<0) {
    playTrack(mp3_NEGATIVE); 
    sayNumber(-n);
  } else if (n==0) {
    playTrack(mp3_ZERO); 
  } else {
    if (n>=1000) {
      int thousands = n / 1000;
      sayNumber(thousands);
      playTrack(mp3_THOUSAND); 
      n %= 1000;
      if ((n > 0) && (n<100)) playTrack(mp3_AND); 
    }
    if (n>=100) {
      int hundreds = n / 100;
      sayNumber(hundreds);
      playTrack(mp3_HUNDRED); 
      n %= 100;
      if (n > 0) playTrack(mp3_AND); 
    }
    if (n>19) {
      int tens = n / 10;
      switch (tens) {
        case 2: playTrack(mp3_TWEN); playTrack(mp3_T); break; 
        case 3: playTrack(mp3_THIR); playTrack(mp3_T); break; 
        case 4: playTrack(mp3_FOUR); playTrack(mp3_T); break; 
        case 5: playTrack(mp3_FIF); playTrack(mp3_T); break; 
        case 6: playTrack(mp3_SIX); playTrack(mp3_T); break; 
        case 7: playTrack(mp3_SEVEN); playTrack(mp3_T); break; 
        case 8: playTrack(mp3_EIGHT); playTrack(mp3_T); break; 
        case 9: playTrack(mp3_NINE); playTrack(mp3_T); break; 
      }
      n %= 10;
    }
    switch(n) {
      case 1: playTrack(mp3_ONE); break; 
      case 2: playTrack(mp3_TWO); break; 
      case 3: playTrack(mp3_THREE); break;
      case 4: playTrack(mp3_FOUR); break; 
      case 5: playTrack(mp3_FIVE); break; 
      case 6: playTrack(mp3_SIX); break; 
      case 7: playTrack(mp3_SEVEN); break;
      case 8: playTrack(mp3_EIGHT); break;
      case 9: playTrack(mp3_NINE); break; 
      case 10: playTrack(mp3_TEN); break; 
      case 11: playTrack(mp3_ELEVEN); break; 
      case 12: playTrack(mp3_TWELVE); break; 
      case 13: playTrack(mp3_THIR); playTrack(mp3_TEEN); break; 
      case 14: playTrack(mp3_FOUR); playTrack(mp3_TEEN); break; 
      case 15: playTrack(mp3_FIF); playTrack(mp3_TEEN); break; 
      case 16: playTrack(mp3_SIX); playTrack(mp3_TEEN); break; 
      case 17: playTrack(mp3_SEVEN); playTrack(mp3_TEEN); break;
      case 18: playTrack(mp3_EIGHT); playTrack(mp3_TEEN); break;
      case 19: playTrack(mp3_NINE); playTrack(mp3_TEEN); break; 
    }
  }
  return;
}


// ========= setTarget function ========= 
void target(char keyPressed) {
  if (keyPressed == '#') {
    // # key will read out the current target that has been set if there is one, else state that there isn't one
    if (currentTarget == -1) {
      Serial.println("\ntarget() - # pressed, no current target has been set\n");
      playTrack(mp3_NO); playTrack(mp3_CURRENT); playTrack(mp3_TARGET); 
    } else {
      Serial.println("\ntarget() - # pressed, current target is: " + String(currentTarget) + "\n");
      playTrack(mp3_CURRENT); playTrack(mp3_TARGET); 
      sayNumber(currentTarget);
    }
  } else if (keyPressed == '*') {
    // # key will read delete the current target if one has been set, or state that there isn't one
    Serial.println("\ntarget() - * pressed, target has been cleared\n");
    currentTarget = -1; 
    playTrack(mp3_SETTING); playTrack(mp3_ZERO); playTrack(mp3_TARGET); 
  } else {
    // digit key will start the target setting process
    Serial.println("\ntarget() - digit pressed, now setting a target\n");
    setTarget(keyPressed);
  }  
}


// ========= setTarget function ========= 
// setTarget function for the target value that will trigger buzzing 
void setTarget(char keyPressed) {
  int tempTarget = String(keyPressed).toInt(); // assign temporary target to the first digit that was pressed
  Serial.println("setTarget() - new digit, new tempTarget: " + String(tempTarget));
  playTrack(mp3_NEW); playTrack(mp3_TARGET); 
  sayNumber(tempTarget);

  // loop and keep reading new input until new target is confirmed
  while (1) {
    keyPressed = keypad.getKey();

    if (keyPressed) {
      Serial.println("setTarget() - keyPressed: " + String(keyPressed));
      // terminating condition: # key will be our terminating character and will set the currentTarget to the confirmed target 
      if (keyPressed == '#') {
        currentTarget = tempTarget;   
        Serial.println("setTarget() - exiting input with new target confirmed");
        playTrack(mp3_SETTING); playTrack(mp3_TARGET); 
        sayNumber(tempTarget);     
        break;
      }
  
      // backspace condition: * key will be our backspace character, and if we backspace a single digit, then we exit the mode with no changes to the currentTarget
      else if (keyPressed == '*') {
        tempTarget /= 10;
        if (tempTarget == 0) {
          Serial.println("setTarget() - exiting input with no change to target");
          playTrack(mp3_NO); playTrack(mp3_NEW); playTrack(mp3_TARGET);
          break;
        } else {
          Serial.println("setTarget() - backspace, new tempTarget: " + String(tempTarget));
          playTrack(mp3_NEW); playTrack(mp3_TARGET);
          sayNumber(tempTarget);
        }
      }
    
      // digits will add into our input
      else {
        if (tempTarget < 1000) {
          tempTarget = tempTarget * 10 + String(keyPressed).toInt();
          Serial.println("setTarget() - new digit, new tempTarget: " + String(tempTarget));
          playTrack(mp3_NEW); playTrack(mp3_TARGET); 
          sayNumber(tempTarget);
        } else {
          Serial.println("\nsetTarget() - exceeded maximum target allowable: only up to 4 digits\n");
          playTrack(mp3_EXCEEDED_MAXIMUM_ALLOWABLE_INPUT);
        }
      }
    }
  }
}


// ========= buzz function ========= 
// controls the buzzing tones for target indications (close, overshot, hit)
// 1 - normal. 2 - close (approaching targetWeight). 3 - overshot (went over targetWeight). 4 - hit (on targetWeight).
void buzz() {
  float difference = currentTarget - currentReading;

  // normal mode, no sound
  if (difference >= 50) {
    // pass  
  } 
  // approaching mode, fast beep
  else if (difference >= 15 and difference < 50) {
    Serial.println("\nbuzz() - approaching target\n");
    tone(buzzPin, 311); //D#
    delay(200);
    noTone(buzzPin);
    delay(100);
  } 
  // hit mode, target reading reached, flatline
  else if (difference >= -15 and difference < 15) {
    Serial.println("\nbuzz() - reached target\n");
    tone(buzzPin, 440); //A
    delay(1000);
    noTone(buzzPin);
    delay(500);
  } 
  // overshot mode, offbeat
  else if (difference < -15) {
    Serial.println("\nbuzz() - overshot target\n");
    playTrack(mp3_OVERSHOT);
    tone(buzzPin, 440); //F#
    delay(50);
    tone(buzzPin, 350); //F#
    delay(50);
    noTone(buzzPin);
    delay(500);
  }
}