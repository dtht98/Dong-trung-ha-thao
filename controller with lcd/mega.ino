#include <EEPROM.h>
#include <DHT.h>
#include <DHT_U.h>
#include<JC_Button.h>
#include<LiquidCrystal.h>
#include <arduino-timer.h>
#include <timer-api.h>
#include <timer_setup.h>

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
#define chieuSang2 32  /*chieu sang led 2*/
//#define coiBaoNuoc 25
#define volumePin A0
#define sensor_Water A1
#define hetNuoc 5

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

float anhSang, nhietDo, doAm, nhietDo_dat(19), doAm_dat(77.5), anhSang_dat(1000);;

String mystring,mystring2,mystring3, tam, tam2, tam3, a, c, e, q, w,k,u;
char x;

int mode(NORMAL);
int giaiDoan(1);
boolean sang = true;
boolean nhanGiu;
boolean inc, dec;
uint16_t giay;

LiquidCrystal lcd(23, 25, 27, 29, 31, 33);
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
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  pinMode(lamLanh, OUTPUT);
  pinMode(taoSuong, OUTPUT);
  pinMode(hetNuoc, OUTPUT);
  pinMode(chieuSang, OUTPUT);
  pinMode(chieuSang2, OUTPUT); //////////////////////////////////

  off(lamLanh);
  off(taoSuong);                                                                                                                      

  /*timer_init_ISR_1Hz(TIMER_DEFAULT);
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
}

void loop() {
    //  kiemTraSuCo();
  timer.tick();
  nutNhan();
  AnhSang();
  nhietDo_doAm();
  hienThi();
  SS_water ();
  SendData();
  receiveData();
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
  if (sang && tMenu.index == 1) {
    //int AD = K_D * anhSang_dat;
    on(chieuSang);
    on(chieuSang2);
    //anhSang = anhSang_dat;
  } else {
    off(chieuSang);
    off(chieuSang2);
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
void SS_water (){
  int reading = analogRead(sensor_Water); 
  if (reading < 300) {
    int cb1 = 0 ;
    digitalWrite(hetNuoc, HIGH);
     Serial.print(cb1);
     Serial.print('j');}
  else {
    digitalWrite(hetNuoc, LOW);
    int cb1 = 1 ;
     Serial.print(cb1);
     Serial.print('j');}
}
void SendData(){

//  Serial1.print(nhietDo,0);
//  Serial1.print('t');
//  Serial1.print(doAm,0);
//  Serial1.print('h');
//  Serial1.print(anhSang,0);
//  Serial1.print('m');
//  Serial1.print(hetNuoc,0);
//  Serial1.print('j');
  }
void receiveData(){
//    if (Serial1.available()>0){
//    x = Serial1.read();
// //   Serial.print(x);
//    
//    if (x != 'b'|| x!= 'd' ||x != 'f') {
//      tam.concat(x);
//      tam2.concat(x);
//      tam3.concat(x);
//    }
//    if(x == 'b'){
//        mystring = tam; 
//        tam = "";
//        tam2 = "";
//        tam3 = "";
//        int q = mystring.indexOf('b', 0);
//        a = mystring.substring(0, mystring.length());
//        nhietDo_dat = a.toInt();
//    //    Serial.print(nhietDo_dat,0);
//  }
//    if(x == 'd'){
//        mystring2 = tam2; 
//        tam2 = "";
//        tam = "";
//        tam3 = "";
//        int w = mystring2.indexOf('d', 0);
//        k = mystring2.substring(0, mystring2.length());
//        doAm_dat = k.toInt();
//   //     Serial.print(doAm_dat,0);
//  }
//    if(x == 'f'){
//        mystring3 = tam3;
//        tam = "";
//        tam2 = "";
//        tam3 = "";
//        int r = mystring3.indexOf('f', 0);
//        u = mystring3.substring(0, mystring3.length());
//        anhSang_dat = u.toInt();
//  //      Serial.print(anhSang_dat,0);
//  }
// }
}
