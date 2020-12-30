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
//#define  DEFAULT_ROLE "TREETOPS"
#define  DEFAULT_ROLE "CUPCAKES"
String role = DEFAULT_ROLE;

// Set up LCD and SD card peripherals
TFT_eSPI lcd = TFT_eSPI();       // Invoke custom library
int sdChipSelectPin = 25;
static int screenWidth = 320;
static int screenHeight = 240;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );
void sendMessage() {
  String msg = "CALL OUT GOURANGA! from node ";
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
  doc["stroke-duration"] = 100;
  doc["strike-duration"] = 10;
  doc["reset-duration"] = 55;
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

void showTime(); // 
Task taskShowTime(TASK_SECOND/8, TASK_FOREVER, &showTime);
void showTime() {
  // update the LCD with the time
  lcd.fillRect(0, 0, 320, 56, TFT_DARKGREY);
  lcd.setTextColor(TFT_WHITE);
  long meshTime = mesh.getNodeTime();
  lcd.setTextSize(3);
  lcd.setTextDatum(BL_DATUM);
  lcd.drawString("T:", 10, 40);
  lcd.setTextDatum(BR_DATUM);
  lcd.drawNumber(meshTime, 300, 40);

}

void showNodeId() {
  // update the LCD with the time
  lcd.fillRect(0, 60, 320, 56, TFT_DARKGREY);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(3);
  lcd.setTextDatum(BL_DATUM);
  lcd.drawString("ID:", 10, 100);
  lcd.setTextDatum(BR_DATUM);
  lcd.drawNumber(mesh.getNodeId(), 300, 100);
}

void showNodeRole() {
  // update the LCD with the time
  lcd.fillRect(0, 120, 320, 56, TFT_DARKGREY);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(3);
  lcd.setTextDatum(BL_DATUM);
  lcd.drawString("Role:", 10, 160);
  lcd.setTextDatum(BR_DATUM);
  lcd.drawString(role, 300, 160);
}

void showConnectedNodes() {
  
  lcd.fillRect(0, 180, 320, 56, TFT_DARKGREY);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(3);
  lcd.setTextDatum(BL_DATUM);
  lcd.drawString("Nodes:", 10, 220);
  lcd.setTextDatum(BR_DATUM);
  lcd.setTextSize(1);
  
  
  std::list<uint32_t> nodes = mesh.getNodeList();
  std::list<uint32_t>::iterator it;
  int yPos = 190;
  int xPos = 300;
  for (it=nodes.begin(); it!=nodes.end(); it++) {
    lcd.drawNumber(*it, xPos, yPos);
    yPos+=8;
  }
  lcd.setTextSize(3);
}


boolean loadRole(String defaultRole) {
  role = defaultRole;

  boolean searchingSdCard = true;

  // look on the SD card first
  if (SD.begin(sdChipSelectPin)) {

    File dir = SD.open("/");
    while (searchingSdCard) {
      File entry = dir.openNextFile();
      if (!entry) { //End of files
        searchingSdCard = false;
        break;
      }

      //This block of code parses the file name to make sure it is valid.
      if (!entry.isDirectory()) {
        TSTRING name = entry.name();
        if (name.length() > 5 && name.indexOf('_') != -1 && name.indexOf('.') != -1) {

          TSTRING roleIndicator = name.substring(1, name.indexOf('_'));
          // Serial.print("Found a file called '");
          // Serial.print(roleIndicator);
          // Serial.println("'.");
          if (roleIndicator.equals("role")) {

            TSTRING newRole = name.substring(name.lastIndexOf('_') + 1, name.indexOf('.'));
            Serial.print("Extracted new role as '");
            Serial.print(newRole);
            Serial.println("'.");
            preferences.putString(PREFKEY_MACHINE_ROLE, newRole);
            searchingSdCard = false;

            TSTRING extension = name.substring(name.indexOf('.') + 1, name.length());

          }
          else {
            Serial.print(".");
            // Serial.print("File '");
            // Serial.print(name);
            // Serial.println("' is not a role file: Ignoring.");
          }
        }
        else {
          Serial.print(".");
          // Serial.print("File '");
          // Serial.print(name);
          // Serial.println("' doesn't have _ and . in it: Ignoring.");
        }
      }
    }
  }
  else {
        Serial.println("Could not mount SD card.");
  }

  // now look for it in prefs
  String terribleDefaultRole = "TERRIBLE";
  String roleFromPrefs = preferences.getString(PREFKEY_MACHINE_ROLE, terribleDefaultRole);
  // Serial.print("roleFromPrefs is ");
  // Serial.print(roleFromPrefs);
  // Serial.println(".");
  if (roleFromPrefs.equals(terribleDefaultRole)) {
    Serial.println("Couldn't load the role from prefs!");
    role = defaultRole;
  }
  else {
    Serial.print("Loaded role ");
    Serial.print(roleFromPrefs);
    Serial.println(" from prefs.");
    role = roleFromPrefs;
    return true;
  }
  
  return false;
}

// Look for a file called role_TREETOPS.txt 

void setup() {
  Serial.begin(115200);
  Serial.print("ORB ONLINE! My default role is ");
  Serial.println(role);

  // Load configuration
  preferences.begin("orb", false);

  loadRole(DEFAULT_ROLE);
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
  lcd.drawString("One Robot Band.", targetPosition-1, barTop+23, 4);
  lcd.setTextColor(TFT_WHITE);
  lcd.drawString("One Robot Band.", targetPosition, barTop+24, 4);
  lcd.drawString("One Robot Band.", targetPosition+1, barTop+24, 4); // bold it with double

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
