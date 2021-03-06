/* =============================================
*       CONFIGURATIONS FOR ARDUINO COMPONENTS
 * =============================================
 */
#include <Arduino.h>


// ========= Vocabulary =========
// this library is obtained from running the tts_audio_generation.py script in the TTS_AUDIO folder
#include "TTS_AUDIO.h"


// ========= Pololu Mini Pushbutton Power Switch =========
// Used to turn on and off the weighing scale
// integrate with a timer to have automatic shutdown after period of inactivity
const int pololuPin = 11; // D11 pin
unsigned long lastActive; // the number of milliseconds passed since turning on that the Arduino board was last active
unsigned long timeout = 120000; // the number of milliseconds that has to pass since lastActive in order to timeout and shut down the device 


// ========= DFPlayer mini mp3 module and speaker configurations =========
// connect mp3 module's SPK1, SPK2 to TRS Breakout's TIP and SLEEVE. Order does not matter
// connect mp3 module's VCC and GND to the arduino's VCC and GND
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
const int mp3TxPin = 4; // D4 pin, connects to mp3 module's RX 
const int mp3RxPin = 5; // D5 pin, connects to mp3 module's TX 
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
const byte rowPins[ROWS] = {A0, A1, A2, A3}; // connect to the row pinouts of the keypad on A0-A3 pins
const byte colPins[COLS] = {A4, A5, 6}; // connect to the column pinouts of the keypad on A4-A5, D6 pins
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


  // ========= Pololu Mini Pushbutton Power Switch =========
  pinMode(pololuPin, OUTPUT); // sets the pololuPin as an Output
  lastActive = millis(); // updates the last active time of the Arduino board


  // ========= DFPlayer mini mp3 module and speaker init =========
  softwareSerial.begin(9600); // initialize serial port for DFPlayer Mini
  if (player.begin(softwareSerial)) {
    Serial.println("DFPlayer OK"); // start communication with DFPlayer Mini
    player.volume(20); // set volume (0 to 30).
    player.EQ(0); // equalize volume
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }


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
  playTrack(wav_WEIGHING_SCALE_IS_TURNING_ON);
  tare(); // reset the scale to 0  
}


void loop() {
  // ========= keep checking latest reading and updating the tm1637 lcd panel =========
  Serial.println("Last active: " + String(lastActive)); 
  currentReading = measure(); // take the current reading from the sensor
  displayNumber(currentReading); // show the reading on the display
  

    // ========= keep checking if tare button was pressed ========= 
  tareState = digitalRead(tarePin);
  if (tareState == HIGH) {
    lastActive = millis();
    tare();
  }

  // ========= keep checking if readout button was pressed ========= 
  readoutState = digitalRead(readoutPin);
  if (readoutState == HIGH) {
    lastActive = millis();    
    readout(currentReading, 'c');
  }

  // ========= keep checking if the keypad was pressed ========= 
  keyPressed = keypad.getKey();
  if (keyPressed) {
    lastActive = millis();
    target(keyPressed);
  }

  // ========= keep checking if there is a need to sound the buzzer ========= 
  // currentTarget == -1 means that we have no currentTarget set hence no need to bother with the buzzer 
  if (currentTarget != -1) {
    // update lastActive inside buzz() on conditions that cause a buzz to occur
    // long periods of no buzzing should not contribute and should not reset the timeout
    buzz();
  } 

  // ========= keep checking if the time since last active has exceeded timeout setting =========
  Serial.println("time passed: " + String(millis() - lastActive));
  if (millis() - lastActive > timeout) {
    Serial.println("Device timeout... shutting down...");
    playTrack(wav_TIMEOUT);
    delay(1000);
    playTrack(wav_TURNING_OFF);
    delay(1000);
    digitalWrite(pololuPin, HIGH);   // Sets the pin to HIGH and shuts down the Arduino
  }
}





/* =============================================
 *                  FUNCTIONALITIES
 * =============================================
 */

// ======== playTrack function =========
// uses the DFPlayer mini to play a track
void playTrack(int trackToPlay) {
  Serial.println("\nplayTrack() - track to play: " + String(trackToPlay));
  delay(50);

  // if player.readState() == 513 then player is playing. if player.readState() == 512 then player has stopped
  while (player.readState() == 513) {
    Serial.println("playTrack() - player busy"); 
    delay(200);
  }

  Serial.println("playTrack() - playing now: " + String(trackToPlay)); 
  player.stop();
  player.play(trackToPlay);

  while (player.readState() == 513) {
    Serial.println("playTrack() - player busy");
    delay(200);
  }  

  Serial.print("\nplayTrack() - finished playing\n");

  delay(50);
}

  
// ======== measure function =========
// uses the load cell to take a measurement
int measure(){
  int Reading = int(scale.get_units(10)*100);
  Serial.println("measure() - currentReading: " + String(currentReading));
  return Reading;
}


// ======== display function =========
// shows the current reading on the 
void displayNumber(int n) {
  uint8_t data[] = { 0x0, 0x0, 0x0, 0x0 };
  display.setSegments(data);
  display.showNumberDec(n, false, 4, 0);
}


