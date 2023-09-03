/*
    Alarm Clock v2.0

    Basic alarm clock code.

    The circuit:
    * Arduino Nano
    * RTC DS3231 with I2C interface (SCL at pin A5 and SDA at pin A4)
    * OLED monochrome display with I2C interface (SCL at pin A5 and SDA at pin A4)
    * Buzzer at pin D2
    * 3 switches at pin D4, D5 and D6 (When pressed GND connected)
    * 9V battery connector at its VIN pin

    Created 20/8/2023
    By Alejandro Ortega Cadahía
    Modified 3/9/2023
    By Alejandro Ortega Cadahía

    Author's GitHub: https://github.com/alex-ortega-07

*/


#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts\FreeMono9pt7b.h>

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 32;
const int OLED_RESET = 4 ;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;

const int BT_UP = 4;
const int BT_MID = 5;
const int BT_DOWN = 6;
const int BTS [] {BT_UP, BT_MID, BT_DOWN};
const int BUZZER = 2;

const String weekDays [] {"Sun.", "Mon.", "Tues.", "Weds.", "Thurs.", "Fri.", "Sat."};

int btPressed = -1;

int screenPage = 0;

int principalScreen = 0;
int numPrincipalScreens = 2;

const String listMenu[] {"Back", "Set hour", "Set date", "Set alarm"};
int listMenuArrowMult = 0;
int listMenuScroll = 0;

const int numberAlarms = 2;

String alarms[numberAlarms][3];

const String listAlarmMenu[numberAlarms + 1];

int listAlarmMenuArrowMult = 0;
int listAlarmMenuScroll = 0;
bool alarmStatusFlag = false;
bool activeAlarm = false;
int alarmCheck;
int numAlarm;
int alarmH;
int alarmM;

unsigned long currentMillis;
unsigned long previousMillis = 0;
const long intervalDim = 30000;
bool displayDim = false;

int hour, min, seg, year, month, day;

int arrowMult = 0;
bool saveFlag = false;

void setup() {
  Serial.begin(9600);

  for(int i = 0; i < sizeof(BTS) / sizeof(BTS[0]); ++i)
    pinMode(BTS[i], INPUT_PULLUP);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.cp437(true);
  display.clearDisplay();

  rtc.begin();
  rtc.adjust(DateTime(2023, 0, 0, 0, 0, 0));

  String alarmParameters[] {"00", "00", "Off"};
  for(int i = 0; i < numberAlarms; ++i){
    for(int j = 0; j < sizeof(alarms[i]) / sizeof(alarms[i][0]); ++j)
      alarms[i][j] = alarmParameters[j];
  }

  listAlarmMenu[0] = "Back";
  for(int i = 1; i < sizeof(listAlarmMenu) / sizeof(listAlarmMenu[0]); ++i)
    listAlarmMenu[i] = alarmConcat(alarms[i - 1]);
}

