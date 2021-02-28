#include <Arduino.h>


typedef struct {
  int bar;
  byte beat;
  byte tick;
} OrbMusicPosition;

enum OrbEventType {
  noteon,
  noteoff,
  strike,
  reset
};

typedef struct {
  OrbMusicPosition position;
  unsigned long time;
  OrbEventType type;
  byte note;
  byte velocity;
  byte channel;
} OrbSequenceEvent;

enum OrbInstrumentState {
  running, 
  stopped, 
  disconnected
};

typedef struct  {
  enum OrbInstrumentState state;
  long meshId;
  int strokeDuration;
  int strikeDuration;
  int resetDuration;
} OrbInstrumentDefinition;


