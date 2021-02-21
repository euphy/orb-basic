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

#include "firmware-build-name.h"

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;
Preferences preferences;


const String FIRMWARE_VERSION_NO = "0.1";

// Set up machine role
const char* PREFKEY_MACHINE_ROLE = "machineRole";
String role = "SEQUENCER";

// Set up LCD and SD card peripherals
TFT_eSPI lcd = TFT_eSPI();       // Invoke custom library
int sdChipSelectPin = 25;
static int screenWidth = 320;
static int screenHeight = 240;

uint32_t tickColourOdd = TFT_BLACK;
uint32_t tickColourEven = TFT_DARKGREY;


// some timing stuff
// Example:
// 120bpm means a beat happens ever 0.5s. (60/120.)
// Each beat has 16 ticks, so a tick happens every 0.3125s (0.5/16.)
int ticksPerBeat = 16;
int beatsPerBar = 4;
float bpm = 120.0;
float beatInterval = 60000.0 / bpm;
float tickInterval = beatInterval / ticksPerBeat;
int ticksPerBar = ticksPerBeat * beatsPerBar;

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


Position
  int: bar
  byte: beat
  byte: tick

*/


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


/*
Sequencer Event
  Position: pos
  EventType: type (eg "noteon")
  int: note
  int: velocity
  int: channel
*/

typedef struct {
  OrbMusicPosition position;
  OrbEventType type;
  byte note;
  byte velocity;
  byte channel;
} OrbSequenceEvent;



/*
Sequence is a list of Events.
A Sequence Player works out real time (in millis) positions for each Event.
It looks at which instrument is on each channel, and subtracts (stroke duration + strike duration) 
of that instrument to create a "broadcast time" to decide when this event should be transmitted 
as a command.

Sequence Player checks it's queue of ordered events at least every <tickInterval>.
It parcels the event up as a instrument command:

Instrument Event
  long: time  (when the event should happen.)
  EventType: type 
  int: velocity
*/

typedef struct {
  long time;
  OrbEventType type;
  byte note;
  byte velocity;
  byte channel;
} OrbInstrumentEvent;

/*
And sends it directly to the instrument on the channel, which will buffer it until 
it's clock is <time - stroke duration>.
*/


