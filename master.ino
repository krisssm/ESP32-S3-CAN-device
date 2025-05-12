#include <mcp_can.h>
#include <SPI.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <TFT_eSPI.h>



//Everything for CAN controller
long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];  // Array to store serial string
#define CAN0_INT 4    // Set INT to pin 4
MCP_CAN CAN0(10);     // Set CS to pin 10



//Define all parameters to read and display engine data
#define PID_ENGINE_RPM 640  //Define ID in decimal for engine rpm
int engineRpm = 893;
int lastEngineRpm = 0;
int maxRPM = 6000; //Default for the maximum RPM
#define MAX_RPM_VALUE 14000
#define MIN_RPM_VALUE 1000

#define PID_COOLANT_TEMP 648  //Define ID in decimal for coolant temperature
int coolantTemp = 0;
int lastCoolantTemp;
int maxCoolantTemp = 100; //Default for the maximum coolant temp
#define MAX_COOLANT_VALUE 120
#define MIN_COOLANT_VALUE 0

#define PID_INTAKE_TEMP 896  //Define ID in decimal for intake temperature
int intakeTemp = 0;
int lastIntakeTemp;
int maxIntakeTemp = 100; //Default for the maximum intake temp
#define MAX_INTAKE_VALUE 140
#define MIN_INTAKE_VALUE 0
float intakePressure = 0;



//Everything for TFT-screen
TFT_eSPI tft = TFT_eSPI();
int screen = 1;  //RPM 1, Coolant 2, Intake 3 (Start at RPM screen)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 4

//Touchscreen coordinates, z is for pressure
uint16_t x, y, z;

//X and Y coordinates for center of display
int centerX = SCREEN_WIDTH / 2;
int centerY = SCREEN_HEIGHT / 2;



void printRPM(int rpm) {
  if (rpm >= maxRPM) {             //Check if rpm reading is higher than max value
    if (lastEngineRpm < maxRPM) {  //Check if the last reading was lower, so the screen has to be changed red
      tft.fillScreen(TFT_RED);
      tft.setTextColor(TFT_WHITE, TFT_RED);
    }
  } else if (lastEngineRpm >= maxRPM) {  //Check if the last reading was higer, so the screen is red and has to be changed to white
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
  }

  tft.drawCentreString("Mootoripoorded", centerX, centerY - 50, FONT_SIZE);
  tft.drawCentreString("   " + String(rpm) + " " + "p/min"+ " ", centerX, centerY - 25, FONT_SIZE);
  tft.drawCentreString("Seadista", centerX, centerY + 80, FONT_SIZE);

  lastEngineRpm = rpm;  //Remember the last rpm value
}

void printCoolantTemp(int temp) {  //Print coolant temp
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Jahutusvedeliku temp", centerX, centerY - 50, FONT_SIZE);
  tft.drawCentreString("   " + String(temp) + " " + "`" + "C" + " ", centerX, centerY - 25, FONT_SIZE);
  tft.drawCentreString("Seadista", centerX, centerY + 80, FONT_SIZE);
}

void printIntakePressure(float pressure) {  //Print coolant temp
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Sisselaske rohk", centerX, centerY - 50, FONT_SIZE);
  tft.drawCentreString("   " + String(pressure) + " " + "bar" + " ", centerX, centerY - 25, FONT_SIZE);
}

void printIntakeTemp(int temp) {  //Print intake temp
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Sisselaske temp", centerX, centerY - 50, FONT_SIZE);
  tft.drawCentreString("   " + String(temp) + " " + "`" + "C" + " ", centerX, centerY - 25, FONT_SIZE);
  tft.drawCentreString("Seadista", centerX, centerY + 80, FONT_SIZE);
}

