
void playNote(byte noteVal, float myMod) {
  
  DacOutA = DacVal[noteVal-MIN_NOTE];
  
  if (bend != 0) { DacOutA = DacOutA + bend; }
  
  dac.outputA(DacOutA);
  analogWrite(TRIGGER_PIN, 255); //GATE ON
}

void addNote(byte note) {

  boolean found = false;
  // a note was just played
  
  // see if it was already being played
  if (noteIndex > 0){
    for (int i = noteIndex; i>0; i--) {
      if (notes[i] == note){
        // this note is already being played
        found = true;
        
        // step forward through the remaining notes, shifting each backward one
        for (int j=i; j<noteIndex; j++) {
          notes[j] = notes[j+1];
        }
        
        // set the last note in the buffer to this note
        notes[noteIndex]=note;
        
        // done adding note
        break;
      }
    }
  }
  
  if (found == false) {
    // the note wasn't already being played, add it to the buffer
    noteIndex = (noteIndex+1) % NOTES_BUFFER;
    notes[noteIndex] = note;
  }
  
  noteNeeded = note;
  
}

void removeNote(byte note) {

  boolean complete=false;
  
  // handle most likely scenario
  if (noteIndex==1 && notes[1]==note){
    // only one note played, and it was this note
    noteIndex=0;
    currentNote=0;
    
    // turn gate off
    analogWrite(TRIGGER_PIN, 0);
    
  } else {
    // a note was just released, but it was one of many
    for (int i = noteIndex; i>0; i--) {
      if (notes[i] == note) {
        // this is the note that was being played, was it the last note?
        if (i == noteIndex) {
          // this was the last note that was being played, remove it from the buffer
          notes[i] = 0;
          noteIndex = noteIndex-1;
          
          // see if there is another note still being held
          if (i > 1) {
            // there are other that are still being held, sound the most recently played one
            addNote(notes[i-1]);
            complete = true;
          }
        }
        else {
          // this was not the last note that was being played, just remove it from the buffer and shift all other notes
          for (int j=i; j<noteIndex; j++) {
            notes[j] = notes[j+1];
          }
          
          // set the last note in the buffer to this note
          notes[noteIndex] = note;
          noteIndex = noteIndex - 1;
          complete = true;
        }
        
        if (complete == false){
          
          // make sure noteIndex is cleared back to zero
          noteIndex = 0;
          currentNote = 0;
          noteNeeded = 0;
          break;
        }
        
        // finished processing the release of the note that was released, just quit
        break;
       
      }
    }
  }
}


void processMidi()
{  
  while(MIDIUSB.available() > 0) {
    // Repeat while notes are available to read.
    MIDIEvent e;
    e = MIDIUSB.read();
    if(e.type == NOTEON) {
      if(e.m1 == (0x90 + MIDI_CHANNEL)){
        if(e.m2 >= MIN_NOTE && e.m2 <= MAX_NOTE){
          if(e.m3==0)         //Note in the right range, if velocity=0, remove note
          removeNote(e.m2);
          else                //Note in right range and velocity>0, add note
            addNote(e.m2);
        } else if (e.m2 < MIN_NOTE) {
          //out of lower range hook
        } else if (e.m2 > MAX_NOTE) {
          //out of upper range hook
        }
      }
    }
    
    if(e.type == NOTEOFF) {
      if(e.m1 == 0x80 + MIDI_CHANNEL){
        removeNote(e.m2);
      }
    }
    
    // set modulation wheel
    if (e.type == CC && e.m2 == 1)
    {
      if (e.m3 <= 3)
      {
        // set mod to zero
        mod=0;
        dac.outputB(0);
      }
      else
      {
        mod=e.m3;
        DacOutB=mod*32;
        dac.outputB(DacOutB);
      }
    }
    
    // set pitch bend
    if (e.type == PB){
      if(e.m1 == (0xE0 + MIDI_CHANNEL)){
        // map bend somewhere between -127 and 127, depending on pitch wheel
        // allow for a slight amount of slack in the middle (63-65)
        // with the following mapping pitch bend is +/- two semitones
        if (e.m3 > 65){
          bend=map(e.m3, 64, 127, 0, 136);
        } else if (e.m3 < 63){
          bend=map(e.m3, 0, 64, -136, 0);
        } else {
          bend=0;
        }
        
        if (currentNote>0){
          playNote (currentNote, 0);
        }
      }
    }
    
    MIDIUSB.flush();
  }
  
  if (noteNeeded>0){
    // on our way to another note
    if (currentNote==0){
      // play the note, no glide needed
      playNote (noteNeeded, 0);
      
      // set last note and current note, clear out noteNeeded becase we are there
      currentNote=noteNeeded;
      noteNeeded=0;
    } else {
      if (glide>0){
        // glide is needed on our way to the note
        if (noteNeeded>int(currentNote)) {
          currentNote=currentNote+glide;
          if (int(currentNote)>noteNeeded) currentNote=noteNeeded;
        } else if (noteNeeded<int(currentNote)) {
          currentNote=currentNote-glide;
          if (int(currentNote)<noteNeeded) currentNote=noteNeeded;
        } else {
          currentNote=noteNeeded;
        }
      } else {
        currentNote=noteNeeded;
      }
      playNote (int(currentNote), 0);
      if (int(currentNote)==noteNeeded){
        noteNeeded=0;
      }
    }
  } else {
    if (currentNote>0){
    }
  }
}
