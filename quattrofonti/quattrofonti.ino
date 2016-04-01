
#include "Wire.h"       //i2c lib to drive the quad digipot chip
#include <TimerOne.h>   //Timer lib to generate interrupt callback
#include <SPI.h>         // Serial peripheral interface
#include <DAC_MCP49xx.h> // DAC libs (remember we use an external ref voltage of 2.5V)

DAC_MCP49xx dac(DAC_MCP49xx::MCP4922, 4, -1);   //NS1nanosynth has DAC SS on pin D4

#define MIN_NOTE 36   //lowest midi note that the synth will respond to normally
#define MAX_NOTE MIN_NOTE+61 // highest
#define TRIGGER_PIN 5 // default GATE pin on NS1nanosynth.
#define NOTES_BUFFER 127 //size of notes buffer

//midi message constants
const byte NOTEON = 0x09;
const byte NOTEOFF = 0x08;
const byte CC = 0x0B;
const byte PB = 0x0E;

byte MIDI_CHANNEL = 0; // Initial MIDI channel (0=1, 1=2, etc...), can be adjusted with notes 12-23

unsigned short noteIndex = 0; //index into notes buffer
int notes[NOTES_BUFFER]; //notes buffer - list of played but unprocessed notes
int noteNeeded=0;
float currentNote=0;
byte analogVal = 0;
float glide=0;
int mod=0;
float currentMod=0;
int bend=0;

//lookup table converting midi notes into 5 octaves (volts) digital values
//first value corresponds to 0v; last value corresponds to 5V.
int DacVal[] = {0, 68, 137, 205, 273, 341, 410, 478, 546, 614, 683, 751, 819, 887, 956, 1024, 1092, 1160, 1229, 1297, 1365, 1433, 1502, 1570, 1638, 1706, 1775, 1843, 1911, 1979, 2048, 2116, 2184, 2252, 2321, 2389, 2457, 2525, 2594, 2662, 2730, 2798, 2867, 2935, 3003, 3071, 3140, 3208, 3276, 3344, 3413, 3481, 3549, 3617, 3686, 3754, 3822, 3890, 3959, 4027, 4095};

//values output to dac
unsigned short DacOutA=0;
unsigned short DacOutB=0;

byte addresses[4] = { 0x00, 0x10, 0x60, 0x70 }; //digipot address
byte digipot_addr= 0x2C;  //i2c bus digipot IC addr

//set these values to non-zero within interrupt to update digipots next time thru the main loop
byte isPot0Ready=0;
byte isPot1Ready=0;
byte isPot2Ready=0;
byte isPot3Ready=0;

//these contain the values to be written to the digipots
byte pot0Value=0;
byte pot1Value=0;
byte pot2Value=0;
byte pot3Value=0;

//how often does the check function run?
#define CHECK_RATE_MICROS 500

//sources are either sequences or LFOs.
//how many sources are there? (one per digipot)
#define SOURCE_COUNT 4

//what's the maximum length of a sequence?
#define MAX_SEQ_LENGTH 16

//how many steps does each sequence have?
#define SEQ_1_LENGTH 16
#define SEQ_2_LENGTH 8
#define SEQ_3_LENGTH 8
#define SEQ_4_LENGTH 5

//each value in the sequence is a number from 0 to..what? (this value - 1)
#define SEQ_VALUE_RANGE 256

//what is the "speed" of each sequence? large numbers are slower
#define SEQ_1_INIT_DIVISOR 400
#define SEQ_2_INIT_DIVISOR 400
#define SEQ_3_INIT_DIVISOR 800
#define SEQ_4_INIT_DIVISOR 400

//how much should each button press adjust the "speed" of each sequence?
#define SEQ_1_DIVISOR_STEP 10
#define SEQ_2_DIVISOR_STEP 10
#define SEQ_3_DIVISOR_STEP 20
#define SEQ_4_DIVISOR_STEP 10

//what is the "speed" of each LFO? large numbers are slower
#define LFO_1_INIT_DIVISOR 50
#define LFO_2_INIT_DIVISOR 100
#define LFO_3_INIT_DIVISOR 200
#define LFO_4_INIT_DIVISOR 400

//how much should each button press adjust the "speed" of each LFO?
#define LFO_1_DIVISOR_STEP 4
#define LFO_2_DIVISOR_STEP 8
#define LFO_3_DIVISOR_STEP 12
#define LFO_4_DIVISOR_STEP 16

//available behaviors of each digipot
#define SEQ_MODE 0
#define GATE_MODE 1
#define LFO_MODE 2

