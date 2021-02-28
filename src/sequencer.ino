//************************************************************
// this is a simple example that uses the painlessMesh library
//
// 1. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 2. prints anything it receives to Serial.print
//
//
//************************************************************
#include "painlessMesh.h"
#include <FS.h>
#include "SD.h"
#include "SPI.h"
#include <Preferences.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <ArduinoJson.h>
#include <LinkedList.h>
#include "defs.h"

#include "firmware-build-name.h"

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

// Things used by everyone
Scheduler userScheduler;
painlessMesh  mesh;
Preferences preferences;

// Tasks
// Prototypes are here - implementation for these is elsewhere
void mesh_sendMessage(); // Prototype so PlatformIO doesn't complain
Task mesh_taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &mesh_sendMessage );
void mesh_describeSelf();
Task mesh_taskDescribeSelf(TASK_SECOND*2, TASK_FOREVER, &mesh_describeSelf);
void lcd_showTime();
Task task_lcdShowTime(TASK_SECOND/50, TASK_FOREVER, &lcd_showTime);
void lcd_showQueue();
Task task_lcdShowQueue(TASK_SECOND/10, TASK_FOREVER, &lcd_showQueue);




// Set up LCD and SD card peripherals
TFT_eSPI lcd = TFT_eSPI();       // Invoke custom library
int sdChipSelectPin = 25;
static int screenWidth = 320;
static int screenHeight = 240;
uint32_t tickColourOdd = TFT_WHITE;
uint32_t tickColourEven = TFT_DARKGREY;


// Global vars
const String FIRMWARE_VERSION_NO = "0.1";

// Machine role
const char* PREFKEY_MACHINE_ROLE = "machineRole";
String role = "SEQUENCER";


// Sequencer timing basic parameters
int ticksPerBeat = 16;
int beatsPerBar = 4;
float bpm = 90.0;
float beatInterval = 60000.0 / bpm;
float tickInterval = beatInterval / ticksPerBeat;
int ticksPerBar = ticksPerBeat * beatsPerBar;
// Example:
// 120bpm means a beat happens ever 0.5s. (60/120.)
// Each beat has 16 ticks, so a tick happens every 0.3125s (0.5/16.)

unsigned long sequencerTime = 0L;
long offset = 0L;
unsigned int bar = 0;
unsigned int beat = 0;
unsigned int tick = 0;
boolean drawLabels = true;

/*
ticksPerBar is how many discrete addresses are available in that bar.
tickInterval is how much time is in between each tick.

Events have a position: bar.beat.tick
Where bar is an integer
beat is 1 to 4 (integer)
tick is 0 to 15 (integer)
1.1.0 is the very first event, first beat of the first bar,
1.2.0 is the second beat,
1.2.8 is halfway between the second and the third beat.

Sequence is a list of Events.
A Sequence Player works out real time (in millis) positions for each Event.
It looks at which instrument is on each channel, and subtracts (stroke duration + strike duration) 
of that instrument to create a "broadcast time" to decide when this event should be transmitted 
as a command.

Sequence Player checks it's queue of ordered events at least every <tickInterval>.
It parcels the event up as a instrument command:

And sends it directly to the instrument on the channel, which will buffer it until 
it's clock is <time - stroke duration>.
*/

long musicPositionInMillis(OrbMusicPosition *position) {
  long val = 0L;
  val = position->bar * ticksPerBar*tickInterval;
  val += position->beat * beatInterval;
  val += position->tick * tickInterval;
  return val;
}

void printMusicTime(OrbMusicPosition *position) {
  Serial.print("Pos ");
  Serial.print(position->bar);
  Serial.print(":");
  Serial.print(position->beat);
  Serial.print(":");
  Serial.print(position->tick);
  Serial.print(" (");
  Serial.print(musicPositionInMillis(position));
  Serial.println("ms)");
}

