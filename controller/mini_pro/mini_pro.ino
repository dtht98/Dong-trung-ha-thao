#include <EEPROM.h>
#include <DHT.h>
#include <DHT_U.h>
#include <arduino-timer.h>

#define nhietDoMin 16
#define nhietDoMax 22
#define doAmMin 65
#define doAmMax 99
#define anhSangMin 600
#define anhSangMax 1100
#define nhietDo_delta_P 1
#define nhietDo_delta_N 1
#define doAm_delta 2

#define DHTTYPE DHT22
#define DHTPIN 4
#define sensor_Water A0

#define lamLanh 5
#define taoSuong 6
#define lamNong 11
#define chieuSang 12
#define coiBaoNuoc 13

#define ADDR_FLAG 0
#define ADDR_ANHSANG 1
#define ADDR_NHIETDO 5
#define ADDR_DOAM 9
#define ADDR_CYCLE 13
#define ADDR_GIAY 14

float nhietDo = 0.0, doAm = 0.0, nhietDo_dat = 19.0, doAm_dat = 77.5;

boolean sang = true, outOfWater = false;

#define N_COMMAND 1
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

DHT dht(DHTPIN, DHTTYPE);

auto timer = timer_create_default();
uintptr_t task;

void setup() {
  Serial.begin(9600);
  Serial.print("flush;");
  pinMode(13, OUTPUT);
  pinMode(lamLanh, OUTPUT);
  pinMode(taoSuong, OUTPUT);
  pinMode(coiBaoNuoc, OUTPUT);
  pinMode(chieuSang, OUTPUT);

  off(lamLanh);
  off(taoSuong);

  readFromEEPROM();

  timer.every(1000, SendData);
}

void loop() {
  //  kiemTraSuCo();
  timer.tick();
  nhietDo_doAm();
  SS_water();
  anhSang();
  serial();
}

void anhSang() {
  if (sang) {
    on(chieuSang);
  } else {
    off(chieuSang);
  }
}

void nhietDo_doAm() {
  nhietDo = 10;//dht.readTemperature();
  doAm = 40;//dht.readHumidity();
  if (nhietDo > nhietDo_dat + nhietDo_delta_P) on(lamLanh);
  if (nhietDo <= nhietDo_dat) off(lamLanh);

  if (nhietDo >= nhietDo_dat) off(lamNong);
  if (nhietDo < nhietDo_dat - nhietDo_delta_N) on(lamNong);

  if (doAm > doAm_dat + doAm_delta) off(taoSuong);
  if (doAm < doAm_dat - doAm_delta) on(taoSuong);
}

void on(int pin) {
  digitalWrite(pin, true);
}

void off(int pin) {
  digitalWrite(pin, false);
}

void limit(float * x, float xmin, float xmax) {
  if (*x > xmax) *x = xmax;
  else if (*x < xmin) *x = xmin;
}

void readFromEEPROM() {
  if (EEPROM.read(ADDR_FLAG)) {  // co du lieu da duoc ghi
    EEPROM.get(ADDR_ANHSANG, sang);
    EEPROM.get(ADDR_NHIETDO, nhietDo_dat);
    EEPROM.get(ADDR_DOAM, doAm_dat);
  } else writeToEEPROM();
}

void writeToEEPROM() {
  EEPROM.write(ADDR_FLAG, 1);
  EEPROM.put(ADDR_ANHSANG, sang);
  EEPROM.put(ADDR_NHIETDO, nhietDo_dat);
  EEPROM.put(ADDR_DOAM, doAm_dat);
}

void SS_water () {
  int reading = analogRead(sensor_Water);
  if (reading < 300) {
    outOfWater = true;
    digitalWrite(coiBaoNuoc, HIGH);

  }
  else {
    outOfWater = false;
    digitalWrite(coiBaoNuoc, LOW);
  }
}

bool SendData(void*) { //  <info>.<t,h,as>;
  Serial.print(String("info.") + String(nhietDo) + String(",") + String(doAm) + String(",") + (sang ? "1" : "0") + ',' + String(nhietDo_dat) + ',' + String(doAm_dat) + String(";"));
  Serial.print(String("water.") + (outOfWater ? "0" : "1") + String(";"));
  //   Serial.print(String("water.") + (outOfWater ? "0" : "1") + String(";"));
  return true;
}

void serial() {                //  truyen thong
  switch (cmd.read()) {
    case 0: {  //setpoint
        String s = cmd.param;
        int sp1 = s.indexOf(',');
        String tsp = s.substring(0, sp);  //temperature
        int sp2 = s.indexOf(',', sp1 + 1);
        String hsp = s.substring(sp1 + 1, sp2); //humidity
        char lsp = s.charAt(s.length() - 1);
        
        nhietDo_dat = tsp.toFloat();
        doAm_dat = hsp.toFloat();
        sang = lsp == '1'? true: false;
        writeToEEPROM();
      }
      break;
  }
}