//each value in the lfo is a number from 0 to..what? (this value - 1)
#define LFO_VALUE_RANGE 256

//defines a perfect square wave 
#define SQUARE_PULSE_WIDTH 127

//current direction of each LFO sweep
#define LFO_UP 0
#define LFO_DOWN 1

//how many lfo shapes are there
#define LFO_SHAPE_COUNT 3

//these are the available lfo shapes
#define LFO_SHAPE_TRI 0
#define LFO_SHAPE_SAW_UP 1
#define LFO_SHAPE_SAW_DOWN 2

// pin numbers

#define UNUSED_PIN 2 //used to seed random number generator

#define STATUS_LED_PIN 6

#define R_SELECTION_LED_PIN A5
#define G_SELECTION_LED_PIN A4
#define B_SELECTION_LED_PIN A3

#define ACTION_BUTTON_PIN 7
#define UP_BUTTON_PIN 9
#define DOWN_BUTTON_PIN 10
#define SELECT_BUTTON_PIN 11
#define MODE_BUTTON_PIN 12

//index in buttons array
#define ACTION_BUTTON_INDEX 0
#define UP_BUTTON_INDEX 1
#define DOWN_BUTTON_INDEX 2
#define MODE_BUTTON_INDEX 3
#define SELECT_BUTTON_INDEX 4

#define GATE_PULSE_STEP 16 
#define GATE_VALUE_RANGE 256

// array of sequences of numbers corresponding to voltages
int sequences[SOURCE_COUNT][MAX_SEQ_LENGTH];
// array of effective lengths of each sequence
int sequenceLengths[SOURCE_COUNT];
// current index of each sequence - which step is each sequence at?
int sequenceIndices[SOURCE_COUNT];
// "speed" of each sequence
int sequenceDivisors[SOURCE_COUNT];
// rate of change of each sequence's speed
int sequenceDivisorSteps[SOURCE_COUNT];
// when these values get to 0, the corresponding sequence increments its step
int sequenceCountdowns[SOURCE_COUNT];

//set sourceModes[i] to LFO_MODE to change that sequence to behave as an LFO instead
int sourceModes[SOURCE_COUNT];

//indicates index of sequence being edited
int selectedSource = 0;

//contains current value of lfo
byte lfoCurrentValues[SOURCE_COUNT];
//contains current direction of lfo travel
int lfoCurrentDirections[SOURCE_COUNT];
//controls "speed" of LFO (larger = slower)
int lfoDivisors[SOURCE_COUNT];
//rate of change of each LFO's speed
int lfoDivisorSteps[SOURCE_COUNT];
//timer ticks until next change in LFO value
int lfoCountdowns[SOURCE_COUNT];
//shape of each lfo - triangle, saw up, saw down
int lfoShapes[SOURCE_COUNT];
// length of gate pulses - 0 and 255 are vanishingly small and large, 127 is 50% duty cycle
byte gateLengths[SOURCE_COUNT];
// last emitted value for each gate
bool gateCurrentStates[SOURCE_COUNT];

#define DEBOUNCE 10  // button debouncer, how many ms to debounce, 5+ ms is usually plenty

// list of pins attached to buttons
byte buttons[] = {ACTION_BUTTON_PIN, UP_BUTTON_PIN, DOWN_BUTTON_PIN, MODE_BUTTON_PIN, SELECT_BUTTON_PIN};

//set the index to 1 if the corresponding button needs to be inverted and the pullup needs to be enabled
byte pullUpAndInvert[] = {0, 0, 0, 1, 1};

#define BUTTONS_COUNT sizeof(buttons)

// we will track if a button is just pressed, just released, or 'currently pressed'
byte pressed[BUTTONS_COUNT], justPressed[BUTTONS_COUNT], justReleased[BUTTONS_COUNT];

void setup() {
  
  pinMode(TRIGGER_PIN, OUTPUT); // set GATE pin to output mode
  analogWrite(TRIGGER_PIN, LOW);  //GATE down

  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(R_SELECTION_LED_PIN, OUTPUT);
  pinMode(G_SELECTION_LED_PIN, OUTPUT);
  pinMode(B_SELECTION_LED_PIN, OUTPUT);
  
  changeSelectionLed();
  
  for (int i=0; i < BUTTONS_COUNT; i++)
  {
    int pinState = pullUpAndInvert[i] ? INPUT_PULLUP : INPUT;
    pinMode(buttons[i], pinState);
  }
  
  randomSeed(analogRead(2));
  initSources();  
  dac.setGain(2);
  Wire.begin();
  Timer1.initialize(CHECK_RATE_MICROS);   //microseconds
  Timer1.attachInterrupt(timerTick);
  
} //setup

