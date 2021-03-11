#include <EEPROM.h>
#include <DHT.h>
#include <DHT_U.h>
#include<JC_Button.h>
#include<LiquidCrystal.h>
#include <arduino-timer.h>
//#include  <virtuabotixRTC.h>

#define nhietDoMin 16
#define nhietDoMax 22
#define doAmMin 65
#define doAmMax 99
#define anhSangMin 600
#define anhSangMax 1100
#define nhietDo_delta_P 1
#define nhietDo_delta_N 1
#define doAm_delta 2
#define K_D 0.008       // He so anh sang

#define DHTTYPE DHT22
#define DHTPIN 22
#define lamLanh 24
#define taoSuong 26
#define lamNong 28
#define chieuSang 30
#define coiBaoNuoc 5
#define volumePin A0
#define sensor_Water A1
//#define hetNuoc

#define NORMAL 0
#define MAIN_MENU 1
#define TEMPERATURE_SETUP 3
#define HUMIDITY_SETUP 4
#define BRIGHTNESS_SETUP 2
#define CYCLE_SETUP 5
#define LONG_PRESS 1000

#define ADDR_FLAG 0
#define ADDR_ANHSANG 1
#define ADDR_NHIETDO 5
#define ADDR_DOAM 9
#define ADDR_CYCLE 13
#define ADDR_GIAY 14

/*
  const float anhSang_default[] = {0, 1000, 700};
  const float nhietDo_default = 19;
  const float doAm_default[] = {77.5, 77.5, 82.5};
*/
//virtuabotixRTC myRTC(2, 3, 4);
float nhietDo, doAm, nhietDo_dat(19), doAm_dat(77.5);

String mystring, mystring2, mystring3, tam, tam2, tam3, a, c, e, q, w, k, u;
char x;

int mode(NORMAL);
int giaiDoan(1);
boolean sang = true;
boolean nhanGiu;
boolean inc, dec;
uint16_t giay;

#define N_COMMAND 3
class Command {
  public:
    const String cmd[N_COMMAND] = {"setpoint"};
    const uint8_t ncmd = N_COMMAND;
    bool done = false;
    String buffer = "";
    String param;

    int read() {
      while (Serial.available() > 0) {
        char b = Serial.read();
        if (b == '\n') return -1;
        if (b == ';') {
          String temp = buffer;
          buffer = "";
          int dot = temp.indexOf('.');
          if (dot != -1) {
            String tempcmd = temp.substring(0, dot);
            for (int i = 0; i < ncmd; i++) {
              if (tempcmd == cmd[i]) {
                if (dot + 1 < temp.length()) param = temp.substring(dot + 1);
                return i;
              }
            }
          } else {
            for (int i = 0; i < ncmd; i++) {
              if (temp == cmd[i]) {
                return i;
              }
            }
          }
        } else {
          buffer += String(b);
        }
      }
      return -1;
    }
};
Command cmd;

class Clock {
  public:
    uint8_t hour, minute, day, month;
    uint16_t year;

    void set(uint8_t _hour, uint8_t _minute, uint8_t _day, uint8_t _month, uint16_t _year) {
      hour = _hour;
      minute = _minute;
      day = _day;
      month = _month;
      year = _year;
    }

    String getTimeString() {
      return form(hour) + String(":") + form(minute);
    }
    String getDateString() {
      return form(day) + String("/") + form(month) + String("/") + String(year);
    }

    String form(int x) {
      if (x < 10) {
        return String("0") + String(x);
      } else {
        return String(x);
      }
    }
};
Clock clk;


DHT dht(DHTPIN, DHTTYPE);

ToggleButton nut_ok(48), nut_len(50), nut_xuong(52);

auto timer = timer_create_default();
uintptr_t task;

typedef struct {
  String label;
  uint8_t len;
  uint8_t index;
  String list[4];
} Menu;