void printInstrumentDefinition(OrbInstrumentDefinition *ins) {
  Serial.print("id: ");
  Serial.print(ins->meshId);
  Serial.print("\tstate: ");
  Serial.print(ins->state);
  Serial.print("\tstrokeDuration: ");
  Serial.print(ins->strokeDuration);
  Serial.print("\tstrikeDuration: ");
  Serial.print(ins->strikeDuration);
  Serial.print("\tresetDuration: ");
  Serial.println(ins->resetDuration);
}

void printSequenceEvent(OrbSequenceEvent *event) {
  Serial.print("");
  OrbMusicPosition *position = &event->position;
  Serial.print(position->bar);
  Serial.print(":");
  Serial.print(position->beat);
  Serial.print(":");
  Serial.print(position->tick);
  Serial.print(" ");
  Serial.print("\ttype: ");
  Serial.print(event->type);
  Serial.print("\tnote: ");
  Serial.print(event->note);
  Serial.print("\tvelocity: ");
  Serial.print(event->velocity);
  Serial.print("\tchannel: ");
  Serial.println(event->channel);
}

// These are an actual sequence 
LinkedList<OrbSequenceEvent> *seqEvents = new LinkedList<OrbSequenceEvent>;

int compareSequencerEventTime(OrbSequenceEvent &a, OrbSequenceEvent &b) {
  unsigned long aTime = a.time;
  unsigned long bTime = b.time;

  int val = 0;
  if (aTime > bTime) val = 1;
  else if (aTime < bTime) val = -1;

  Serial.print("compare ");
  Serial.println(val);
  return val;
}

void sortSequencerEvent(LinkedList<OrbSequenceEvent> *events) {
  for (int i=0; i<events->size(); i++) {
    OrbSequenceEvent seqEvent = seqEvents->get(i);
    seqEvent.time = musicPositionInMillis(&seqEvent.position); 
    events->set(i, seqEvent);
  }

  events->sort(compareSequencerEventTime);
}

void fillSeqEvents() {
  int numberOfEvents=20;
  int maxBars = 10;
  for (int i = 0; i<numberOfEvents; i++) {
    seqEvents->add({
      { random(0, maxBars),
        random(1,beatsPerBar), 
        random(1,ticksPerBeat)}, 0L, noteon, random(0,128), random(0,128), 10});
  }

  sortSequencerEvent(seqEvents);
}




void setup() {
  Serial.begin(115200);
  Serial.println("SEQ ONLINE!");

  fillSeqEvents();

  // dump out sequence events
  for (int i=0; i<seqEvents->size(); i++) {
    OrbSequenceEvent event = seqEvents->get(i);
    printSequenceEvent(&event);
  };

  OrbInstrumentDefinition snare = {disconnected, 0L, 1000, 125, 1600};
  
  Serial.print("id: ");
  Serial.print(snare.meshId);
  Serial.print("\tstate: ");
  Serial.print(snare.state);
  Serial.print("\tstrokeDuration: ");
  Serial.print(snare.strokeDuration);
  Serial.print("\tstrikeDuration: ");
  Serial.print(snare.strikeDuration);
  Serial.print("\tresetDuration: ");
  Serial.println(snare.resetDuration);

  printInstrumentDefinition(&snare);




  // Load configuration
  preferences.begin("orb", false);
  Serial.print("Role confirmed as ");
  Serial.println(role);

  lcd_initLCD();

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&mesh_receivedCallback);
  mesh.onNewConnection(&mesh_newConnectionCallback);
  mesh.onChangedConnections(&mesh_changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&mesh_nodeTimeAdjustedCallback);
  mesh.initOTAReceive(role);

  userScheduler.addTask(mesh_taskSendMessage);
  userScheduler.addTask(task_lcdShowTime);
  userScheduler.addTask(mesh_taskDescribeSelf);
  userScheduler.addTask(task_lcdShowQueue);
  
  // mesh_taskSendMessage.enable();
  // mesh_taskDescribeSelf.enable();
  task_lcdShowTime.enable();
  task_lcdShowQueue.enable();
  

}

void rebootEspWithReason(String reason) {
  Serial.println(reason);
  delay(1000);
  ESP.restart();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}