void loop() {
  checkButtons();
  actOnButtons();
  resetButtons();
  writeToPots();
} //loop

void initSources() {
  
  sequenceLengths[0] = SEQ_1_LENGTH;
  sequenceLengths[1] = SEQ_2_LENGTH;
  sequenceLengths[2] = SEQ_3_LENGTH;
  sequenceLengths[3] = SEQ_4_LENGTH;
  
  sequenceDivisors[0] = SEQ_1_INIT_DIVISOR;
  sequenceDivisors[1] = SEQ_2_INIT_DIVISOR;
  sequenceDivisors[2] = SEQ_3_INIT_DIVISOR;
  sequenceDivisors[3] = SEQ_4_INIT_DIVISOR;
  
  sequenceDivisorSteps[0] = SEQ_1_DIVISOR_STEP;
  sequenceDivisorSteps[1] = SEQ_2_DIVISOR_STEP;
  sequenceDivisorSteps[2] = SEQ_3_DIVISOR_STEP;
  sequenceDivisorSteps[3] = SEQ_4_DIVISOR_STEP;
  
  sourceModes[0] = SEQ_MODE;
  sourceModes[1] = SEQ_MODE;
  sourceModes[2] = SEQ_MODE;
  sourceModes[3] = SEQ_MODE;
  
  lfoDivisors[0] = LFO_1_INIT_DIVISOR;
  lfoDivisors[1] = LFO_2_INIT_DIVISOR;
  lfoDivisors[2] = LFO_3_INIT_DIVISOR;
  lfoDivisors[3] = LFO_4_INIT_DIVISOR;

  lfoDivisorSteps[0] = LFO_1_DIVISOR_STEP; 
  lfoDivisorSteps[1] = LFO_2_DIVISOR_STEP;
  lfoDivisorSteps[2] = LFO_3_DIVISOR_STEP;
  lfoDivisorSteps[3] = LFO_4_DIVISOR_STEP;  
  
  for (int i = 0; i < SOURCE_COUNT; i++) {
    gateLengths[i] = SQUARE_PULSE_WIDTH;
    gateCurrentStates[i] = false;
    sequenceIndices[i] = 0;
    initLfo(i);
    lfoCountdowns[i] = lfoDivisors[i];
    lfoShapes[i] = LFO_SHAPE_TRI;
    sequenceCountdowns[i] = sequenceDivisors[i];
    setSequence(i);
  } //for
  
} //initSources

void initLfo(int seqToSet) {
  
  lfoCurrentDirections[seqToSet] = 0;
  lfoCurrentValues[seqToSet] = 0;
  
} //initLfo

void initGate(int gateToSet) {
  gateLengths[gateToSet] = SQUARE_PULSE_WIDTH;
}

void setSequence(int seqToSet) {
  
  for (int i = 0; i< sequenceLengths[seqToSet]; i++) {
    
    if (i > 0) { Serial.print(","); }
    sequences[seqToSet][i] = random(SEQ_VALUE_RANGE);
    Serial.print(sequences[seqToSet][i]);
    
  } //for
  
  Serial.println("\n");
} //setSequence

byte getPrevSourceNumber(int currSourceNumber)
{
  int linkedSource = currSourceNumber - 1;
  if (linkedSource == -1) { linkedSource = SOURCE_COUNT - 1; }
  return (byte) linkedSource;
}

byte getNextSourceNumber(int currSourceNumber)
{
  int linkedSource = currSourceNumber + 1;
  if (linkedSource == SOURCE_COUNT) { linkedSource = 0; }
  return (byte) linkedSource;
}