void printCoolantMessage() {

  if (engineRpm >= maxRPM) {  //Check if the rpm is at the max, so the colors need to change, because the background is red
    tft.setTextColor(TFT_RED, TFT_WHITE);
    tft.drawCentreString("Kontrolli jahutusvedelikku!", centerX, centerY + 45, FONT_SIZE);
    tft.setTextColor(TFT_WHITE, TFT_RED);
  } else {
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawCentreString("Kontrolli jahutusvedelikku!", centerX, centerY + 45, FONT_SIZE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
  }
}

void printIntakeMessage() {

  if (engineRpm >= maxRPM) {  //Check if the rpm is at the max, so the colors need to change, because the background is red
    tft.setTextColor(TFT_RED, TFT_WHITE);
    tft.drawCentreString("Kontrolli sisselaset!", centerX, centerY + 45, FONT_SIZE);
    tft.setTextColor(TFT_WHITE, TFT_RED);
  } else {
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawCentreString("Kontrolli sisselaset!", centerX, centerY + 45, FONT_SIZE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
  }
}

//Function to print the arrows and the button to save the new max value
void printMaxScreen() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Salvesta", centerX, centerY + 80, FONT_SIZE);
  //Print left arrow
  tft.drawLine(10, 120, 25, 100, TFT_BLACK);
  tft.drawLine(10, 120, 25, 140, TFT_BLACK);
  //Print right arrow
  tft.drawLine(295, 100, 310, 120, TFT_BLACK);
  tft.drawLine(295, 140, 310, 120, TFT_BLACK);
}

void changeRPMValue() {

  printMaxScreen();
  tft.drawCentreString("Poorete maksimum", centerX, centerY - 50, FONT_SIZE);
  tft.drawCentreString("   " + String(maxRPM) + " " + "p/min"+ " ", centerX, centerY - 15, FONT_SIZE);

  while (1) {
    delay(100);

    if (tft.getTouch(&x, &y, 600)) {
      delay(100);                    //Delay to ignore multiple presses
      if (x < 30 && 90 < y < 130) {  //If left arrow is pressed

        if (maxRPM == MIN_RPM_VALUE) {  //Check if the rpm is at the minimum, so the value cannot decrease
          maxRPM = MIN_RPM_VALUE;
        } else {
          maxRPM -= 500;
        }
        tft.drawCentreString("   " + String(maxRPM) + " " + "p/min"+ " ", centerX, centerY - 15, FONT_SIZE);

      } else if (x > 270 && 90 < y < 130) {  //If right arrow is pressed

        if (maxRPM == MAX_RPM_VALUE) {  //Check if the rpm is at the maximum, so the value cannot increase
          maxRPM = MAX_RPM_VALUE;
        } else {
          maxRPM += 500;
        }
        tft.drawCentreString("   " + String(maxRPM) + " " + "p/min"+ " ", centerX, centerY - 15, FONT_SIZE);

      } else if (100 < x < 2000 && y < 40) {  //Check if the "Save" button is pressed
        tft.fillScreen(TFT_WHITE);
        return;
      }
    }
  }
}

void changeCoolantValue() {

  printMaxScreen();
  tft.drawCentreString("Jahutusvedeliku", centerX, centerY - 85, FONT_SIZE);
  tft.drawCentreString("temperatuuri maksimum", centerX, centerY - 50, FONT_SIZE);
  tft.drawCentreString("   " + String(maxCoolantTemp) + " " + "`" + "C" + " ", centerX, centerY - 15, FONT_SIZE);
  while (1) {
    delay(100);
    if (tft.getTouch(&x, &y, 600)) {
      delay(100);                    //Delay to ignore multiple presses
      if (x < 30 && 90 < y < 130) {  //If left arrow is pressed

        if (maxCoolantTemp == MIN_COOLANT_VALUE) {  //Check if the coolant temp is at the minimum, so the value cannot decrease
          maxCoolantTemp = MIN_COOLANT_VALUE;
        } else {
          maxCoolantTemp -= 2;
        }
        tft.drawCentreString("   " + String(maxCoolantTemp) + " " + "`" + "C" + " ", centerX, centerY - 15, FONT_SIZE);

      } else if (x > 270 && 90 < y < 130) {        //If right arrow is pressed
        if (maxCoolantTemp == MAX_COOLANT_VALUE) {  //Check if the coolant temp is at the maximum, so the value cannot increase
          maxCoolantTemp = MAX_COOLANT_VALUE;
        } else {
          maxCoolantTemp += 2;
        }
        tft.drawCentreString("   " + String(maxCoolantTemp) + " " + "`" + "C" + " ", centerX, centerY - 15, FONT_SIZE);

      } else if (100 < x < 2000 && y < 40) {  //Check if the "Save" button is pressed
        tft.fillScreen(TFT_WHITE);
        return;
      }
    }
  }
}

void changeIntakeValue() {
  printMaxScreen();
  tft.drawCentreString("Sisselaske", centerX, centerY - 85, FONT_SIZE);
  tft.drawCentreString("temperatuuri maksimum", centerX, centerY - 50, FONT_SIZE);
  tft.drawCentreString("   " + String(maxIntakeTemp) + " " + "`" + "C" + " ", centerX, centerY - 15, FONT_SIZE);
  while (1) {
    delay(100);  //Delay to ignore multiple presses
    if (tft.getTouch(&x, &y, 600)) {
      delay(100);  //Delay to ignore multiple presses

      if (x < 30 && 90 < y < 130) {  //If left arrow is pressed

        if (maxIntakeTemp == MIN_INTAKE_VALUE) {  //Check if the intake temp is at the minimum, so the value cannot decrease
          maxIntakeTemp = MIN_INTAKE_VALUE;
        } else {
          maxIntakeTemp -= 2;
        }

        tft.drawCentreString("   " + String(maxIntakeTemp) + " " + "`" + "C" + " ", centerX, centerY - 15, FONT_SIZE);
      } else if (x > 270 && 90 < y < 130) {  //If right arrow is pressed

        if (maxIntakeTemp == MAX_INTAKE_VALUE) {  //Check if the intake temp is at the maximum, so the value cannot increase
          maxIntakeTemp = MAX_INTAKE_VALUE;
        } else {
          maxIntakeTemp += 2;
        }

        tft.drawCentreString("   " + String(maxIntakeTemp) + " " + "`" + "C" + " ", centerX, centerY - 15, FONT_SIZE);
      } else if (100 < x < 2000 && y < 40) {  //Check if the "Save" button is pressed
        tft.fillScreen(TFT_WHITE);
        return;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  //Start the tft 
  tft.init();
  //Set the TFT display to landscape
  tft.setRotation(1);

  //Clear the screen before writing to it
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  //Initialize MCP2515 at 8MHz, baudrate 500kb/s, without masks and filters.
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
  } else {
    Serial.println("Error Initializing MCP2515 controller!");
  }

  CAN0.setMode(MCP_NORMAL);  //Set operation mode to normal
  pinMode(CAN0_INT, INPUT);  //Configure MCP2515 pin for input
}

void loop() {

  if (tft.getTouch(&x, &y, 600)) {
    if (100 < x < 200 && y < 40) {  //Check if the "Set value" button is pressed
      switch (screen) {
        case 1:
          changeRPMValue();
          break;
        case 2:
          changeCoolantValue();
          break;
        case 3:
          changeIntakeValue();
          break;
        default:
          tft.fillScreen(TFT_WHITE);
          break;
      }
    } else if (screen != 4) {  //If "Set value" button was not pressed, change the screen
      screen += 1;
      tft.fillScreen(TFT_WHITE);
    } else {
      screen = 1;
      tft.fillScreen(TFT_WHITE);
    }
    delay(400);
  }

  if (!digitalRead(CAN0_INT)) {  //If INT pin is low, the buffer can be read

    CAN0.readMsgBuf(&rxId, &len, rxBuf);  // Read data

    //Look for correct message id's
    if (rxId == PID_ENGINE_RPM) {

      engineRpm = ((int)rxBuf[3] * 100 + (int)rxBuf[2] * 1) / 2;  //Calculate value for rpm
    }
    else if (rxId == PID_COOLANT_TEMP) {
      coolantTemp = (int)rxBuf[1] * 0.75 - 48;  //Calculate value for coolant temperature
      intakePressure = ((rxBuf[7] - 75) / 4) - 0.0689475729; //Calculate value for intake pressure
    }
    else if (rxId == PID_INTAKE_TEMP) {
      intakeTemp = (int)rxBuf[1] * 0.75 - 48;  //Calculate value for intake temp
    }
  }

  //Set the correct screen
  switch (screen) {
    case 1:
      printRPM(engineRpm);
      break;
    case 2:
      printCoolantTemp(coolantTemp);
      break;
    case 3:
      printIntakeTemp(intakeTemp);
      break;
    case 4:
      printIntakePressure(intakePressure);
      break;
    default:
      tft.fillScreen(TFT_WHITE);
      break;
  }

  if (coolantTemp >= maxCoolantTemp) { //Check if coolant temp is over set max value
    printCoolantMessage();
  } else if (intakeTemp >= maxIntakeTemp) { //Check if intake temp is over set max value
    printIntakeMessage();
  }
}