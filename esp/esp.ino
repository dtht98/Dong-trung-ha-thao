#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Ticker.h>
#include <BlynkSimpleEsp8266.h>
#define BLYNK_MAX_SENDBYTES 256


WiFiUDP ntpUDP;                 //giao thưc TCP
NTPClient timeClient(ntpUDP, "pool.ntp.org");
/*NTPClient n(u,"1.asia.pool.ntp.org",7*3600);// khai báo truy cập vào NTP sever khu vực ASIA, mui +7, 60*60= 3600s */
//Week Days
String weekDays[7] = {"0", "1", "2", "3", "4", "5", "6"};

//Month names
String months[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};

WidgetLED w(V6);
//char auth[] = "qJGUs1-T3mqFFpvshJwGSXHvXdGrRTQ5";
char auth[] = "vg7WD2YadZ8PkaV3wBAYHFVlwdKzRchk";

String myblynk, tam, temperature = "11", humidity = "34", j;
String waterState = "1";
bool light;
bool water_notified = false;
unsigned long last_notification = 0;

String date;

#define N_COMMAND 6
class Command {
  public:
    const String cmd[N_COMMAND] = {"scan", "connect", "disconnect", "info", "get", "water"};
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
int strength;

class {
  public:
    int order[10];
    int n;
    String password;
    bool CONNECTED = false;

    void sort(int nn) {
      int count = 0;
      for (int i = 0; i < nn; i++) {
        if (WiFi.RSSI(i) > -70) {
          int tt = count;
          order[tt] = i;
          if (tt != 0) {
            while (WiFi.RSSI(order[tt]) > WiFi.RSSI(order[tt - 1])) {
              swp(tt, tt - 1);
              tt--;
              if (tt == 0) break;
            }
          }
          count ++;
        }
      }
      n = count;
    }