void loop() {
  display.clearDisplay();

  DateTime date = rtc.now();
  btPressed = pressedBTS();
  alarmCheck = checkAlarms(date);

  if(alarmCheck > -1 || activeAlarm){
    alarmTriggered(date);

    if(btPressed > -1)
      activeAlarm = false;
  }

  else{
    if(screenPage == 0){
      showPrincipalMenu(date, principalScreen);
      
      if(BTS[btPressed] == BT_UP)
        principalScreen++;

      else if(BTS[btPressed] == BT_MID){
        screenPage = 1;
        principalScreen = 0;

        hour = date.hour();
        min = date.minute();
        seg = date.second();
        year = date.year();
        month = date.month();
        day = date.day();
      }

      else if(BTS[btPressed] == BT_DOWN)
        principalScreen--;

      checkOptions(principalScreen, numPrincipalScreens);
    }

    else if(screenPage == 1){
      showListMenu(listMenu, sizeof(listMenu) / sizeof(listMenu[0]), listMenuArrowMult, listMenuScroll);
      
      if(BTS[btPressed] == BT_UP){
        if(listMenuArrowMult == 0 && listMenuScroll > 0)
          listMenuScroll--;

        listMenuArrowMult = 0;
      }

      else if(BTS[btPressed] == BT_MID){
        if(listMenu[listMenuArrowMult + listMenuScroll] == "Back")
          screenPage = 0;
          
        if(listMenu[listMenuArrowMult + listMenuScroll] == "Set hour")
          screenPage = 2;

        else if(listMenu[listMenuArrowMult + listMenuScroll] == "Set date")
          screenPage = 3;

        else if(listMenu[listMenuArrowMult + listMenuScroll] == "Set alarm")
          screenPage = 4;
      }

      else if(BTS[btPressed] == BT_DOWN){
        if(listMenuArrowMult == 1 && listMenuScroll < sizeof(listMenu) / sizeof(listMenu[0]) - 2)
          listMenuScroll++;

        listMenuArrowMult = 1;
      }
    }

    else if(screenPage == 2){
      if(arrowMult > 2)
        showSaveChanges(saveFlag);

      else{
        showSetHour(hour, min, seg);
        showUpArrow(87 - 33 * arrowMult, 8);
        showDownArrow(87 - 33 * arrowMult, 25);
      }

      if(BTS[btPressed] == BT_UP){
        if(arrowMult == 0)
          seg++;

        else if(arrowMult == 1)
          min++;

        else if(arrowMult == 2)
          hour++;

        else if(arrowMult > 2)
          saveFlag = !saveFlag;

        checkBoundaries(hour, min, seg);
      }

      else if(BTS[btPressed] == BT_MID)
        if(arrowMult > 2){
          if(saveFlag)
            rtc.adjust(DateTime(date.year(), date.month(), date.day(), hour, min, seg));
            
          screenPage = 1;
          saveFlag = false;
          arrowMult = 0;
        }

        else
          arrowMult++;

      else if(BTS[btPressed] == BT_DOWN){
        if(arrowMult == 0)
          seg--;

        else if(arrowMult == 1)
          min--;

        else if(arrowMult == 2)
          hour--;

        else if(arrowMult > 2)
          saveFlag = !saveFlag;

        checkBoundaries(hour, min, seg);
      }
    }

    else if(screenPage == 3){
      if(arrowMult > 2)
        showSaveChanges(saveFlag);

      else{
        showSetDate(year, month, day);
        showUpArrow(80 - 33 * arrowMult, 8);
        showDownArrow(80 - 33 * arrowMult, 25);
      }

      if(BTS[btPressed] == BT_UP){
        if(arrowMult == 0)
          year++;

        else if(arrowMult == 1)
          month++;

        else if(arrowMult == 2)
          day++;

        else if(arrowMult > 2)
          saveFlag = !saveFlag;
      }

      else if(BTS[btPressed] == BT_MID){
        if(arrowMult > 2){
          if(saveFlag)
            rtc.adjust(DateTime(year, month, day, date.hour(), date.minute(), date.second()));
            
          screenPage = 1;
          saveFlag = false;
          arrowMult = 0;
        }

        else
          arrowMult++;
      }

      else if(BTS[btPressed] == BT_DOWN){
        if(arrowMult == 0)
          year--;

        else if(arrowMult == 1)
          month--;

        else if(arrowMult == 2)
          day--;

        else if(arrowMult > 2)
          saveFlag = !saveFlag;
      }
    }

    else if(screenPage == 4){
      showListMenu(listAlarmMenu, sizeof(listAlarmMenu) / sizeof(listAlarmMenu[0]), listAlarmMenuArrowMult, listAlarmMenuScroll);

      if(BTS[btPressed] == BT_UP){
        if(listAlarmMenuArrowMult == 0 && listAlarmMenuScroll > 0)
          listAlarmMenuScroll--;

        listAlarmMenuArrowMult = 0;
      }

      else if(BTS[btPressed] == BT_MID){
        if(listAlarmMenu[listAlarmMenuArrowMult + listAlarmMenuScroll] == "Back")
          screenPage = 1;

        else if(listAlarmMenuArrowMult + listAlarmMenuScroll > 0){
          screenPage = 5;
          numAlarm = listAlarmMenuArrowMult + listAlarmMenuScroll - 1;
          alarmH = alarms[numAlarm][0].toInt();
          alarmM = alarms[numAlarm][1].toInt();
        }
      }

      else if(BTS[btPressed] == BT_DOWN){
        if(listAlarmMenuArrowMult == 1 && listAlarmMenuScroll < sizeof(listAlarmMenu) / sizeof(listAlarmMenu[0]) - 2)
          listAlarmMenuScroll++;

        listAlarmMenuArrowMult = 1;
      }
    }

    else if(screenPage == 5){
      if(arrowMult > 2)
        showSaveChanges(saveFlag);

      else if(arrowMult == 2)
        showSetAlarmStatus(alarmStatusFlag);

      else{
        showSetAlarm(alarmH, alarmM);
        showUpArrow(74 - 33 * arrowMult, 8);
        showDownArrow(74 - 33 * arrowMult, 25);
      }

      if(BTS[btPressed] == BT_UP){
        if(arrowMult == 0)
          alarmM++;

        else if(arrowMult == 1)
          alarmH++;

        else if(arrowMult == 2)
          alarmStatusFlag = !alarmStatusFlag;

        else if(arrowMult > 2)
          saveFlag = !saveFlag;

        checkBoundaries(alarmH, alarmM);
      }

      else if(BTS[btPressed] == BT_MID){
        if(arrowMult > 2){
          if(saveFlag){
            alarms[numAlarm][0] = digf(alarmH);
            alarms[numAlarm][1] = digf(alarmM);

            if(alarmStatusFlag)
              alarms[numAlarm][2] = "On";

            else
              alarms[numAlarm][2] = "Off";

            updateListAlarm();
          }

          screenPage = 4;
          arrowMult = 0;
          saveFlag = false;
          alarmStatusFlag = false;
        }

        else
          arrowMult++;
      }

      else if(BTS[btPressed] == BT_DOWN){
        if(arrowMult == 0)
          alarmM--;

        else if(arrowMult == 1)
          alarmH--;

        else if(arrowMult == 2)
          alarmStatusFlag = !alarmStatusFlag;

        else if(arrowMult > 2)
          saveFlag = !saveFlag;

        checkBoundaries(alarmH, alarmM);
      }
    }
  }

  currentMillis = millis();
  if(currentMillis - previousMillis >= intervalDim)
    setDim(true);

  if(btPressed > -1)
    setDim(false);

  digitalWrite(BUZZER, activeAlarm);

  display.dim(displayDim);
  display.display();
}

