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
  OrbEventType type;
  byte note;
  byte velocity;
  byte channel;
} OrbSequenceEvent;

typedef struct {
  long time;
  OrbEventType type;
  byte note;
  byte velocity;
  byte channel;
} OrbInstrumentEvent;

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


