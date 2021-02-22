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