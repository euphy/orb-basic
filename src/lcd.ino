#include <Arduino.h>
#include "painlessMesh.h"
#include <TFT_eSPI.h>


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

void lcd_showTime() {

  // update the LCD with the time
  long microseconds = micros();
  long milliseconds = microseconds/1000;
  long seconds = milliseconds/1000;
  long currentOffset = milliseconds - (mesh.getNodeTime()/1000);

  int textSize = 2;
  int rowPosition = textSize * 10;

  // lcd.fillRect(0, 0, 320, (rowPosition*3+24), TFT_BLACK);

  // sequencer time
  lcd.setTextSize(textSize);

  if (drawLabels) {
    lcd.setTextDatum(BL_DATUM);
    lcd.drawString("SEQ TIME:", 10, rowPosition);
  }
  lcd.setTextDatum(BR_DATUM);

  lcd.setTextColor(TFT_BLACK);
  lcd.drawNumber(sequencerTime, 300, rowPosition);
  if (seconds % 2 == 0) {
    lcd.setTextColor(tickColourEven);
  }
  else {
    lcd.setTextColor(tickColourOdd);
  }
  lcd.drawNumber(milliseconds, 300, rowPosition);
  sequencerTime = milliseconds;

  
  // offset from mesh time
  rowPosition += textSize * 10;
  if (drawLabels) {
    lcd.setTextColor(TFT_WHITE);
    lcd.setTextDatum(BL_DATUM);
    lcd.drawString("OFFSET:", 10, rowPosition);
  }
  lcd.setTextDatum(BR_DATUM);
  lcd.setTextColor(TFT_BLACK);
  lcd.drawNumber(offset, 300, rowPosition);
  lcd.setTextColor(TFT_WHITE);
  lcd.drawNumber(currentOffset, 300, rowPosition);
  offset = currentOffset;
  

  // Show musical time
  rowPosition += textSize * 10+8;
  int totalBeats = microseconds/1000/beatInterval;
  int totalBars = totalBeats/beatsPerBar;
  int totalTicks = microseconds/1000/tickInterval;

  int currentBar = totalBars;
  int currentBeat = totalBeats - (currentBar*beatsPerBar)+1;
  int currentTick = totalTicks - (totalBeats*ticksPerBeat)+1;
  

  rowPosition = 80;
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);
  if (drawLabels) {
    lcd.setTextDatum(BR_DATUM);
    lcd.drawString("Bar", 50, rowPosition);
    lcd.drawString("Beat", 180, rowPosition);
    lcd.drawString("Tick", 270, rowPosition);
  }

  lcd.setTextSize(3);
  lcd.setTextColor(TFT_BLACK);
  lcd.drawNumber(bar, 115, rowPosition);
  lcd.drawNumber(beat, 210, rowPosition);
  lcd.drawNumber(tick, 310, rowPosition);

  lcd.setTextColor(TFT_WHITE);
  lcd.drawNumber(currentBar, 115, rowPosition);
  lcd.drawNumber(currentBeat, 210, rowPosition);
  lcd.drawNumber(currentTick, 310, rowPosition);
  bar = currentBar;
  beat = currentBeat;
  tick = currentTick;

  drawLabels = false;
}


// void lcd_showNodeId() {
//   // update the LCD with the time
//   lcd.fillRect(0, 60, 320, 56, TFT_BLACK);
//   lcd.setTextColor(TFT_WHITE);
//   lcd.setTextSize(3);
//   lcd.setTextDatum(BL_DATUM);
//   lcd.drawString("ID:", 10, 100);
//   lcd.setTextDatum(BR_DATUM);
//   lcd.drawNumber(mesh.getNodeId(), 300, 100);
// }

void lcd_showNodeRole() {
  // update the LCD with the time
  lcd.fillRect(0, 120, 320, 56, TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(3);
  lcd.setTextDatum(BL_DATUM);
  lcd.drawString("Role:", 10, 160);
  lcd.setTextDatum(BR_DATUM);
  lcd.drawString(role, 300, 160);
}


void lcd_showConnectedNodes() {
  
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

void lcd_showInstrumentEvent(OrbInstrumentEvent *event, int x, int y) {

    // "%d:%d:%d t:%d  v:%d c:%d",
  
  char buffer [50];
  sprintf(buffer, 
    "%08d t:%02d n:%02d v:%03d c:%02d",
    event->time,
    event->type,
    event->note, 
    event->velocity,
    event->channel);

  lcd.drawString(buffer, x, y);


}

void lcd_showQueue() {
  OrbInstrumentEvent event = insEvents->get(0);
  lcd.setTextDatum(BL_DATUM);
  lcd.setTextColor(TFT_RED);
  lcd.setTextSize(2);

  // build list of instrument events
  lcd.fillRect(0, 130-16, 320, 130+(seqEvents->size()*16), TFT_BLACK);
  int row = 0;
  for (int i=0; i<seqEvents->size(); i++) {
    OrbSequenceEvent inEvent = seqEvents->get(i);
    OrbInstrumentEvent outEvent = {
      sequencerTime + musicPositionInMillis(&inEvent.position),
      inEvent.type,
      inEvent.note,
      inEvent.velocity
    };
    lcd_showInstrumentEvent(&outEvent, 0, 130+(row*16));
    row++;
  };
}