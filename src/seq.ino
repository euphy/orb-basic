void seq_onBeat() {
    Serial.print("\tBeat ");
    Serial.print(beat);
    Serial.print("\t\t");
    Serial.print(seq_getSequencerTime());
    Serial.print("\t Events: ");
    Serial.println(seqEvents->size());
}

void seq_onBar() {
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



