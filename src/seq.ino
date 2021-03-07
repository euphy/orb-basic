void seq_onBeat() {

  // work out current beat and bar
  unsigned long milliseconds = seq_getSequencerTime();
  int totalBeats = milliseconds/beatInterval;
  int totalTicks = milliseconds/tickInterval;

  bar = totalBeats/beatsPerBar;
  beat = totalBeats - (bar*beatsPerBar)+1;
  tick = totalTicks - (totalBeats*ticksPerBeat)+1;

  Serial.print("\tBeat ");
  Serial.print(beat);
  Serial.print("\t\t");
  Serial.print(seq_getSequencerTime());
  Serial.print("\t Events: ");
  Serial.print(seqEvents->size()); 
  Serial.print("\t");
  long m = millis();
  Serial.print(m);
  Serial.print(" (");
  Serial.print(lastBeatTime-m);
  Serial.print("ms since, ");
  Serial.print(lastBeatTime-m+beatInterval);
  Serial.println("ms off)");
  lastBeatTime = millis();
}

void seq_onBar() {
  // work out what the current bar is
  Serial.print("Bar ");
  Serial.print(bar);
  Serial.print("\t\t\t");
  Serial.println(seq_getSequencerTime());
}

unsigned long seq_getSequencerTime() {
  sequencerTime = millis()-offsetFromSystemTime;
  return sequencerTime;
}

unsigned long getNextEventTime() {
  if (seqEvents->size() > 0) {
    OrbSequenceEvent event = seqEvents->get(0);
    Serial.println("Next event is ");
    printSequenceEvent(&event);
    return event.time;
  }
  else {
    Serial.println("No more events!");
    return -1;
  }
}

void seq_handleEvent() {
  // Looks for events that are due now, and does the needful with them.
  Serial.println("An event");

  // This should only be called when the next event is ready to fire,
  // so no need to check the time - just do it!

  // take the event out of seqEvents
  OrbSequenceEvent event = seqEvents->shift();
  Serial.println("The event is ");
  printSequenceEvent(&event);
  pastSeqEvents->add(event);


  // find when the next event is and reset this task to fire then
  seq_taskHandleEvents.setInterval(getNextEventTime()-seq_getSequencerTime());
}

void seq_handleLoop() {
  // reset sequencer time to the beginning of the loop
  // by changing the offset
  Serial.println("LOOOOOOOP!!!");
  offsetFromSystemTime = millis();
}