long musicPositionInMillis(OrbMusicPosition *position) {
  long val = 0L;
  val = position->bar * ticksPerBar;
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

OrbSequenceEvent seqEvent = {{1,2,3}, noteon, 12, 64, 10};
OrbInstrumentEvent insEvent = {
  millis()+musicPositionInMillis(&seqEvent.position),
  seqEvent.type,
  seqEvent.note,
  seqEvent.velocity,
  seqEvent.channel
};

LinkedList<OrbSequenceEvent> *seqEvents = new LinkedList<OrbSequenceEvent>;
LinkedList<OrbInstrumentEvent> *insEvents = new LinkedList<OrbInstrumentEvent>;

void sendMessage(); // Prototype so PlatformIO doesn't complain
Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );
void sendMessage() {
  String msg = "BEWARE THE SPIDER CONTROLLER from node ";
  msg += role;
  msg += mesh.getNodeId();
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void describeSelf();
Task taskDescribeSelf(TASK_SECOND*2, TASK_FOREVER, &describeSelf);
void describeSelf() {
  DynamicJsonDocument doc(2048);
  doc["name"] = "machine name";
  doc["id"] = mesh.getNodeId();
  doc["role"] = role;
  serializeJsonPretty(doc, Serial);
  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast("no, don't");
  Serial.println("Hi!");
  std::list<uint32_t> nodes = mesh.getNodeList();
  std::list<uint32_t>::iterator it;
  Serial.print("Connected nodes: ");
  for (it=nodes.begin(); it!=nodes.end(); it++) {
    Serial.print(*it);
    Serial.print(" ");
  }
  Serial.println(".");

  showNodeId();
  showNodeRole();  
  showConnectedNodes();
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void showTime();
Task taskShowTime(TASK_SECOND/50, TASK_FOREVER, &showTime);
void showTime() {
  // update the LCD with the time
  long meshTime = mesh.getNodeTime();
  long seconds = meshTime/1000/1000;
  if (seconds % 2 == 0) {
    lcd.fillRect(0, 0, 320, 56, tickColourEven);
  }
  else {
    lcd.fillRect(0, 0, 320, 56, tickColourOdd);
  }
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(3);
  lcd.setTextDatum(BL_DATUM);
  lcd.drawString("T:", 10, 40);
  lcd.setTextDatum(BR_DATUM);
  lcd.drawNumber(meshTime, 300, 40);
}

long getTime() {
  return mesh.getNodeTime();
}

void showNodeId() {
  // update the LCD with the time
  lcd.fillRect(0, 60, 320, 56, TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(3);
  lcd.setTextDatum(BL_DATUM);
  lcd.drawString("ID:", 10, 100);
  lcd.setTextDatum(BR_DATUM);
  lcd.drawNumber(mesh.getNodeId(), 300, 100);
}

void showNodeRole() {
  // update the LCD with the time
  lcd.fillRect(0, 120, 320, 56, TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(3);
  lcd.setTextDatum(BL_DATUM);
  lcd.drawString("Role:", 10, 160);
  lcd.setTextDatum(BR_DATUM);
  lcd.drawString(role, 300, 160);
}

void showConnectedNodes() {
  
  std::list<uint32_t> nodes = mesh.getNodeList();

  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(3);
  lcd.setTextDatum(BL_DATUM);

  if (nodes.empty()) {
    lcd.fillRect(0, 180, 320, 56, TFT_RED);
  }
  else {
    lcd.fillRect(0, 180, 320, 56, TFT_BLACK);
  }
  lcd.drawString("Nodes:", 10, 220);
  lcd.drawNumber(nodes.size(), 130, 220);
  
  // print the connected node IDs
  lcd.setTextDatum(BR_DATUM);
  lcd.setTextSize(1);
  
  std::list<uint32_t>::iterator it;
  int yPos = 190;
  int xPos = 300;
  for (it=nodes.begin(); it!=nodes.end(); it++) {
    lcd.drawNumber(*it, xPos, yPos);
    yPos+=8;
  }
  lcd.setTextSize(3);
}

// channel info
enum OrbInstrumentState {
  running, 
  stopped, 
  disconnected
};

struct OrbInstrumentDefinition {
  OrbInstrumentState state;
  long meshId;
  int strokeDuration;
  int strikeDuration;
  int resetDuration;
};

void printInstrumentDefinition(struct OrbInstrumentDefinition *ins) {
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

// Why doesn't this typedeffed version work?
// typedef struct  {
//   OrbInstrumentState state;
//   long meshId;
//   int strokeDuration;
//   int strikeDuration;
//   int resetDuration;
// } OrbInstrumentDefinition;

// void printInstrumentDefinition(OrbInstrumentDefinition *ins) {
//   Serial.print("id: ");
//   Serial.print(ins->meshId);
//   Serial.print("\tstate: ");
//   Serial.print(ins->state);
//   Serial.print("\tstrokeDuration: ");
//   Serial.print(ins->strokeDuration);
//   Serial.print("\tstrikeDuration: ");
//   Serial.print(ins->strikeDuration);
//   Serial.print("\tresetDuration: ");
//   Serial.println(ins->resetDuration);
// }



void printSequenceEvent(OrbSequenceEvent *event) {
  Serial.print("");
  OrbMusicPosition *position = &event->position;
  Serial.print(position->bar);
  Serial.print(":");
  Serial.print(position->beat);
  Serial.print(":");
  Serial.print(position->tick);
  Serial.print(" (");
  Serial.print(musicPositionInMillis(position));
  Serial.print("ms) ");
  Serial.print("\ttype: ");
  Serial.print(event->type);
  Serial.print("\tnote: ");
  Serial.print(event->note);
  Serial.print("\tvelocity: ");
  Serial.print(event->velocity);
  Serial.print("\tchannel: ");
  Serial.println(event->channel);
}

void printInstrumentEvent(OrbInstrumentEvent *event) {
  Serial.print("time: ");
  Serial.print(event->time);
  Serial.print("ms ");
  Serial.print("\ttype: ");
  Serial.print(event->type);
  Serial.print("\tnote: ");
  Serial.print(event->note);
  Serial.print("\tvelocity: ");
  Serial.print(event->velocity);
  Serial.print("\tchannel: ");
  Serial.println(event->channel);
}





void setup() {
  Serial.begin(115200);
  Serial.println("SEQ ONLINE!");
  
  Serial.println(seqEvents->size());
  seqEvents->add(seqEvent);
  seqEvents->add({{2,0,4}, noteon, 10, 65, 10});
  seqEvents->add({{0,0,4}, noteon, 10, 65, 10});
  seqEvents->add({{1,0,4}, noteon, 9, 65, 10});
  seqEvents->add({{4,0,4}, noteon, 8, 65, 10});
  seqEvents->add({{5,0,4}, noteon, 7, 65, 10});
  seqEvents->add({{6,0,4}, noteon, 6, 65, 10});
  Serial.println(seqEvents->size());

  // dump out sequence events
  for (int i=0; i<seqEvents->size(); i++) {
    OrbSequenceEvent event = seqEvents->get(i);
    printSequenceEvent(&event);
  };

  // build list of instrument events
  for (int i=0; i<seqEvents->size(); i++) {
    OrbSequenceEvent inEvent = seqEvents->get(i);
    OrbInstrumentEvent outEvent = {
      millis()+musicPositionInMillis(&inEvent.position),
      inEvent.type,
      inEvent.note,
      inEvent.velocity
    };
    insEvents->add(outEvent);
    printInstrumentEvent(&outEvent);
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
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.initOTAReceive(role);

  userScheduler.addTask(taskSendMessage);
  userScheduler.addTask(taskShowTime);
  userScheduler.addTask(taskDescribeSelf);
  taskSendMessage.enable();
  taskShowTime.enable();
  taskDescribeSelf.enable();

}

void rebootEspWithReason(String reason) {
  Serial.println(reason);
  delay(1000);
  ESP.restart();
}

void lcd_drawSplashScreen()
{
  lcd.fillScreen(TFT_BLACK);
  int barTop = 80;
  int barHeight = 100;
  int targetPosition = 35;

  lcd.fillRect(0, barTop, screenWidth, barHeight, TFT_RED);
  lcd.setTextSize(1);

  // write it with a drop shadow
  lcd.setTextColor(TFT_MAROON);
  lcd.drawString("O.R.B. SEQUENCER.", targetPosition-1, barTop+23, 4);
  lcd.setTextColor(TFT_WHITE);
  lcd.drawString("O.R.B. SEQUENCER.", targetPosition, barTop+24, 4);
  lcd.drawString("O.R.B. SEQUENCER.", targetPosition+1, barTop+24, 4); // bold it with double

  lcd.setTextColor(TFT_MAROON);
  lcd.drawString("An open-source art project", targetPosition+2, barTop+31+(9*3), 2);
  lcd.setTextColor(TFT_WHITE);
  lcd.drawString("An open-source art project", targetPosition+3, barTop+32+(9*3), 2);

  lcd.setTextDatum(BR_DATUM);
  lcd.drawString("v"+FIRMWARE_VERSION_NO, 310, 210, 1);
  lcd.drawString(firmwareBuildName, 310, 220, 1);
  lcd.drawString(role, 310, 230, 1);
}

void lcd_initLCD()
{
  lcd.init();
  lcd.setRotation(3);
  lcd.setTextDatum(TL_DATUM);
  lcd_drawSplashScreen();
  delay(1000);
  lcd.fillScreen(TFT_BLACK);
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}