void actOnButtons() {
  
  if (justPressed[MODE_BUTTON_INDEX]) {
    Serial.println("change mode");
    
    if (sourceModes[selectedSource] == SEQ_MODE) {
      //only switch to GATE_MODE if preceding mode is a sequence
      byte linkedSource = getPrevSourceNumber(selectedSource);
      if (sourceModes[linkedSource] == SEQ_MODE)
      { //gate mode is only valid if the preceding source is set to sequence mode
        sourceModes[selectedSource] = GATE_MODE;
        initGate(selectedSource);
      } else { //if that's not the case, then skip to the next mode after gate mode.
        sourceModes[selectedSource] = LFO_MODE;
        initLfo(selectedSource);
      }

      //check if next sequence is in gate mode - if it is, reset it to sequencer mode
      linkedSource = getNextSourceNumber(selectedSource);

      if (sourceModes[linkedSource] == GATE_MODE)
      {
        sourceModes[linkedSource] = SEQ_MODE;
        setSequence(linkedSource);
      }

    } else if (sourceModes[selectedSource] == GATE_MODE) {
      sourceModes[selectedSource] = LFO_MODE;
      initLfo(selectedSource);
    } else {
      sourceModes[selectedSource] = SEQ_MODE;
      setSequence(selectedSource);
    }
  }//if pressed mode button
  
  if (justPressed[SELECT_BUTTON_INDEX]) {
    
    selectedSource = (selectedSource + 1) % SOURCE_COUNT;
    Serial.print("select seq ");
    Serial.println(selectedSource);
    changeSelectionLed();
  } //if pressed select button
  
  if (justPressed[ACTION_BUTTON_INDEX]) {
    if (sourceModes[selectedSource] == SEQ_MODE) {
      Serial.print("rand seq ");
      Serial.println(selectedSource);
      setSequence(selectedSource);
    } else if (sourceModes[selectedSource] == GATE_MODE) 
    {
      Serial.println("linked gate");
      initGate(selectedSource);
    } else { 
      lfoShapes[selectedSource] = (lfoShapes[selectedSource] + 1) % LFO_SHAPE_COUNT;
      Serial.print("lfo ");
      Serial.print(selectedSource);
      Serial.print(" shape ");
      Serial.println(lfoShapes[selectedSource]);
      initLfo(selectedSource);
    } // if sourceModes
  } //if justPressed action button
  
  if (justPressed[UP_BUTTON_INDEX])
  {
    Serial.println("up");
    if (sourceModes[selectedSource] == SEQ_MODE) {
      sequenceDivisors[selectedSource] += sequenceDivisorSteps[selectedSource];
    } else if (sourceModes[selectedSource] == GATE_MODE) {
      int tempVal;
      tempVal = gateLengths[selectedSource] +=  GATE_PULSE_STEP;
      if (tempVal >= GATE_VALUE_RANGE)  { tempVal = GATE_VALUE_RANGE -1; }
      gateLengths[selectedSource] = (byte) tempVal;
    } else if (sourceModes[selectedSource] == LFO_MODE) {
      lfoDivisors[selectedSource] += lfoDivisorSteps[selectedSource];
    }
  } //if just pressed up button
  
  if (justPressed[DOWN_BUTTON_INDEX])
  {
    Serial.println("down");
    if (sourceModes[selectedSource] == SEQ_MODE) {
      sequenceDivisors[selectedSource] -= sequenceDivisorSteps[selectedSource];
      if (sequenceDivisors[selectedSource] < 1) sequenceDivisors[selectedSource] = 1;
    } else if (sourceModes[selectedSource] == GATE_MODE) {
      int tempVal;
      tempVal = gateLengths[selectedSource] -=  GATE_PULSE_STEP;
      if (tempVal < 0) { tempVal = 0; }  
      gateLengths[selectedSource] = (byte) tempVal;
    } else if (sourceModes[selectedSource] == LFO_MODE) {
      lfoDivisors[selectedSource] -= lfoDivisorSteps[selectedSource];
      if (lfoDivisors[selectedSource] < 1) lfoDivisors[selectedSource] = 1;
      Serial.println(lfoDivisors[selectedSource]);
    }
  } //if just pressed down button
} //actOnButtons

