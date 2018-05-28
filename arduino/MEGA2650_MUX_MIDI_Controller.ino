// MIDI Controller, more than inspired by tttapa http://www.instructables.com/id/Custom-Arduino-MIDI-Controller
// Code for the Arduino MEGA 2560, Mux Shield II, and PSYCHOTRONv1.1

#include <MuxShield.h>
#include <MIDI.h>

MuxShield mux;                          // initialize Mux Shield
#define IO1mode DIGITAL_OUT             // initialize the Mux Shield rows mode
#define IO2mode DIGITAL_OUT             // change the values here and it should work fine
#define IO3mode ANALOG_IN

MIDI_CREATE_DEFAULT_INSTANCE(); // initialize MIDI
//struct MySettings : public midi::DefaultSettings {
  //static const long BaudRate = 115200;
//  static const long BaudRate = 93750;
//};
//MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings); // tentative d'avoir plus de flux.


#define RESOLUTION 64           // defines the precison of the analog inputs (min 2, max 128). may increase stability

// number of i/o constants
#define N_ANALOG_I 9            // max 13 inputs (usable pins on the MEGA 2560 once the Mux Shield is plugged)
                                // pin 0, 1, 2 are used by the Mux Shield

#define N_DIGITAL_I 26          // max 43 inputs or outputs (usable pins on the MEGA 2560 once the Mux Shield is plugged
#define N_DIGITAL_O 10          // pin 0 and 1 are needed for the serial communication
                                // pin 2, 4, 6, 7, 8, 10, 11, 12 are used by the Mux Shield
                                // pin 13 for the status LED

#define N_AD_IO_MUX1 16         // max 16, number of i/o for 1st row on the Mux Shield
#define N_AD_IO_MUX2 16         // max 16, number of i/o for 2nd row on the Mux Shield
#define N_AD_IO_MUX3 16         // max 16, number of i/o for 3rd row on the Mux Shield


#define CHANNEL_I 1             // send all messages on channel 1
#define CHANNEL_O 2             // receive all messages on channel 2

#define DELAY 10                // delay between loops, to prevent overflowing the computer with MIDI messages

#define MAX_TEMPO 10000         // in ms, maximum delay between two flashes
#define MIN_TEMPO 100           // in ms, minimum delay between two flashes

#define START_NOTE 127
#define KEEP_ACTIVE 2000
#define IS_OFF 0
#define HAS_STARTED 1
#define IS_ON 2
#define HAS_STOPPED 3
int patch_status = IS_OFF;
int last_beep = 0;
int two_beeps = 0;

// pins 3, 4, 5 are on the shield. Pins 8, 9, 10 are under the shield and hard to reach
int ARDanalogs[9] = {A7,A8,A9,A10,A11,A12,A13,A14,A15};

// pins 3, 5, 9 are on the shield. Pins 14, 15, 16, 17, 18 are under the shield and hard to reach
int ARDdigitalsI[26] = {22,23,24,25,26,27,
                        28,29,30,31,32,33,34,35,36,37,
                        38,39,40,41,42,43,44,45,46,47};
                       
int ARDdigitalsO[10] = {3,5,9,13,48,49,50,51,52,53};

// controllers arrays
// more on http://midi.org/techspecs/midimessages.php#3
int ARDcontrols[13];
int IO1controls[16];
int IO2controls[16];
int IO3controls[16];

// notes arrays
// more on http://www.electronics.dit.ie/staff/tscarff/Music_technology/midi/midi_note_numbers_for_octaves.htm
int ARDnotes[43];
int IO1notes[16];
int IO2notes[16];
int IO3notes[16];

// notes detection arrays
int ARDnotes2[43];
int IO1notes2[16];
int IO2notes2[16];
int IO3notes2[16];

int analogIVal[N_ANALOG_I];   // We declare an array for the values from the analog inputs
int analogIOld[N_ANALOG_I];   // We declare an array for the previous analog values.

int digitalIVal[N_DIGITAL_I]; // We declare an array for the values from the digital inputs.
int digitalIOld[N_DIGITAL_I]; // We declare an array for the previous digital values.

int digitalOVal[N_DIGITAL_O]; // We declare an array for the values from the digital outputs.
int digitalOOld[N_DIGITAL_O]; // We declare an array for the previous digital values.

int ADIO1Val[N_AD_IO_MUX1];  // We declare an array for the values from the 1st row of the Mux Shield analog or digital inputs or outputs
int ADIO1Old[N_AD_IO_MUX1];  // We declare an array for the Mux Shield previous analog values.

int ADIO2Val[N_AD_IO_MUX2];  // We declare an array for the values from the 2nd row of the Mux Shield analog or digital inputs or outputs
int ADIO2Old[N_AD_IO_MUX2];  // We declare an array for the Mux Shield previous analog values.