Menu mainMenu, tMenu;

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  pinMode(lamLanh, OUTPUT);
  pinMode(taoSuong, OUTPUT);
  pinMode(coiBaoNuoc, OUTPUT);
  pinMode(chieuSang, OUTPUT);

  off(lamLanh);
  off(taoSuong);
  /*
    nut_anhSang.begin();
    nut_nhietDo.begin();
    nut_doAm.begin(); */
  nut_len.begin();
  nut_xuong.begin();
  nut_ok.begin();
  lcd.begin(20, 4);
  dht.begin();

  mainMenu.list[0] = " Cai dat chieu sang  ";
  mainMenu.list[1] = " Cai dat nhiet do  ";
  mainMenu.list[2] = " Cai dat do am  ";
  mainMenu.list[3] = " Thoat  ";
  mainMenu.len = 4;
  tMenu.label = "  CHE DO CHIEU SANG";
  tMenu.list[0] = "Khong chieu sang";
  tMenu.list[1] = "Sang 12 tieng/ngay";
  tMenu.len = 2;
  readFromEEPROM();

  timer.every(1000, SendData);

  // myRTC.setDS1302Time(25, 07, 13, 6, 27, 1, 2021); // sec/min/h/weekdays_dd/mm/yyyy
  // nap cuong trinh xong tha'o Module ra => xoa dong lenh myRTC.set nạp lại chương trình
}

void loop() {
  //  kiemTraSuCo();
  timer.tick();
  nutNhan();
  AnhSang();
  nhietDo_doAm();
  hienThi();
  SS_water ();
  //  rtct();
  // myRTC.updateTime();

}

void nutNhan() {
  /*
    nut_anhSang.read();
    if (nut_anhSang.changed()) {
    mode = mode == 1 ? 0 : 1;
    lcd.clear();
    return;
    }
    nut_nhietDo.read();
    if (nut_nhietDo.changed()) {
    mode = mode == 2 ? 0 : 2;
    lcd.clear();
    return;
    }
    nut_doAm.read();
    if (nut_doAm.changed()) {
    mode = mode == 3 ? 0 : 3;
    lcd.clear();
    return;
    }
  */
  nut_ok.read();
  boolean ok = nut_ok.changed();
  nut_len.read();
  nut_xuong.read();
  switch (mode) {
    case NORMAL:
      if (ok) mode = MAIN_MENU;
      break;
    case MAIN_MENU:
      if (nut_xuong.changed()) {
        if (mainMenu.index == mainMenu.len - 1) mainMenu.index = 0;
        else mainMenu.index++;
      } else if (nut_len.changed()) {
        if (mainMenu.index == 0) mainMenu.index = mainMenu.len - 1;
        else mainMenu.index--;
      } else if (ok) {
        switch (mainMenu.index) {
          case 3:
            mode = NORMAL;
            mainMenu.index = 0;
            break;
          case 0:
            mode = CYCLE_SETUP;
            break;
          default:
            mode = mainMenu.index + 2;
        }
      }
      break;
    case CYCLE_SETUP:
      if (nut_xuong.changed()) {
        if (tMenu.index == tMenu.len - 1) tMenu.index = 0;
        else tMenu.index++;
      } else if (nut_len.changed()) {
        if (tMenu.index == 0) tMenu.index = tMenu.len - 1;
        else tMenu.index--;
      } else if (ok) {
        if (tMenu.index == 0) {
          mode = MAIN_MENU;
          writeToEEPROM();
        } else mode = BRIGHTNESS_SETUP;
      }
      break;

    default:
      if (!nhanGiu) {
        inc = nut_len.pressedFor(LONG_PRESS);
        dec = nut_xuong.pressedFor(LONG_PRESS);
        if (inc || dec) {
          nhanGiu = true;
          task = timer.every(150, changeValue);
        } else {
          inc = nut_len.changed();
          dec = nut_xuong.changed();
          if (inc || dec) {
            int temp = inc ? 1 : -1;
            changeValue();
          }
        }
      } else {
        if (nut_len.wasReleased() || nut_xuong.wasReleased()) {
          timer.cancel(task);
          nhanGiu = false;
        }
      }
      if (ok) {
        mode = MAIN_MENU;
        writeToEEPROM();
      }
      break;
  }
  if (ok) lcd.clear();
}