int pressedBTS(){
  for(int i = 0; i < sizeof(BTS) / sizeof(BTS[0]); ++i){
    if(digitalRead(BTS[i]) == LOW){
      while(digitalRead(BTS[i]) == LOW){}
      return i;
    }
  }

  return -1;
}

void showStatusBar(DateTime &date, int screen){
  display.setTextColor(SSD1306_WHITE);
  display.setFont();
  display.setCursor(0, 0);
  display.setTextSize(1);

  if(screen > 0){
    display.print(weekDays[date.dayOfTheWeek()]);
  }

  else{
    display.print(date.day());
    display.print("/");
    display.print(date.month());
    display.print("/");
    display.print(date.year());
  }

  if(getNumAlarmsActive() > 0)
    showAlarmIcon(110, 7);
}

void showPrincipalMenu(DateTime &date, int screen){
  showStatusBar(date, screen);

  display.setTextColor(SSD1306_WHITE);
  display.setFont(&FreeMono9pt7b);
  if(screen == 0){
    display.setCursor(16, 22);
    display.setTextSize(1);
    display.print(digf(date.hour()));
    display.print(":");
    display.print(digf(date.minute()));
    display.print(":");
    display.print(digf(date.second()));
  }

  else if(screen == 1){
    display.setCursor(22, 22);
    display.setTextSize(1);
    display.print(rtc.getTemperature());
    display.setFont();
    display.setCursor(80, 12);
    display.write(248);
    display.setFont(&FreeMono9pt7b);
    display.setCursor(90, 22);
    display.print("C");
  }

  else if(screen == 2){
    display.setCursor(19, 21);
    display.print(getNumAlarmsActive());

    display.print(" Alarm");
    if(getNumAlarmsActive() != 1)
      display.print("s");
  }
}

void showListMenu(String list[], int listSize, bool arrowMult, int listScroll){
  display.setFont(&FreeMono9pt7b);
  display.setTextSize(1);
  display.setCursor(0, 12 + 15 * arrowMult);
  display.print(">");

  for(int i = 0; i < listSize; ++i){
    display.setCursor(10, 12 + 15 * (i - listScroll));
    display.print(list[i]);
  }
}

void showSetHour(int hour, int min, int seg){
  display.setFont(&FreeMono9pt7b);
  display.setCursor(16, 21);
  display.setTextSize(1);
  display.print(digf(hour));
  display.print(":");
  display.print(digf(min));
  display.print(":");
  display.print(digf(seg));
}

