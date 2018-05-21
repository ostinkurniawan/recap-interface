#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <DS3231.h>

#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 8   // can be a digital pin
#define XP 9   // can be a digital pin

#define TS_MINX 130
#define TS_MAXX 905

#define TS_MINY 75
#define TS_MAXY 930

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
// optional
#define LCD_RESET A4

#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF


#define MINPRESSURE 10
#define MAXPRESSURE 1000

#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
DS3231 rtc(SDA, SCL);

int currDoseLevel;
bool timeToCheckIn;
bool lockedOut;
String doses[4] = {"1 Tylenol", "2 Tylenol", "1 Dilaudid", "2 Dilaudid"};
int lastDose;

long lockOutStartTime;
long lockOutInterval;

Time lastCheckInTime;

long lockOutDuration;

////  SETUP  ////
void setup(void) {
  Serial.begin(9600);

  // TFT SETUP //
  tft.reset();
  tft.begin(tft.readID()); //  read the type of TFT screen
  Serial.print("TFT size is "); Serial.print(tft.width()); Serial.print("x"); Serial.println(tft.height());
  tft.setRotation(1);
  tft.setFont(&FreeSansBold12pt7b);
  tft.fillScreen(WHITE);

  // REAL TIME CLOCK SETUP //
  rtc.begin();

  // SETUP VARIABLES //
  timeToCheckIn = true;
  lockedOut = false;
  lockOutInterval = 1.0; //lock out interval, in minutes.
  lockOutDuration = lockOutInterval * 60 * 1000;

  //debug touch
}

////  MAIN LOOP  ////
void loop()
{
  if (timeToCheckIn) {
    checkInScreen();
  } else {
    if (lockedOut) {
      lockedOutScreen();
    } else {
      nonCheckInScreen();
    }
  }
}

//// CHECK IN SCREEN ////
void checkInScreen() {
  resetScreen();

  tft.setCursor(0, 92);
  tft.setFont(&FreeSansBold12pt7b);
  tft.println("     Hello!");
  tft.println("     It's time for you to check in.");
  tft.setTextSize(1);
  tft.setFont(&FreeSansBold9pt7b);
  tft.println("       Your last check in was at 10:00am."); //// add lastCheckInTime


  showContButton();
  int answer = getContAnswer();

  if (answer == 1) {
    getPainLevelScreen();
  }
}

//// TIME TO CHECK IN SCREEN ////
void nonCheckInScreen() {
  resetScreen();
  tft.setTextColor(BLACK);
  tft.setCursor(0, 70);
  tft.setFont(&FreeSans12pt7b);
  tft.println("     Your next check in is in");
  tft.setFont(&FreeSansBold12pt7b);
  tft.println("     2 hours, at 12:00pm."); //// add nextCheckInTime
  tft.setTextSize(1);
  tft.setFont(&FreeSans9pt7b);
  tft.print("       Your last check in was at ");
  tft.setFont(&FreeSansBold9pt7b);
  tft.print("10:00am"); //// add lastCheckInTime
  tft.setFont(&FreeSans9pt7b);
  tft.println(" and ");
  tft.print("       you received ");
  tft.setFont(&FreeSansBold9pt7b);
  String tempLD = getLastDose(lastDose);
  tft.print(tempLD);
  tft.println(".");
  tft.println();
  tft.println("       If you are currently in pain and need medication,");
  tft.println("       press Override.");

  showOverrideButton(0);

  int answer = getContAnswer();
  if (answer == 1) {
    confirmOverrideScreen();
  }
}


//// LOCKED OUT FROM TAKING MEDICATION SCREEN ////
void lockedOutScreen() {
  long timeElapsed = millis() - lockOutStartTime;
  refreshDelayScreen(lockOutDuration - timeElapsed);

  while (lockedOut) {
    long timeElapsed = millis() - lockOutStartTime;
    if (millis() - lockOutStartTime > lockOutDuration) {
      lockedOut = false;
    } else {
      refreshDelayScreen(lockOutDuration - timeElapsed);
      if ((lockOutDuration - timeElapsed) > 120000) {
        delay(60000);
      } else if ((lockOutDuration - timeElapsed) > 60000) {
        delay(15000);
      } else {
        delay(2000);
      }
    }
  }
}

