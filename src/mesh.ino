#include <Arduino.h>
#include "painlessMesh.h"
#include <TFT_eSPI.h>

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


long getTime() {
  return mesh.getNodeTime();
}