void showSetDate(int year, int month, int day){
  display.setFont(&FreeMono9pt7b);
  display.setCursor(9, 21);
  display.setTextSize(1);
  display.print(digf(day));
  display.print("/");
  display.print(digf(month));
  display.print("/");
  display.print(digf(year));
}

void showSaveChanges(bool save){
  display.setFont(&FreeMono9pt7b);
  display.setCursor(0, 12);
  display.setTextSize(1);
  display.print("Save");
  display.setCursor(0, 25);
  display.print("changes?");

  display.setCursor(90, 25);
  if(save){
    display.print("yes");
    return;
  }

  display.print("no");
}

void showSetAlarm(int hour, int min){
  display.setFont(&FreeMono9pt7b);
  display.setCursor(36, 21);
  display.print(digf(hour));
  display.print(":");
  display.print(digf(min));
}

void showSetAlarmStatus(int status){
  display.setFont(&FreeMono9pt7b);
  display.setCursor(0, 12);
  display.print("Turn");
  display.setCursor(0, 25);
  display.print("alarm ");
  
  if(status){
    display.print("on");
    return;
  }

  display.print("off");
}

void checkOptions(int &option, int numOptions){
  if(option > numOptions)
    option = 0;

  else if(option < 0)
    option = numOptions;
}

String digf(int num){
  String res = "";
  if(num >= 0 && num < 10)
    res = "0";

  res += num;
  return res;
}

String alarmConcat(String alarm[]){
  String res = "";
  res = alarm[0] + ":" + alarm[1] + "  " + alarm[2];
  
  return res;
}

int getNumAlarmsActive(void){
  int res = 0;

  for(int i = 0; i < sizeof(alarms) / sizeof(alarms[0]); ++i){
    if(alarms[i][2] == "On")
      res++;
  }

  return res;
}

int checkAlarms(DateTime &date){
  for(int i = 0; i < sizeof(alarms) / sizeof(alarms[0]); ++i){
    if(alarms[i][2] == "On"){
      if(alarms[i][0].toInt() == date.hour() &&
         alarms[i][1].toInt() == date.minute() &&
         date.second() == 0){
        activeAlarm = true;
        return i;
      }
    }
  }

  return -1;
}

void alarmTriggered(DateTime &date){
  display.drawRoundRect(0, 0, display.width(), display.height(), 4, SSD1306_WHITE);
  display.setCursor(18, 8);
  display.setFont();
  display.print("Alarm on");
  display.setCursor(18, 18);
  display.setFont(&FreeMono9pt7b);
  display.print(digf(date.hour()));
  display.print(":");
  display.print(digf(date.minute()));
  display.print(":");
  display.print(digf(date.second()));
  showAlarmIcon(100, 9);
}

void updateListAlarm(void){
  for(int i = 1; i < sizeof(listAlarmMenu) / sizeof(listAlarmMenu[0]); ++i)
    listAlarmMenu[i] = alarmConcat(alarms[i - 1]);
}

void setDim(bool dim){
  displayDim = dim;
  previousMillis = currentMillis;
}

void checkBoundaries(int &hour, int &min){
  if(min > 59)
    min = 0;

  if(hour > 23)
    hour = 0;

  if(min < 0)
    min = 59;

  if(hour < 0)
    hour = 23;
}

void checkBoundaries(int &hour, int &min, int &seg){
  if(seg > 59)
    seg = 0;

  if(seg < 0)
    seg = 59;

  checkBoundaries(hour, min);
}

void showAlarmIcon(int posx, int posy){
  display.drawPixel(posx, posy, SSD1306_WHITE);
  display.drawLine(posx - 3, posy - 2, posx + 3, posy - 2, SSD1306_WHITE);
  display.fillRect(posx - 2, posy - 6, 5, 4, SSD1306_WHITE);
  display.drawPixel(posx, posy - 7, SSD1306_WHITE);
}

void showUpArrow(int posx, int posy){
  display.fillTriangle(posx, posy, posx + 10, posy, posx + 5, posy - 5, SSD1306_WHITE);
}

void showDownArrow(int posx, int posy){
  display.fillTriangle(posx, posy, posx + 10, posy, posx + 5, posy + 5, SSD1306_WHITE);
}