    void swp(int i, int j) {
      int temp = order[i];
      order[i] = order[j];
      order[j] = temp;
    }

} wf;
/////////////////////////////////////////////////DOC THOI GIAN THUC TU SEVER//////////////////////////////////////////////////////
Ticker ticker1;
void sentdata() {
  unsigned long epochTime = timeClient.getEpochTime();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  String weekDay = weekDays[timeClient.getDay()];

  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  int currentYear = ptm->tm_year + 1900;

  //Print complete date:
  String currentDate = "time." + String(currentMinute) + ":" + String(currentHour) + "," +  String(weekDay) + "," + String(monthDay) + "/" + String(currentMonth) + "/" + String(currentYear) + ";"  ;
  if (WiFi.status() == WL_CONNECTED) Serial.print(String("strength.") + WiFi.RSSI() + ";");
  Serial.print(currentDate);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////DOC TU APP BLYNK XUONG////////////////////////////////////////////////////////////////////////

BlynkTimer timer;
BLYNK_WRITE(V3) {   //Cai dat gia tri Nhiet do//
  int pinValue = param.asInt();
  Serial.print(String("setpoint.") + String("t:") +  pinValue + String(";"));
}
BLYNK_WRITE(V4) {     //Cai dat gia tri Do am//
  int pinValue = param.asInt();
  Serial.print(String("setpoint.") +  String("h:") + pinValue + String(";"));
}
BLYNK_WRITE(V5) {      // Cai dat gia tri Anh sang//
  int pinValue = param.asInt();
  if (pinValue == 1) {
    light = true;
  } else light = false;
  Serial.print(String("setpoint.") +  String("as:") + (light ? "1" : "0") + String(";"));
}

//VIET LEN APP BLYNK//////
void myTimerEvent()
{
  //  Blynk.virtualWrite(V0, temperature);
  //  Blynk.virtualWrite(V1, humidity);
  //  Blynk.virtualWrite(V5, light);
  //  Blynk.virtualWrite(V6, waterState);
}
void setup() {
  Serial.begin(9600);
  Serial.setTimeout(100);
  Serial.print("flush;");
  // ket noi wifi
  ticker1.attach(1, sentdata);  // 1s thực hiện void sentdata
  cmd.buffer = "";
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(25200);
  //  Serial.println(WiFi.SSID() + ":" + WiFi.psk());
  //  unsigned long t1 = millis();
  //  while (WiFi.status() != WL_CONNECTED) {
  //    if (millis() - t1 > 3000) break;
  //  }
  timer.setInterval(1000L, myTimerEvent);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wf.CONNECTED) {
      wf.CONNECTED = true;
      Serial.print("connected." + WiFi.SSID() + "::" + WiFi.psk() + ";");
    }
    if (!Blynk.connected()) {
      char c_ssid[50];
      char c_pw[30];
      WiFi.SSID().toCharArray(c_ssid, WiFi.SSID().length());
      WiFi.psk().toCharArray(c_pw, WiFi.psk().length());
      Blynk.begin(auth, c_ssid, c_pw);
    }
  }
  timeClient.update();
  if (wf.CONNECTED) {
    timeClient.update(); // cập nhật sever thời gian hiện tại
    /* Serial.print(n.getHours());
      Serial.print(":");
      inso(n.getMinutes());
      Serial.print(":");
      inso(n.getSeconds());
      Serial.println(""); */
    Blynk.run();
    timer.run();
  }
  //read cmd
  switch (cmd.read()) {
    case 0:  // scan
      { scan();
        break;
      }
    case 1:  // connect
      //  <ssid>::<password>
      { int sp = cmd.param.indexOf("::");
        String ssid = cmd.param.substring(0, sp);
        String pw = cmd.param.substring(sp + 2);
        WiFi.disconnect();
        delay(300);
        WiFi.begin(ssid, pw);
        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED)
        {
          delay(100);
        }
        char c_ssid[50];
        char c_pw[30];
        ssid.toCharArray(c_ssid, ssid.length());
        pw.toCharArray(c_pw, pw.length());
        Blynk.begin(auth, c_ssid, c_pw);
        timer.setInterval(1000L, myTimerEvent); //1s gui du lieu 1 lan len Blynk
        timeClient.begin();// khởi động truy cập sever thời gian
        Serial.print("status.1;");
        wf.CONNECTED = true;
        break;
      }
    case 2:  //disconnect
      {
        WiFi.disconnect();
        delay(100);
        Serial.print("status.0;");
      }
      break;
    case 3:  // info.<t>,<h>,<l>;
      {
        String s = cmd.param; // <t>,<h>,<l>   "24.5,96,0"
        int vt_phay1 = s.indexOf(',');                   //vi tri cua ky tu =4
        int vt_phay2 = s.indexOf(',', vt_phay1 + 1);
        temperature = s.substring(0, vt_phay1);          //chuoi tu vi tri 0 - (vt_phay1 -1) = 24.5
        humidity = s.substring(vt_phay1 + 1, vt_phay2);
        String ls = s.substring(vt_phay2 + 1, s.length());
        if (ls != "nope") light = ls == "0" ? false : true;
        Blynk.virtualWrite(V0, temperature);
        Blynk.virtualWrite(V1, humidity);
        Blynk.virtualWrite(V5, light);
        Serial.print("da nhan;");
      }
      break;
    case 4: // get
      {
        String s = cmd.param;
        if (s == "wifi") {
          if (WiFi.status() == WL_CONNECTED) {
            Serial.print("connected." + WiFi.SSID() + "::" + WiFi.psk() + ";");
          }
        }
      }
      break;
    case 5:  //water.1, water.0
      {
        waterState = cmd.param;
        if (waterState == "1") {  //con nuoc
          w.off();
          water_notified = false;
        }
        else {
          w.on();     // het nuoc
          if (!water_notified && ((millis() - last_notification > 1000) || last_notification == 0)) {
            Blynk.notify(" Hết nước!");
            water_notified = true;
            last_notification = millis();
          }
        }
      }
  }

}




//  list.<n>,<ssid1>::<strength1>\\<ssid2>....;
void scan() {
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  //  WiFi.mode(WIFI_STA);
  //  WiFi.disconnect();
  //  delay(100);
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  wf.sort(n);
  Serial.print("list.");
  for (int i = 0; i < wf.n; i++) {
    // Print SSID and RSSI for each network found
    if (i > 0) Serial.print("\\\\");  //name
    Serial.print(WiFi.SSID(wf.order[i]));  //name
    Serial.print("::");
    Serial.print(WiFi.RSSI(wf.order[i]));  //strength in dBm
  }
  Serial.print(';');  //name
}

/*void inso(byte so){
  if(so < 10)
   Serial.print('0');
  Serial.print(so, DEC);
  }*/