void AnhSang() {
  if (clk.hour < 06 || clk.hour > 18) sang = 0;
  else sang = 1;

  if (sang && tMenu.index == 1) {
    //int AD = K_D * anhSang_dat;
    on(chieuSang);
    //anhSang = anhSang_dat;
  } else {
    off(chieuSang);
    //anhSang = 0;
  }


}



void nhietDo_doAm() {
  nhietDo = dht.readTemperature();
  doAm = dht.readHumidity();
  if (nhietDo > nhietDo_dat + nhietDo_delta_P) on(lamLanh);
  if (nhietDo <= nhietDo_dat) off(lamLanh);

  if (nhietDo >= nhietDo_dat) off(lamNong);
  if (nhietDo < nhietDo_dat - nhietDo_delta_N) on(lamNong);

  if (doAm > doAm_dat + doAm_delta) off(taoSuong);
  if (doAm < doAm_dat - doAm_delta) on(taoSuong);
}

//void kiemTraSuCo() {
//  if (digitalRead(hetNuoc)) suCoHetNuoc();
//  digitalWrite(coiBaoNuoc, digitalRead(hetNuoc));
//
//
//}

void suCoHetNuoc() {

}

void hienThi() {
  switch (mode) {
    case NORMAL:
      lcd.setCursor(0, 0);
      lcd.print("   CAC THONG SO");

      lcd.setCursor(0, 1);
      lcd.print("Anh sang: ");
      lcd.print(anhSang, 0);
      lcd.print(" LUX   ");

      lcd.setCursor(0, 2);
      lcd.print("Nhiet do: ");
      lcd.print(nhietDo, 0);
      lcd.print(" \xDF");
      lcd.print('C');
      lcd.print('/');
      lcd.print(nhietDo_dat, 0);
      lcd.print(" \xDF");
      lcd.print("C     ");

      lcd.setCursor(0, 3);
      lcd.print("Do am   : ");
      lcd.print(doAm, 0);
      lcd.print("%");
      lcd.print('/');
      lcd.print(doAm_dat, 0);
      lcd.print("%     ");
      break;
    case MAIN_MENU:
      int i;
      for (i = 0; i < mainMenu.len; i++) {
        lcd.setCursor(0, i);
        if (mainMenu.index == i) lcd.print('\x7E');
        lcd.print(mainMenu.list[i]);
      }
      break;
    case CYCLE_SETUP:
      lcd.setCursor(0, 0);
      lcd.print(tMenu.label);
      switch (tMenu.index) {
        case 0:
          lcd.setCursor(1, 1);
          lcd.print('\x7E');
          lcd.print(tMenu.list[0]);
          lcd.print('\x7F');
          lcd.setCursor(0, 2);
          lcd.print(" ");
          lcd.print(tMenu.list[1]);
          lcd.print("   ");
          break;
        case 1:
          lcd.setCursor(1, 1);
          lcd.print(" ");
          lcd.print(tMenu.list[0]);
          lcd.print("  ");
          lcd.setCursor(0, 2);
          lcd.print('\x7E');
          lcd.print(tMenu.list[1]);
          lcd.print('\x7F');
          break;
      }
      break;
    case BRIGHTNESS_SETUP:
      lcd.setCursor(0, 0);
      lcd.print("  Cai dat do sang:");
      lcd.setCursor(0, 2);
      lcd.print("      ");
      lcd.print(anhSang_dat, 0);
      lcd.print(" LUX    ");
      break;
    case TEMPERATURE_SETUP:
      lcd.setCursor(0, 0);
      lcd.print(" Cai dat nhiet do:");
      lcd.setCursor(0, 2);
      lcd.print("       ");
      lcd.print(nhietDo_dat, 0);
      lcd.print(" \xDF");
      lcd.print("C   ");
      break;
    case HUMIDITY_SETUP:
      lcd.setCursor(0, 0);
      lcd.print("   Cai dat do am:");
      lcd.setCursor(0, 2);
      lcd.print("        ");
      lcd.print(doAm_dat, 0);
      lcd.print(" %   ");
      break;
  }
}

