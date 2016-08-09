//MIDI specs
#define MIDI_CHANNEL 0x09 // can be customized from 0x00 to 0x0F, ox09 = Channel 10 = percussion in General Midi

#define MIDI_NOTE_ON 0x90 | MIDI_CHANNEL
#define MIDI_NOTE_OFF 0x80  | MIDI_CHANNEL
#define MIDI_SERIAL_RATE 31250
#define MIDI_MAX_VELOCITY 127

// Number of sensors
#define NOTES 6
#define DEBOUNCING_TIME 160 // in milliseconds

unsigned short notes[NOTES];

//Sensitivity range for velocity
boolean noteOn[NOTES];
boolean noteSent[NOTES];
boolean calibrationFinished;
boolean sending;

unsigned short noteVelocity[NOTES];  // from analog input in range 0..1023
unsigned short noisegate[NOTES];
unsigned short peak[NOTES];

unsigned long powerOnTime; // Used to enable calibration mode
unsigned long lastNoteTime[NOTES];


void setup()
{
  Serial.begin(MIDI_SERIAL_RATE);

  powerOnTime = millis();
  calibrationFinished = false;

  for (short note = 0; note < NOTES; ++note)
  {
    noteOn[note] = false;
    noteSent[note] = true;
    noteVelocity[note] = 0;
    lastNoteTime[note] = 0;
  }

  // Note assignments
  // see https://en.wikipedia.org/wiki/General_MIDI#Percussion
  notes[0] = 35; // Bass Drum 1
  notes[1] = 42; // Low Tom 1
  notes[2] = 50; // Ride Cymbal 1
  notes[3] = 48; // Crash Cymbal 1
  notes[4] = 37; // Snare Drum 1
  notes[5] = 49; // High Tom 1
}

void loop()
{
  unsigned long currentTime = millis();
  boolean calibrationMode = (currentTime-powerOnTime) < 2000; // First 2 seconds at power on: Don't hit the pads!

  for (short note = 0; note < NOTES; ++note)
  {
    delay(1);
    unsigned short velocity = analogRead(note);

    if(peak[note]<velocity) peak[note]=velocity;

    if(calibrationMode)
    {
      if(noisegate[note] < (1.5 * velocity)){
        noisegate[note] = (1.5 * velocity);
      }
    }
    else
    {
      if(!calibrationFinished)
      {
        beep();
        calibrationFinished = true;
      }
      else if (velocity > noisegate[note])
      {
        if (noteOn[note])
        {
          if (velocity > noteVelocity[note])
          {
            noteVelocity[note] = velocity;
          }
          else {
            // Peak sample reached
            sendNote(note);
            noteOn[note] = false;
            noteVelocity[note] = 0;
          }
        }
        else if ((currentTime - lastNoteTime[note]) > DEBOUNCING_TIME)
        {
          noteOn[note] = true;
          noteVelocity[note] = velocity;
          noteSent[note]=false;
          lastNoteTime[note] = currentTime;
        }
      }
      else
      {
        if(noteOn[note])
        {
          if(noteSent[note]==false)
          {
            // Single sample above noisegate
            sendNote(note);
          }
          noteOn[note] = false;
          noteVelocity[note] = 0;
        }
      }
    }
  }
}

void sendNote(short note)
{
    Serial.write(MIDI_NOTE_ON);
    Serial.write(notes[note]);
    Serial.write(map(noteVelocity[note], noisegate[note], peak[note], 0, MIDI_MAX_VELOCITY));
    //Serial.write(MIDI_NOTE_OFF);
    //Serial.write(notes[note]);
    //Serial.write(1);
    noteSent[note] = true; 
}

void beep()
{
   // Usse piezo sensor as buzzer
   pinMode(A1, OUTPUT); 
   tone(A1, 220, 333);
   delay(333);
   tone(A1, 440, 333);
   delay(333);
   tone(A1, 880, 333);
   delay(333);
   pinMode(A1, INPUT); // disable pullup
}