int ADIO3Val[N_AD_IO_MUX3];  // We declare an array for the values from the 3rd row of the Mux Shield analog or digital inputs or outputs
int ADIO3Old[N_AD_IO_MUX3];  // We declare an array for the Mux Shield previous analog values.

void initControls() { // initialize the controls in order for their numbers to be continuous, whatever the configuration
  int n_assigned = 0;

  for(int i = 0; i < N_ANALOG_I; i++) {
    ARDcontrols[i] = (byte) n_assigned;
    n_assigned+=1;
  }
  if(IO1mode == ANALOG_IN) {
    for(int i = 0; i < N_AD_IO_MUX1; i++) {
      IO1controls[i] = (byte) (n_assigned);
      n_assigned+=1;
    }
  }
  if(IO2mode == ANALOG_IN) {
    for(int i = 0; i < N_AD_IO_MUX2; i++) {
      IO2controls[i] = (byte) (n_assigned);
      n_assigned+=1;
    }
  }
  if(IO3mode == ANALOG_IN) {
    for(int i = 0; i < N_AD_IO_MUX3; i++) {
      IO3controls[i] = (byte) (n_assigned);
      n_assigned+=1;
    }
  }
}

void initSentNotes() {
  int n_assigned = 0;

  for(int i = 0; i < N_DIGITAL_I; i++) {
    ARDnotes[i] = (byte) n_assigned;
    n_assigned+=1;
  }
  if(IO1mode == DIGITAL_IN_PULLUP) {
    for(int i = 0; i < N_AD_IO_MUX1; i++) {
      IO1notes[i] = (byte) (n_assigned);
      n_assigned+=1;
    }
  }
  if(IO2mode == DIGITAL_IN_PULLUP) {
    for(int i = 0; i < N_AD_IO_MUX2; i++) {
      IO2notes[i] = (byte) (n_assigned);
      n_assigned+=1;
    }
  }
  if(IO3mode == DIGITAL_IN_PULLUP) {
    for(int i = 0; i < N_AD_IO_MUX3; i++) {
      IO3notes[i] = (byte) (n_assigned);
      n_assigned+=1;
    }
  }
}

void initReceivedNotes() {
  int n_assigned = 0;

  for(int i = 0; i < N_DIGITAL_O; i++) {
    ARDnotes2[i] = (byte) n_assigned;
    n_assigned+=1;
  }
  if(IO1mode == DIGITAL_OUT) {
    for(int i = 0; i < N_AD_IO_MUX1; i++) {
      IO1notes2[i] = (byte) (n_assigned);
      n_assigned+=1;
    }
  }
  if(IO2mode == DIGITAL_OUT) {
    for(int i = 0; i < N_AD_IO_MUX2; i++) {
      IO2notes2[i] = (byte) (n_assigned);
      n_assigned+=1;
    }
  }
  if(IO3mode == DIGITAL_OUT) {
    for(int i = 0; i < N_AD_IO_MUX3; i++) {
      IO3notes2[i] = (byte) (n_assigned);
      n_assigned+=1;
    }
  }
}

void handleARDanalogsI(int analogs[], int n_io, int values[], int old[], int controls[], bool force) {  
  for(int i = 0; i < n_io; i++) {                                     // repeat this procedure for every analog input.

    values[i] = map(analogRead(analogs[i]),0,1023,0, RESOLUTION-1);   // read the pots and map their values them to the chosen resolution 
    values[i] = map(values[i],0,RESOLUTION-1,0,127);                 // map the values from the chosen resolution to the MIDI resolution (from 0 to 127)
    
    if((values[i] != old[i]) || force){                                     // Only send the value, if it is a different value than last time.
      MIDI.sendControlChange(controls[i],values[i], CHANNEL_I);
      old[i] = values[i];                                        // Put the analog values in the array for old analog values, so we can compare the new values with the previous ones.
    }
  } 
}

void handleARDdigitalsI(int digitals[], int n_io, int values[], int old[], int notes[], bool force) {
  for(int i = 0; i < n_io; i++) {
    values[i] = digitalRead(digitals[i]);                   // read the switch and store the value (1 or 0) in the digitalVal array.
    if((values[i] != old[i]) || force){               
      if(values[i] == 0){                                      // if the i'th switch is pressed:
        MIDI.sendNoteOn(notes[i],127,CHANNEL_I);
      } else {                                                      // if the i'th switch is released:
        MIDI.sendNoteOff(notes[i],0,CHANNEL_I);
      }
      old[i] = values[i];
    }                                          
  }
}