// ======== tare function =========
void tare() {
  Serial.println("\ntare() - start taring");
  playTrack(wav_CALIBRATING_SCALE); 
  scale.tare(); // taring
  playTrack(wav_READY); 
  Serial.println("tare() - finish taring\n");
}


// ========= readout function ========= 
// will say the current value that is passed to it when the readout button is pressed
// could be either the actual reading or a pending input, indicated by char c (this is not really necessary, just for a print statement switch)
void readout(int n, char c) {  
  // want to read out the current reading
  if (c == 'r') {
    Serial.println("\nreadout() - current reading: " + String(n) + "\n");
  } else if (c =='i') {
    Serial.println("\nreadout() - current target being typed: " + String(n) + "\n");
  }
  sayNumber((int)n);
  playTrack(wav_GRAMS);    
}


// ========= sayNumber function ========= 
// Say any number between -999,999 and 999,999 
void sayNumber(int n) {
  if (n<0) {
    playTrack(wav_NEGATIVE); 
    sayNumber(-n);
  } else if (n==0) {
    playTrack(wav_ZERO); 
  } else {
    if (n>=1000) {
      int thousands = n / 1000;
      switch(thousands) {
        case 1: playTrack(wav_ONE_THOUSAND); break; 
        case 2: playTrack(wav_TWO_THOUSAND); break; 
        case 3: playTrack(wav_THREE_THOUSAND); break;
        case 4: playTrack(wav_FOUR_THOUSAND); break; 
        case 5: playTrack(wav_FIVE_THOUSAND); break; 
        case 6: playTrack(wav_SIX_THOUSAND); break; 
        case 7: playTrack(wav_SEVEN_THOUSAND); break;
        case 8: playTrack(wav_EIGHT_THOUSAND); break;
        case 9: playTrack(wav_NINE_THOUSAND); break; 
      }
      n %= 1000;
      delay(100);
      // if ((n > 0) && (n<100)) playTrack(wav_AND); 
    }
    if (n>=100) {
      int hundreds = n / 100;
      switch(hundreds) {
        case 1: playTrack(wav_ONE_HUNDRED); break; 
        case 2: playTrack(wav_TWO_HUNDRED); break; 
        case 3: playTrack(wav_THREE_HUNDRED); break;
        case 4: playTrack(wav_FOUR_HUNDRED); break; 
        case 5: playTrack(wav_FIVE_HUNDRED); break; 
        case 6: playTrack(wav_SIX_HUNDRED); break; 
        case 7: playTrack(wav_SEVEN_HUNDRED); break;
        case 8: playTrack(wav_EIGHT_HUNDRED); break;
        case 9: playTrack(wav_NINE_HUNDRED); break; 
      }
      n %= 100;
      delay(100);
      // if (n > 0) playTrack(wav_AND); 
    }
    if (n>19) {
      int tens = n / 10;
      switch (tens) {
        case 2: playTrack(wav_TWENTY); break; 
        case 3: playTrack(wav_THIRTY); break; 
        case 4: playTrack(wav_FOURTY); break; 
        case 5: playTrack(wav_FIFTY); break; 
        case 6: playTrack(wav_SIXTY); break; 
        case 7: playTrack(wav_SEVENTY); break; 
        case 8: playTrack(wav_EIGHTY); break; 
        case 9: playTrack(wav_NINETY); break; 
      }
      n %= 10;
      delay(100);
    }
    switch(n) {
      case 1: playTrack(wav_ONE); break; 
      case 2: playTrack(wav_TWO); break; 
      case 3: playTrack(wav_THREE); break;
      case 4: playTrack(wav_FOUR); break; 
      case 5: playTrack(wav_FIVE); break; 
      case 6: playTrack(wav_SIX); break; 
      case 7: playTrack(wav_SEVEN); break;
      case 8: playTrack(wav_EIGHT); break;
      case 9: playTrack(wav_NINE); break; 
      case 10: playTrack(wav_TEN); break; 
      case 11: playTrack(wav_ELEVEN); break; 
      case 12: playTrack(wav_TWELVE); break; 
      case 13: playTrack(wav_THIRTEEN); break; 
      case 14: playTrack(wav_FOURTEEN); break; 
      case 15: playTrack(wav_FIFTEEN); break; 
      case 16: playTrack(wav_SIXTEEN); break; 
      case 17: playTrack(wav_SEVENTEEN); break;
      case 18: playTrack(wav_EIGHTEEN); break;
      case 19: playTrack(wav_NINETEEN); break; 
    }
    delay(100);
  }
  return;
}