void runLfos() {
  for (int i = 0; i < SOURCE_COUNT; i++) {
    // do this for each source
    
    if (sourceModes[i] == LFO_MODE) {
      // this is an lfo
      
      if (lfoCountdowns[i] <= 0) { 
        //ready to increment/decrement lfo
        
        if(lfoShapes[i] == LFO_SHAPE_TRI) {
          // is lfo at its extent? if so, reverse direction
          
          if ((lfoCurrentValues[i] >= (LFO_VALUE_RANGE - 1)) && lfoCurrentDirections[i] == LFO_UP)
          { //lfo is at top - send it back down
            lfoCurrentDirections[i] = LFO_DOWN;
          }
          if (lfoCurrentValues[i] <= 0 && lfoCurrentDirections[i] == LFO_DOWN)
          { //lfo is at bottom - send it back up
            lfoCurrentDirections[i] = LFO_UP;
          }
          
          int delta = (lfoCurrentDirections[i] == LFO_UP) ? 1 : -1;
          lfoCurrentValues[i] += delta;
        } // if lfo == tri
        
        if (lfoShapes[i] == LFO_SHAPE_SAW_UP)
        {
          if (lfoCurrentValues[i] >= (LFO_VALUE_RANGE - 1)) { lfoCurrentValues[i] = 0; } 
          else { lfoCurrentValues[i]++; } 
        } //if lfo == saw up
        
        if (lfoShapes[i] == LFO_SHAPE_SAW_DOWN)
        {
          if (lfoCurrentValues[i] <= 0) { lfoCurrentValues[i] = LFO_VALUE_RANGE - 1; }
          else { lfoCurrentValues[i]--; }
        } //if lfo = saw down

        preparePotValue(i, lfoCurrentValues[i]);
        if (i == selectedSource) { changeStatusLed(lfoCurrentValues[i]); } 

        //reset lfo timer
        lfoCountdowns[i] = lfoDivisors[i];
      } //if countdown hits 0
      
      //finally, tick
      lfoCountdowns[i]--;
      
    } //if lfoCountdowns

  }//if lfo
}// runLfos


void stepSequencers() {
  
  for (int i = 0; i < SOURCE_COUNT; i++) {
    // do this for each source
   
    if (sourceModes[i] == SEQ_MODE) {
      //this is a sequence
        byte nextSource = getNextSourceNumber(i); 
      
      if (sourceModes[nextSource] == GATE_MODE)
      { // there is a gate source linked to this sequence (the next one). deal with it.
                
        //map(value, fromLow, fromHigh, toLow, toHigh)
        int progression = sequenceDivisors[i] - sequenceCountdowns[i];

     //   Serial.print("linked countdown: ");
     //   Serial.println(progression);
        //we have gate lengths as a value between 0 and 255
        //convert the current countdown progress to the same scale
        int relativeProgression = map(progression, 0, sequenceDivisors[i], 0, GATE_VALUE_RANGE-1);

     //   Serial.print("gate#: ");
     //   Serial.print(nextSource);
     //   Serial.print(":::");
     //   Serial.println(relativeProgression);
     
        //relativeProgression is a value between 0 and 255 that represents how far into this "beat" we are.
        //if it's less than the gate length, we should emit HIGH on that pin, otherwise LOW.
        //but look at the current value first and see if it changed - don't write the pin every loop.
        bool newState = (relativeProgression <= gateLengths[nextSource]);

        if (gateCurrentStates[nextSource] != newState)
        { // the state of the linked gate has changed
           gateCurrentStates[nextSource] = !gateCurrentStates[nextSource];
           
            Serial.println(gateCurrentStates[nextSource]);

           byte valToWrite = gateCurrentStates[nextSource] ? GATE_VALUE_RANGE-1 : 0;
           //update the digipot
           preparePotValue(nextSource, valToWrite );

           if (nextSource == selectedSource) { changeStatusLed(valToWrite); } 
        }
      }
      
      if (sequenceCountdowns[i] <= 0)
      { // it's time to move the current sequence one step forward
        sequenceIndices[i] = (sequenceIndices[i] + 1) % sequenceLengths[i];
       
        preparePotValue(i, sequences[i][sequenceIndices[i]]);
        if (i == selectedSource) { changeStatusLed(sequences[i][sequenceIndices[i]]); } 
        //restart the countdown for this sequence
        sequenceCountdowns[i] = sequenceDivisors[i];
      } //if end of countdown
      
      //finally, tick
      sequenceCountdowns[i]--;
      
    } //if seq_mode
  } //for
}

void timerTick(){
  stepSequencers();
  runLfos();
  processMidi();
}

void writeSelectionLedPins(int r, int g, int b) {
  digitalWrite(R_SELECTION_LED_PIN, r);
  digitalWrite(G_SELECTION_LED_PIN, g);
  digitalWrite(B_SELECTION_LED_PIN, b);
}

void changeSelectionLed() {
  switch(selectedSource) {
    case 0:
      writeSelectionLedPins(1,0,0);
    break;
    case 1:
      writeSelectionLedPins(0,1,0);
    break;
    case 2:
      writeSelectionLedPins(0,0,1);
    break;
    case 3:
      writeSelectionLedPins(1,0,1);
    break;
    default:
      writeSelectionLedPins(0,0,0);
    break;
  }
}

void changeStatusLed(byte toWrite)
{
  //Serial.print("status: ");
 // Serial.print(toWrite);
  analogWrite(STATUS_LED_PIN, toWrite);
}