void handleMUX(int row, int mode, int n_io, int values[], int old[], int controls[], int notes[], bool force) {
  switch(mode) {
    case ANALOG_IN:
      for (int i = 0; i < n_io; i++) {
        values[i] = mux.analogReadMS(row,i);
        values[i] = map(values[i],0,1023,0,RESOLUTION-1);
        values[i] = map(values[i],0,RESOLUTION-1,0,127);
  
        if((values[i] != old[i]) || force){
          MIDI.sendControlChange(controls[i],values[i], CHANNEL_I);
          old[i] = values[i];
        }
      }
    break;
    
    case DIGITAL_IN_PULLUP:
      for (int i = 0; i < n_io; i++) {
        values[i] = mux.digitalReadMS(row,i);
        if ((values[i] != old[i]) || force) {
          if (values[i] == 0) {
            MIDI.sendNoteOn(notes[i], 127, CHANNEL_I);
          } else {
            MIDI.sendNoteOff(notes[i], 0, CHANNEL_I);
          }
          old[i] = values[i];
        }
      }
    break;
    
    default:
    break;
  }
}

void handleNoteOn(byte chan, byte pitch, byte velocity) { // define a function to call when a note is received 
  // for "tempoed" led, use a formula to compute the delay between two states
  // TODO implementation
  // float tempo = pow(10, (velocity*(log(MAX_TEMPO)-log(MIN_TEMPO))/127));
  
  if(chan == CHANNEL_O) {
    bool ok = false;

    // Test if a specific noteOn have been heard to stop the "sparkles" - hold on sequence
    // The note must be repeated every other seconds to keep the status on
    if ((int) pitch == START_NOTE) {
      switch (patch_status) {
        case IS_OFF:
          patch_status = HAS_STARTED;
          last_beep = millis();
        break;

        case IS_ON:
          last_beep = millis();
          two_beeps = 0;
        break;
      }
    }
    
    for (int i = 0; i < N_DIGITAL_O; i++) {
      if ((int) pitch == ARDnotes2[i]) {
        digitalWrite(ARDdigitalsO[i], HIGH);
        ok = true;
      }
    }

    if (IO1mode == DIGITAL_OUT && !ok) {
      for (int i = 0; i < N_AD_IO_MUX1; i++) {
        if ((int) pitch == IO1notes2[i]) {
          mux.digitalWriteMS(1, i, HIGH);
          //lightLed(1, i, velocity);
          ok = true;
        }
      }
    }

    if (IO2mode == DIGITAL_OUT && !ok) {
      for (int i = 0; i < N_AD_IO_MUX2; i++) {
        if ((int) pitch == IO2notes2[i]) {
          mux.digitalWriteMS(2, i, HIGH);
          //lightLed(2, i, velocity);
          ok = true;
        }
      }
    }

    if (IO3mode == DIGITAL_OUT && !ok) {
      for (int i = 0; i < N_AD_IO_MUX3; i++) {
        if ((int) pitch == IO3notes2[i]) {
          mux.digitalWriteMS(3, i, HIGH);
          //lightLed(3, i, velocity);
          ok = true;
        }
      }
    }
  }
}

void handleNoteOff(byte chan, byte pitch, byte velocity) {
  if(chan == CHANNEL_O) {
    bool ok = false;
    
    for (int i = 0; i < N_DIGITAL_O; i++) {
      if ((int) pitch == ARDnotes2[i]) {
        digitalWrite(ARDdigitalsO[i], LOW);
        ok = true;
      }
    }

    if (IO1mode == DIGITAL_OUT && !ok) {
      for (int i = 0; i < N_AD_IO_MUX1; i++) {
        if ((int) pitch == IO1notes2[i]) {
          mux.digitalWriteMS(1, i, LOW);
          //lightLed(1, i, velocity);
          ok = true;
        }
      }
    }

    if (IO2mode == DIGITAL_OUT && !ok) {
      for (int i = 0; i < N_AD_IO_MUX2; i++) {
        if ((int) pitch == IO2notes2[i]) {
          mux.digitalWriteMS(2, i, LOW);
          //lightLed(2, i, velocity);
          ok = true;
        }
      }
    }

    if (IO3mode == DIGITAL_OUT && !ok) {
      for (int i = 0; i < N_AD_IO_MUX3; i++) {
        if ((int) pitch == IO3notes2[i]) {
          mux.digitalWriteMS(3, i, LOW);
          //lightLed(3, i, velocity);
          ok = true;
        }
      }
    }
  }
}

void handleControlChange(byte chan, byte number, byte value) {
  if(chan == CHANNEL_O) {
    // TODO (if needed)
  }
}

