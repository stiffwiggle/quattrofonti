
void resetButtons() {
  for (int i=0; i<BUTTONS_COUNT; i++) {
    justPressed[i] = false;
    justReleased[i] = false;
  }
}

void checkButtons() {
  
  static byte previousState[BUTTONS_COUNT];
  static byte currentState[BUTTONS_COUNT];
  static unsigned long lastTime;
  byte index;

  // keep waiting if not enough time has passed to debounce
  if ((lastTime + DEBOUNCE) > millis()) { return; }
  
  // ok we have waited DEBOUNCE milliseconds, let's reset the timer
  lastTime = millis();
  
  for (index = 0; index < BUTTONS_COUNT; index++)
  {
    // when we start, we clear out the "just" indicators
    justReleased[index] = 0; //this was in the original code...
    justPressed[index] = 0; //this wasn't...
    
    currentState[index] = digitalRead(buttons[index]);   // read the button
    
    if (!pullUpAndInvert[index]){ currentState[index] = !currentState[index]; }
    
    if (currentState[index] == previousState[index]) {
      //check for press
      if ((pressed[index] == LOW) && (currentState[index] == LOW)) { justPressed[index] = 1; }
      //check for release
      else if ((pressed[index] == HIGH) && (currentState[index] == HIGH)) { justReleased[index] = 1; }
      // digital HIGH means NOT pressed so invert before saving to pressed list
      pressed[index] = !currentState[index];  
    } //if current = prev
    
    previousState[index] = currentState[index];
  }
}