//// LOCKED OUT FROM TAKING MEDICATION SCREEN  – DISPLAY ////

void refreshDelayScreen(long t) {
  resetScreen();
  tft.setTextColor(BLACK);
  tft.setCursor(0, 70);
  tft.setFont(&FreeSansBold12pt7b);
  tft.println("     Your next check in is in");
  tft.println("     2 hours, at 12:00pm."); //// add nextCheckInTime
  tft.setTextSize(1);
  tft.setFont(&FreeSans9pt7b);
  tft.print("       Your last check in was at ");
  tft.setFont(&FreeSansBold9pt7b);
  tft.print("10:00am"); //// add lastCheckInTime
  tft.setFont(&FreeSans9pt7b);
  tft.println(" and");
  tft.print("       you received ");
  tft.setFont(&FreeSansBold9pt7b);
  String tempLD = getLastDose(lastDose);
  tft.print(tempLD);
  tft.println(".");
  tft.println();
  tft.println("       It is not safe for you to take another");
  tft.print("       dose for ");
  long totalSeconds = t/1000; // time in seconds
  int totalMinutes = int(totalSeconds/60);
  int secondsLeft = int(totalSeconds%60);

  if (totalMinutes > 0) {
    tft.print(totalMinutes + 1); // add 1 because dividing integers rounds down
    tft.print(" minutes.");
  } else {
    tft.print(secondsLeft);
    tft.print(" seconds.");
  }
}

//// USER SELECTS THE DOSE NEEDED ////
void selectDoseScreen(int level) {
  currDoseLevel = level;
  resetScreen();

  showBackButton();
  showDoseButtons();

  tft.setCursor(0, 130);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(BLACK);
  tft.println("      You selected");
  tft.setFont(&FreeSansBold12pt7b);


  if (level == 1) {
    tft.println("     no hurt.");
  } else if (level == 2) {
    tft.println("     hurts little bit.");
  } else if (level == 3) {
    tft.println("     hurts little more.");
  } else if (level == 4) {
    tft.println("     hurts even more.");
  } else if (level == 5) {
    tft.println("     hurts whole lot.");
  } else if (level == 6) {
    tft.println("     hurts most.");
  }
  tft.setFont(&FreeSans12pt7b);
  tft.println();
  tft.println("      Select a");
  tft.println("      medication");
  tft.println("      option.");

  int answer = getDoseLevelAnswer();

  if (answer == 9) { //back
    getPainLevelScreen();
  } else if (answer == 5) { // none
    lastDose = answer - 1;
    noMedSelectedScreen();
  }
  else {
    lastDose = answer - 1;
    confirmDoseScreen(answer);
  }

}

//// USER CONFIRMS THE DOSE CHOSEN ////
void confirmDoseScreen(int a) {
  resetScreen();
  showBackButton();
  showDispenseButton();
  tft.setCursor(90, 150);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(BLACK);
  tft.print("   You selected ");
  tft.setFont(&FreeSansBold12pt7b);
  int selection = a - 1;
  if (selection == 4) {
    tft.print("none");
  } else {
    tft.print(doses[selection]);
  }
  tft.print(".");

  int answer = getConfirmDispenseAnswer();

  if (answer == 1) {
    dispenseConfirmedScreen(selection);
  } else if (answer == 2) {
    selectDoseScreen(currDoseLevel);
  }
}

//// USER CONFIRMS THE OVERRIDE ////
void confirmOverrideScreen() {
  resetScreen();
  showBackButton();
  showOverrideButton(1);
  tft.setCursor(0, 150);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(BLACK);
  tft.println("    It is not recommended that you");
  tft.println("    take a dose for another 30 minutes.");

  int answer = getConfirmDispenseAnswer();

  if (answer == 1) {
    getPainLevelScreen();
  } else if (answer == 2) {
    nonCheckInScreen();
  }
}