// Launch a "hold on" sequence until it receives a certain signal
void sparkles() {
  switch (patch_status) {
    case IS_OFF:
      handleNoteOn((byte) CHANNEL_O, (byte) 0, (byte) 127);
//      delay(500);
//      for (int i = 0; i < 50; i++) {
//        handleNoteOff((byte) CHANNEL_O, (byte) i, (byte) 127);
//      }
//      delay(500);
    break;

    case HAS_STARTED:
      handleNoteOff((byte) CHANNEL_O, (byte) 0, (byte) 0);

      handleARDanalogsI(ARDanalogs, N_ANALOG_I, analogIVal, analogIOld, ARDcontrols, true);
      delay(1);
      handleARDdigitalsI(ARDdigitalsI, N_DIGITAL_I, digitalIVal, digitalIOld, ARDnotes, true);
      delay(1);
      handleMUX(1, IO1mode, N_AD_IO_MUX1, ADIO1Val, ADIO2Old, IO1controls, IO1notes, true);
      delay(1);
      handleMUX(2, IO2mode, N_AD_IO_MUX2, ADIO2Val, ADIO2Old, IO2controls, IO2notes, true);
      delay(1);
      handleMUX(3, IO3mode, N_AD_IO_MUX3, ADIO3Val, ADIO2Old, IO3controls, IO3notes, true);
      delay(1);
      patch_status = IS_ON;
    break;

    case IS_ON:
      two_beeps = millis() - last_beep;
      if (two_beeps > KEEP_ACTIVE) {
        handleNoteOn((byte) CHANNEL_O, (byte) 0, (byte) 127);
        delay(500);
        patch_status = HAS_STOPPED;
      }
    break;

    case HAS_STOPPED:
      for (int i = 0; i < 127; i++) {
        handleNoteOff((byte) CHANNEL_O, (byte) i, (byte) 0);
      }
      patch_status = IS_OFF;
    break;
  }
}

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI); // initiate MIDI communications, listen to all channels
  MIDI.setHandleNoteOn(handleNoteOn); // connect the handleNoteOn function to the library
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  
  //Set 1st, 2nd and 3r row of the Mux Shield as analog or digital inputs or outputs
  mux.setMode(1,IO1mode);
  mux.setMode(2,IO2mode);
  mux.setMode(3,IO3mode);

  initControls();
  initSentNotes();
  initReceivedNotes();
  
  //pinMode(13, OUTPUT);   // Set pin 13 (the one with the LED) to output
  //digitalWrite(13, LOW); // Turn off the LED
  
  for(int i = 0; i < N_ANALOG_I; i++) {
  analogIOld[i]=-1;  // We make all values of analogIOld -1, so it will always be different from any possible analog reading.
  }
  
  for(int i = 0; i < N_DIGITAL_I; i++) {
    digitalIOld[i]=-1;
    pinMode(ARDdigitalsI[i], INPUT_PULLUP); // Set all switch-pins as input, and enable the internal pull-up resistor.
  }
  for(int i = 0; i < N_DIGITAL_O; i++) {
    digitalOOld[i]=-1;
    pinMode(ARDdigitalsO[i], OUTPUT); // Set all switch-pins as outputs
  }
  for(int i = 0; i < N_AD_IO_MUX1; i++) {
    ADIO1Old[i]=-1;
  }
  for(int i = 0; i < N_AD_IO_MUX2; i++) {
    ADIO2Old[i]=-1;
  }
  for(int i = 0; i < N_AD_IO_MUX3; i++) {
    ADIO3Old[i]=-1;
  }
  
  delay(2000);  // Wait 2 seconds before sending messages, to be sure everything is set up, and to make uploading new sketches easier.
  
  //digitalWrite(13, HIGH); // Turn on the LED, when the loop is about to start.
}

void loop() {
// ---------- //
// -- START-- //
// ---------- //
  sparkles();

// ---------- //
// - INPUTS - //
// ---------- //
  if(patch_status==IS_ON) {
    handleARDanalogsI(ARDanalogs, N_ANALOG_I, analogIVal, analogIOld, ARDcontrols, false);
    handleARDdigitalsI(ARDdigitalsI, N_DIGITAL_I, digitalIVal, digitalIOld, ARDnotes, false);
    handleMUX(1, IO1mode, N_AD_IO_MUX1, ADIO1Val, ADIO2Old, IO1controls, IO1notes, false);
    handleMUX(2, IO2mode, N_AD_IO_MUX2, ADIO2Val, ADIO2Old, IO2controls, IO2notes, false);
    handleMUX(3, IO3mode, N_AD_IO_MUX3, ADIO3Val, ADIO2Old, IO3controls, IO3notes, false);
  }
  
// ----------- //
// - OUTPUTS - //
// ----------- //
  MIDI.read();
 
  // delay(DELAY); // avoid overflowing the computer with MIDI messages
}