void on(int pin) {
  digitalWrite(pin, false);
}

void off(int pin) {
  digitalWrite(pin, true);
}

void limit(float * x, float xmin, float xmax) {
  if (*x > xmax) *x = xmax;
  else if (*x < xmin) *x = xmin;
}


bool changeValue(void *) {
  int temp = inc ? 1 : -1;
  switch (mainMenu.index) {
    case 0:
      temp = inc ? 10 : -10;
      anhSang_dat += temp;
      limit(&anhSang_dat, anhSangMin, anhSangMax);
      break;
    case 1:
      nhietDo_dat += temp;
      limit(&nhietDo_dat, nhietDoMin, nhietDoMax);
      break;
    case 2:
      doAm_dat += temp;
      limit(&doAm_dat, doAmMin, doAmMax);
      break;
  }
  return true;
}

bool changeValue() {
  int temp = inc ? 1 : -1;
  switch (mainMenu.index) {
    case 0:
      temp = inc ? 10 : -10;
      anhSang_dat += temp;
      limit(&anhSang_dat, anhSangMin, anhSangMax);
      break;
    case 1:
      nhietDo_dat += temp;
      limit(&nhietDo_dat, nhietDoMin, nhietDoMax);
      break;
    case 2:
      doAm_dat += temp;
      limit(&doAm_dat, doAmMin, doAmMax);
      break;
  }
  return true;
}
/*
  #define ADDR_FLAG 0
  #define ADDR_ANHSANG 1
  #define ADDR_NHIETDO 5
  #define ADDR_DOAM 9
  #define ADDR_CYCLE 13
  #define ADDR_GIAY 14
*/
void readFromEEPROM() {
  if (EEPROM.read(ADDR_FLAG)) {  // co du lieu da duoc ghi
    EEPROM.get(ADDR_ANHSANG, anhSang_dat);
    EEPROM.get(ADDR_NHIETDO, nhietDo_dat);
    EEPROM.get(ADDR_DOAM, doAm_dat);
    tMenu.index = EEPROM.read(ADDR_CYCLE);
    EEPROM.get(ADDR_GIAY, giay);
  } else writeToEEPROM();
}

void writeToEEPROM() {
  EEPROM.write(ADDR_FLAG, 1);
  EEPROM.put(ADDR_ANHSANG, anhSang_dat);
  EEPROM.put(ADDR_NHIETDO, nhietDo_dat);
  EEPROM.put(ADDR_DOAM, doAm_dat);
  EEPROM.write(ADDR_CYCLE, tMenu.index);
  EEPROM.put(ADDR_GIAY, giay);
}
/*void rtct()
  {
  Serial.print(myRTC.dayofmonth);             //You can switch between day and month if you're using American system
  Serial.print("/");
  Serial.print(myRTC.month);
  Serial.print("/");
  Serial.print(myRTC.year);
  Serial.print(" ");
  Serial.print(myRTC.hours);
  Serial.print(":");
  Serial.print(myRTC.minutes);
  Serial.print(":");
  Serial.println(myRTC.seconds);
  }*/
void SS_water () {
  int reading = analogRead(sensor_Water);
  if (reading < 300) {
    int cb1 = 0 ;
    digitalWrite(coiBaoNuoc, HIGH);
  }
  else {
    digitalWrite(coiBaoNuoc, LOW);
    int cb1 = 1 ;
  }
}
bool SendData(void*) { //  <info>.<t,h,as>;
  Serial1.print(String("info.") + String(nhietDo) + String(",") + String(doAm) + String(",") + sang ? 1 : 0 + String(";"));
  return true;
}
void receiveData() {

}