//// USER SELECTS THE PAIN LEVEL THEY FEEL ////
void getPainLevelScreen() {
  resetScreen();

  tft.setCursor(0,130);
  tft.setFont(&FreeSansBold12pt7b);
  tft.println("           How much pain are you in?");

  showPLButtons();
  showBackButton();
  int answer = getPainLevelAnswer();

  if (answer == 9) {
    //back
    return;
  } else {
    selectDoseScreen(answer);
  }

}

//// DEVICE CONFIRMS DISPENSING ////
void dispenseConfirmedScreen(int selection) {
  resetScreen();
  tft.setTextColor(BLACK);
  tft.setCursor(0, 70);
  tft.setFont(&FreeSans12pt7b);
  tft.print("     Dispensing ");
  tft.setFont(&FreeSansBold12pt7b);
  tft.print(doses[selection]);
  tft.println(".");
  tft.println();
  tft.setFont(&FreeSans12pt7b);
  tft.println("     See you at your next check-in, in");
  tft.setFont(&FreeSansBold12pt7b);
  tft.println("     2 hours, at 12:00pm."); //// add nextCheckInTime

  // ADD SERVO HERE //

  delay(2000);
  timeToCheckIn = false;
  lockedOut = true;
  lockOutStartTime = millis();
  lastCheckInTime = rtc.getTime();

}

//// USER SELECTED "NO MEDICATION" ////
void noMedSelectedScreen() {
  resetScreen();
  showBackButton();
  showContButton();
  tft.setTextColor(BLACK);
  tft.setCursor(0, 150);
  tft.setFont(&FreeSansBold12pt7b);
  tft.println("            No medication selected.");
  tft.println("            Are you sure?");

  int answer = getConfirmDispenseAnswer();

  if (answer == 2) {
    selectDoseScreen(currDoseLevel);
  } else if (answer == 1) {
    nonCheckInScreen();
    lockedOut = false;
    lockOutStartTime = millis();
  }
}


//// LOOPING LOGIC FOR SCREENS ////
///////////////////////////////////

int getConfirmDispenseAnswer() {
  int answer;
  bool valid = false;
  while (!valid) {
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    p = mapCoords(p);
    answer = getConfirmDispenseTouchInput(p); //mapped touch input
    if (answer != 0) {
      valid = true;
      delay(300); //debounce touch
    }
  }
  return answer;
}

int getContAnswer() {
  int answer;
  bool valid = false;
  while (!valid) {
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    p = mapCoords(p);
    answer = getContTouchInput(p); //mapped touch input
    if (answer != 0) {
      valid = true;
      delay(300); //debounce touch
    }
  }
  return answer;
}

int getPainLevelAnswer() {
  int answer;
  bool valid = false;
  while (!valid) {
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    p = mapCoords(p);
    answer = getPLTouchInput(p); //mapped touch input
    if (answer != 0) {
      valid = true;
      delay(300); //debounce touch
    }
  }
  return answer;
}


int getDoseLevelAnswer() {
  int answer;
  bool valid = false;
  while (!valid) {
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    p = mapCoords(p);
    answer = getDoseTouchInput(p); //mapped touch input
    if (answer != 0) {
      valid = true;
      delay(300); //debounce touch
    }
  }
  return answer;
}