// ========= setTarget function ========= 
void target(char keyPressed) {
  if (keyPressed == '#') {
    // # key will read out the current target that has been set if there is one, else state that there isn't one
    if (currentTarget == -1) {
      Serial.println("\ntarget() - # pressed, no current target has been set\n");
      playTrack(wav_NO_TARGET_HAS_BEEN_SET);
    } else {
      Serial.println("\ntarget() - # pressed, current target is: " + String(currentTarget) + "\n");
      playTrack(wav_CURRENT_TARGET_SET_IS);
      sayNumber(currentTarget);
    }
  } else if (keyPressed == '*') {
    // # key will read delete the current target if one has been set, or state that there isn't one
    Serial.println("\ntarget() - * pressed, target has been cleared\n");
    currentTarget = -1;
    playTrack(wav_CURRENT_TARGET_REMOVED);
  } else {
    // digit key will start the target setting process
    Serial.println("\ntarget() - digit pressed, now setting a target\n");
    playTrack(wav_SETTING_TARGET);
    setTarget(keyPressed);
  }  
}


// ========= setTarget function ========= 
// setTarget function for the target value that will trigger buzzing 
void setTarget(char keyPressed) {
  int tempTarget = String(keyPressed).toInt(); // assign temporary target to the first digit that was pressed
  Serial.println("setTarget() - new digit, new tempTarget: " + String(tempTarget));
  playTrack(wav_INPUT_TARGET);

  // switch the display and voice to show the target being keyed in instead
  displayNumber(tempTarget);
  sayNumber(tempTarget);

  // loop and keep reading new input until new target is confirmed
  while (1) {
    // ========= keep checking if readout button was pressed ========= 
    readoutState = digitalRead(readoutPin);
    if (readoutState == HIGH) readout(tempTarget, 'i');    

    keyPressed = keypad.getKey();

    if (keyPressed) {
      Serial.println("setTarget() - keyPressed: " + String(keyPressed));
      // terminating condition: # key will be our terminating character and will set the currentTarget to the confirmed target 
      if (keyPressed == '#') {
        currentTarget = tempTarget;   
        Serial.println("setTarget() - exiting input with new target confirmed");
        playTrack(wav_NEW_TARGET_CONFIRMED);
        sayNumber(tempTarget);     
        break;
      }
  
      // backspace condition: * key will be our backspace character, and if we backspace a single digit, then we exit the mode with no changes to the currentTarget
      else if (keyPressed == '*') {
        tempTarget /= 10;
        if (tempTarget == 0) {
          Serial.println("setTarget() - exiting input with no change to target");
          playTrack(wav_CANCELLING_INPUT);
          playTrack(wav_TARGET_UNCHANGED);
          break;
        } else {
          Serial.println("setTarget() - backspace, new tempTarget: " + String(tempTarget));
          playTrack(wav_INPUT_TARGET);

          // switch the display show the target currently being keyed in
          displayNumber(tempTarget);              
          sayNumber(tempTarget);
        }
      }
    
      // digits will add into our input
      else {
        if (tempTarget < 1000) {
          tempTarget = tempTarget * 10 + String(keyPressed).toInt();
          Serial.println("setTarget() - new digit, new tempTarget: " + String(tempTarget));
          playTrack(wav_INPUT_TARGET);

          // switch the display show the target currently being keyed in
          displayNumber(tempTarget);              
          sayNumber(tempTarget);
        } else {
          Serial.println("\nsetTarget() - exceeded maximum target allowable: only up to 4 digits\n");
          playTrack(wav_EXCEEDED_MAXIMUM_ALLOWABLE_INPUT);
        }
      }
    }
  }
}


// ========= buzz function ========= 
// controls the buzzing tones for target indications (close, overshot, hit)
// 1 - normal. 2 - close (approaching targetWeight). 3 - overshot (went over targetWeight). 4 - hit (on targetWeight).
// timeout is updated only in sutations that buzz
void buzz() {
  // ========= keep checking latest reading and updating the tm1637 lcd panel ========= 
  currentReading = measure(); // take the current reading from the sensor
  displayNumber(currentReading); // show the reading on the display

  float difference = currentTarget - currentReading;

  // normal mode, no sound
  if (difference >= 50) {
    // pass  
  } 
  // approaching mode, fast beep
  else if (difference >= 15 and difference < 50) {
    lastActive = millis();

    Serial.println("\nbuzz() - approaching target\n");
    tone(buzzPin, 311);
    delay(200);
    noTone(buzzPin);
    delay(500);
  } 
  // hit mode, target reading reached, flatline
  else if (difference >= -15 and difference < 15) {
    lastActive = millis();

    Serial.println("\nbuzz() - reached target\n");
    playTrack(wav_TARGET_REACHED);
    tone(buzzPin, 440);
    delay(1000);
    noTone(buzzPin);
    delay(250);
  } 
  // overshot mode, offbeat
  else if (difference < -15) {
    lastActive = millis();

    Serial.println("\nbuzz() - overshot target\n");
    playTrack(wav_OVERSHOT_TARGET);
    tone(buzzPin, 440);
    delay(50);
    tone(buzzPin, 350);
    delay(50);
    tone(buzzPin, 440);
    delay(50);
    tone(buzzPin, 350);
    delay(50);    
    noTone(buzzPin);
    delay(200);
  }
}