int getContTouchInput(TSPoint p) {
  if (p.z > MINPRESSURE) {
    if ( (p.y > 186) && (p.y < 305) && (p.x > 38) && (p.x < 140)) {
      Serial.println("USER TOUCHED CONTINUE");
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}


//// GET TOUCH INPUT FOR SCREENS ////
////////////////////////////////////

int getConfirmDispenseTouchInput(TSPoint p) {
  if (p.z > MINPRESSURE) {
    if ( (p.y > 186) && (p.y < 305) && (p.x > 38) && (p.x < 140)) {
      Serial.println("USER TOUCHED CONTINUE");
      return 1;
    } else if ( (p.y > 20) && (p.y < 80) && (p.x > 355) && (p.x < 435)) {
      Serial.println("USER TOUCHED BACK");
      return 2;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

int getPLTouchInput(TSPoint p) {
  if (p.z > MINPRESSURE) {
    if ( (p.x > 10) && (p.x < 180) && (p.y > 10) && (p.y < 50)) {
      Serial.println("USER TOUCHED NO 00");
      return 1;
    } else if ( (p.x > 10) && (p.x < 180) && (p.y > 60) && (p.y < 105) ) {
      Serial.println("USER TOUCHED 01");
      return 2;
    } else if ( (p.x > 10) && (p.x < 180) && (p.y > 115) && (p.y < 160) ) {
      Serial.println("USER TOUCHED 02");
      return 3;
    } else if ( (p.x > 10) && (p.x < 180) && (p.y > 165) && (p.y < 210) ) {
      Serial.println("USER TOUCHED 03");
      return 4;
    } else if ( (p.x > 10) && (p.x < 180) && (p.y > 215) && (p.y < 265) ) {
      Serial.println("USER TOUCHED 04");
      return 5;
    } else if ( (p.x > 10) && (p.x < 180) && (p.y > 270) && (p.y < 315) ) {
      Serial.println("USER TOUCHED 05");
      return 6;
    } else if ( (p.y > 20) && (p.y < 80) && (p.x > 355) && (p.x < 435)) {
      Serial.println("USER TOUCHED BACK");
      return 9; // back
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

int getDoseTouchInput(TSPoint p) {
  if (p.z > MINPRESSURE) {
    if ( (p.x > 10) && (p.x < 80) && (p.y > 185) && (p.y < 310)) {
      Serial.println("USER TOUCHED DOSE OP-2");
      return 4;
    } else if ( (p.x > 90) && (p.x < 180) && (p.y > 185) && (p.y < 310) ) {
      Serial.println("USER TOUCHED DOSE OP-1");
      return 3;
    } else if ( (p.x > 190) && (p.x < 270) && (p.y > 185) && (p.y < 310) ) {
      Serial.println("USER TOUCHED DOSE OTC-2");
      return 2;
    } else if ( (p.x > 280) && (p.x < 360) && (p.y > 185) && (p.y < 310) ) {
      Serial.println("USER TOUCHED DOSE OTC-1");
      return 1;
    } else if ( (p.x > 370) && (p.x < 460) && (p.y > 185) && (p.y < 310) ) {
      Serial.println("USER TOUCHED DOSE-NONE");
      return 5;
    } else if ( (p.y > 20) && (p.y < 80) && (p.x > 355) && (p.x < 435)) {
      Serial.println("USER TOUCHED BACK");
      return 9;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

//// GLOBAL MATH FOR CONVERTING TOUCH INTO COORDS ////
TSPoint mapCoords(TSPoint p) {
  p.x = p.x + p.y;
  p.y = p.x - p.y;
  p.x = p.x - p.y;
  p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
  p.y = tft.height()-(map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
  return p;
}

//// RESET THE SCREEN WITH ALL WHITE ////
void resetScreen() {
  tft.fillScreen(WHITE);
  tft.setTextColor(BLACK);
  tft.setTextSize(1);
}

//// CONVERT INTEGER DOSE LEVEL TO DOSE STRING, INCLUDES NONE ////
String getLastDose(int doseLevel) {
  if (doseLevel == 4) {
    return "none";
  } else {
    return doses[doseLevel];
  }
}

//// COMPARE TIMES ////
int minutesBetweenTimes(Time curr, Time CItime) {
  int diffHour = curr.hour - CItime.hour;
  if (curr.Hour < CItime.hour) {
    diffHour = curr.hour + 24 - CItime.hour;
  }
  if (diffHour == 0) {
    return curr.min - CItime.min;
  } else if (diffHour == 1) {
    return curr.min + (60-CItime.min);
  } else {
    return ((diffHour - 1)*60) + curr.min + (60-CItime.min);
  }
}

//// A DEBUG METHOD FOR FIGURING OUT TOUCH COORDS ////
void debug() {
  TSPoint p = ts.getPoint();
  p = mapCoords(p);
  Serial.print("("); Serial.print(p.x);
  Serial.print(", "); Serial.print(p.y);
  Serial.println(")");
}


//// DRAW ON SCREEN METHODS ////

void showYNButtons() {
  tft.fillRoundRect(10, 190, 220, 120, 10, BLUE);
  tft.fillRoundRect(250, 190, 220, 120, 10, BLUE);
  tft.setTextColor(BLACK);
  tft.setCursor(20, 220);
  tft.print("YES");
  tft.setCursor(258, 220);
  tft.print("NO");
}


void showContButton() {
  tft.fillRoundRect(270, 215, 185, 74, 10, 0x6E72);
  tft.setTextColor(WHITE);
  tft.setCursor(310, 257);
  tft.setFont(&FreeSansBold12pt7b);
  tft.print("Continue");
}

void showDispenseButton() {
  tft.fillRoundRect(270, 215, 185, 74, 10, 0x6E72);
  tft.setTextColor(WHITE);
  tft.setCursor(310, 255);
  tft.setFont(&FreeSansBold12pt7b);
  tft.print("Dispense");
}


void showOverrideButton(int a) {
  if (a == 0) {
    tft.fillRoundRect(270, 215, 185, 74, 10, 0xF649); //yellow
  } else {
    tft.fillRoundRect(270, 215, 185, 74, 10, 0xEAAA); // red
  }

  tft.setTextColor(WHITE);
  tft.setCursor(310, 255);
  tft.setFont(&FreeSansBold12pt7b);
  tft.print("Override");
}

void showBackButton() {
  tft.setFont(&FreeSans12pt7b);
  tft.fillRoundRect(26, 20, 94, 62, 10, 0xF649);
  tft.setTextColor(WHITE);
  tft.setCursor(50, 57);
  tft.print("back");
}

void showPLButtons() {
  tft.fillRoundRect(10, 177, 70, 135, 10, 0x6E72);
  tft.fillRoundRect(88, 177, 70, 135, 10, 0x8F15);
  tft.fillRoundRect(166, 177, 70, 135, 10, 0x9F34);
  tft.fillRoundRect(244, 177, 70, 135, 10, 0xDED0);
  tft.fillRoundRect(322, 177, 70, 135, 10, 0xED8E);
  tft.fillRoundRect(400, 177, 70, 135, 10, 0xEAAA);

  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(WHITE);
  tft.setCursor(21, 210); tft.print("No"); tft.setCursor(21, 232); tft.print("Hurt");
  tft.setCursor(95, 210); tft.print("Hurts"); tft.setCursor(95, 232); tft.print("Little"); tft.setCursor(95, 254); tft.print("Bit");
  tft.setCursor(173, 210); tft.print("Hurts"); tft.setCursor(173, 232); tft.print("Little"); tft.setCursor(173, 254); tft.print("More");
  tft.setCursor(251, 210); tft.print("Hurts"); tft.setCursor(251, 232); tft.print("Even"); tft.setCursor(251, 254); tft.print("More");
  tft.setCursor(325, 210); tft.print("Hurts"); tft.setCursor(325, 232); tft.print("Whole"); tft.setCursor(325, 254); tft.print("Lot");
  tft.setCursor(407, 210); tft.print("Hurts"); tft.setCursor(407, 232); tft.print("Most");
}

void showDoseButtons() {
  tft.fillRoundRect(277, 16, 185, 55, 10, 0x2CFB);
  tft.fillRoundRect(277, 76, 185, 55, 10, 0x2CFB);
  tft.fillRoundRect(277, 136, 185, 55, 10, 0x2CFB);
  tft.fillRoundRect(277, 196, 185, 55, 10, 0x2CFB);
  tft.fillRoundRect(277, 256, 185, 55, 10, 0x2CFB);

  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(WHITE);
  tft.setCursor(290, 50); tft.print("none");
  tft.setCursor(290, 110); tft.print(doses[0]);
  tft.setCursor(290, 170); tft.print(doses[1]);
  tft.setCursor(290, 230); tft.print(doses[2]);
  tft.setCursor(290, 290); tft.print(doses[3]);
}
