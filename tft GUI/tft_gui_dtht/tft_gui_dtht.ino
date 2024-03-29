#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <C:\font\FontConvert\roboto24pt.h>
#include <C:\font\FontConvert\roboto-black-8pt.h>
#include <C:\font\FontConvert\hand16pt.h>
#include <C:\font\FontConvert\test.h>  // hand24pt
#include <C:\font\FontConvert\ss10pt.h>
#include <C:\font\FontConvert\ss13pt.h>
#include <C:\font\FontConvert\ss16pt.h>
#include <C:\font\FontConvert\ss20pt.h>
#include <avr\pgmspace.h>
#include <TouchScreen.h>
#include <arduino-timer.h>

#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <SD.h>
#include <SPI.h>

#include <EEPROM.h>

#include "text.h"
#include "gui_elements.h"
#include "gradient.h"

#define ADDR_TIMER 100

#define upto(n) for (int i = 0; i < n; i++)
#define fromto(m,n) for (int i = m; i <= n; i++)

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define PIN_SD_CS 10 // Adafruit SD shields and modules: pin 10
//
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
#define LCD_CS A3   // Chip Select goes to Analog 3
#define LCD_CD A2  // Command/Data goes to Analog 2
#define LCD_WR A1  // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

//Param For 3.5"
#define TS_MINX 118
#define TS_MAXX 906

#define TS_MINY 92
#define TS_MAXY 951



#define MINPRESSURE 10
#define MAXPRESSURE 1000

#define SCREEN__MAIN 0
#define SCREEN__SETUP 1
#define SCREEN__ENVIR 2
#define SCREEN__HT_INPUT 3
#define SCREEN__WIFI_PASSWORD 4
#define SCREEN__WIFI_LIST 5
#define SCREEN__WIFI_STATUS 6
#define SCREEN__DEVICE_STATUS 7
#define SCREEN__LIGHT_SCREEN__SETUP 8
#define SCREEN__TIMER_LIST 9
#define SCREEN__TIMER_SET 10

#define AUTO 0
#define MANUAL 1
#define Backcolor24 {8, 44, 54}
#define topBarBackground 0x1a6b

unsigned long lastPing = 0;
unsigned long lastTouchParamMainScreen;

auto timer = timer_create_default();

MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);  //300 Ohm
int wscr = 480;
int hscr = 320;
int tx, ty, tz;
bool touched, modeChangedRecently = false;
uint8_t mode = AUTO;

float temperature = 0, humidity = 0, temperatureSP = 0, humiditySP = 0;
bool light, prev_light;
bool outOfWater = false, prev_outOfWater = false;

class Screen {
  public:
    bool Changed = false;
    uint8_t history[10], prev = SCREEN__MAIN;
    int8_t i = -1;
    void add(uint8_t scr) {
      if (i != -1) prev = history[i];
      i++;
      history[i] = scr;
      //changeScreen(scr);
      Changed = true;
    }
    void pop() {
      prev = history[i];
      if (i > 0) i--;
      else history[i] = SCREEN__MAIN;
      //changeScreen(history[i]);
      Changed = true;
    }
    uint8_t current() {
      return history[i];
    }
    uint8_t previous() {
      return history[max(i - 1, 0)];
    }
    uint8_t at(uint8_t to) {
      return history[to];
    }
    void truncate(uint8_t to) {
      prev = history[i];
      i = to;
      //changeScreen(history[i]);
      Changed = true;
    }
    void reset() {
      prev = history[i];
      i = 0;
      //changeScreen(history[0]);
      Changed = true;
    }

    bool hasJustChanged() {
      bool btemp = Changed;
      Changed = false;
      return btemp;
    }

    //    void changeScreen(uint8_t md) {
    //      switch (current()) {
    //        case SCREEN__MAIN:
    //          drawMainScreen();
    //          break;
    //        case SCREEN__SETUP:
    //          drawSetupScreen();
    //          break;
    //        case SCREEN__ENVIR:
    //          drawEnvirScreen();
    //          break;
    //        case SCREEN__HT_INPUT:
    //          drawEnvirInputs();
    //          break;
    //        case ENTER_PASSWORD_SCREEN:
    //          drawEnterPasswordScreen();
    //          break;
    //      }
    //    }
};
Screen screen;

#define N_COMMAND 6
class {
  public:
    uint8_t port = 0;
    const String cmd[N_COMMAND] = {"list", "status", "time", "strength", "connected", "setpoint"};
    const uint8_t ncmd = N_COMMAND;
    bool done = false;
    String buffer = "";
    String param;

    int read() {
      while (Serial2.available() > 0) {
        char b = Serial2.read();
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
} cmd;

#define N_COMMAND2 3
class {
  public:
    uint8_t port = 0;
    const String cmd[N_COMMAND2] = {"info", "water", "deviceStatus"};
    const uint8_t ncmd = N_COMMAND2;
    bool done = false;
    String buffer = "";
    String param;

    int read() {
      while (Serial1.available() > 0) {
        char b = Serial1.read();
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
} cmd2;

//------------------------------------------------------For showing bitmap image and logging (work with SD card)------------------------------------------------------

//---------------------------------------------------------------Graphics: Gradient-----------------------------------------------------

//-----------------------------------------------------------Graphics Elements---------------------------------------------------------------
class GroupBox {
  public:
    String name;
    uint16_t x, y, x2, y2, border, fill, hideLine, nameBackColor = Backcolor, nameForeColor;
    uint8_t r, namepxh = 16, nameTxtSize;

    GroupBox(String NAME, int X, int  Y, int X2, int Y2, uint16_t BORDER = WHITE, uint8_t R = 5, uint16_t FILL = Transparent) {
      name = NAME;
      x = X;
      y = Y;
      x2 = X2;
      y2 = Y2;
      r = R;
      border = BORDER;
      fill = FILL;
      r = R;
      hideLine = 90;
      nameForeColor = WHITE;
      nameTxtSize = 1;
    }
    void draw() {
      if (r == 0) {
        tft.drawRect(x, y, x2 - x, y2 - y, border);
        if (fill != Transparent) tft.fillRect(x, y, x2 - x, y2 - y, fill);
      } else {
        tft.drawRoundRect(x, y, x2 - x, y2 - y, r, border);
        if (fill != Transparent) tft.fillRoundRect(x, y, x2 - x, y2 - y, r, fill);
      }
      //draw label
      if (hideLine != 0) tft.drawFastHLine(x + 5, y, hideLine, nameBackColor);
      text(name, x + 10, y + namepxh / 2, nameTxtSize, nameForeColor, &hand16pt);
    }
};

class Wifi {
  public:
    uint8_t spacing = 2, hmax = 20, w = 3;
    uint8_t bar = 0;
    bool stt = false, receivedWifiList, printedWifiList;
    String ssid[10];
    signed int rssi[10];
    int n = 0;
    String password, name;
    signed int strength;
    int x = 450, y = 27;

    void drawSignal(bool eraseBackground) {
      if (eraseBackground)
        tft.fillRect(x - 7, y - hmax, 35, hmax, topBarBackground);
      if (stt) {
        uint16_t color[] = {RED, YELLOW, GREEN, GREEN};
        for (char i = 0; i < 4; i++) {
          int h = (i + 1) * hmax / 4;
          if (i < bar) tft.fillRect(x + i * (w + spacing), y - h, w, h, color[bar - 1]);
          else {
            tft.fillRect(x + i * (w + spacing), y - h, w, h, topBarBackground);
            tft.drawRect(x + i * (w + spacing), y - h, w, h, Backcolor);
          }
        }
      } else {
        for (char i = 0; i < 4; i++) {
          int h = (i + 1) * hmax / 4;
          tft.drawRect(x + i * (w + spacing), y - h, w, h, RED);
        }
        uint16_t diagon = RED;
        tft.drawLine(x - 5, y - hmax, x + 22, y - 2, diagon);
        tft.drawLine(x - 5, y - hmax + 1, x + 22, y - 1, diagon);
        tft.drawLine(x - 5, y - hmax + 2, x + 22, y, diagon);
      }
    }

    void setStrength(signed int str) {
      strength = str;
      bar = map(str, -90, -55, 1, 4);
    }

    bool pressed = false, prev;
    void onclicked(void (*f)()) {
      prev = pressed;
      pressed = touched && (tx >= x - 5 && tx < x + 22 && ty >= y - hmax && ty < y);
      if (!prev && pressed ) { //pressed down
        (*f)();
      }
    }
};

class Switch {
  public:
    bool st;
    uint16_t x, y;
    uint8_t w, h, r, pos;
    uint16_t onColor = tft.color565(75, 215, 100);
    uint16_t offColor = tft.color565(215, 220, 222);
    Switch(uint16_t X, uint16_t Y, uint16_t W = 60, uint16_t H = 30) {
      x = X;
      y = Y;
      w = W;
      h = H;
      r = (uint8_t)(0.46 * h);
    }
    void draw(bool stt) {
      st = stt;
      if (st) {
        tft.fillRoundRect(x, y, w, h, h / 2, onColor);
        tft.fillCircle(x + w - h / 2 + pos, y + h / 2, r, WHITE);
      } else {
        tft.fillRoundRect(x, y, w, h, h / 2, offColor);
        tft.fillCircle(x + h / 2, y + h / 2, r, WHITE);
      }
    }
};

class Clock {
  public:
    uint8_t hour, minute, day, month, dayOfWeek = 0;
    uint16_t year;

    void set(uint8_t _hour, uint8_t _minute = 0, uint8_t _day = 0, uint8_t _month = 0, uint16_t _year = 0) {
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
      return (dayOfWeek == 0 ? String("CN") : String('T') + String(dayOfWeek + 1)) + String(", ") + form(day) + String("/") + form(month) + String("/") + String(year);
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

class Message {
  public:
    String s;
    int w = 100, h = 20, x, y, xText = 3, yText = 3, sizeText = 1;
    uint16_t color = WHITE;
    const GFXfont *font = NULL;
    uint16_t backcolor = RED;

    void draw(String _s, int xx, int yy) {
      s = _s;
      x = xx;
      y = yy;
      tft.fillRect(x, y, w, h, backcolor);
      text(s, x + xText, y + yText, sizeText, color, font);
    }

    void redraw() {
      tft.fillRect(x, y, w, h, backcolor);
      text(s, x + xText, y + yText, sizeText, color, font);
    }
};
Message msg;

class Btn {     //Button
  public:
    String btnText;
    uint16_t border = BLACK, back = WHITE, fore = BLACK;
    GradRectH gr;
    int x, y, w, h, xText = 7;
    bool visible = true;

    Btn(String s, int ww = 200, int hh = 30) {
      btnText = s;
      w = ww;
      h = hh;
    }

    void draw(int xx, int yy) {
      if (visible == false) return;
      x = xx; y = yy;
      tft.fillRoundRect(x, y, w, h, 4, back);
      tft.drawRoundRect(x, y, w, h, 4, border);
      //      if (clicked) {
      //        tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 4, border);
      //        tft.drawRect(x + 2, y + 2, w - 4, h - 4, border);
      //      }

      gr.x = x + 2;  gr.len = w - 4;
      gr.n = 2;

      gr.p[1] = y + h - 2;     //end gradient
      gr.color[0] = {255, 255, 255};  //begin color
      //      if (clicked) {
      //        gr.p[0] = y + h / 3 + 2; //begin gradient
      //        gr.color[1] = {50, 50, 50};  //end color
      //      } else {
      gr.p[0] = y + h / 4 + 2; //begin gradient
      gr.color[1] = {150, 150, 150};  //end color
      //      }
      gr.draw();

      text(btnText, x + xText, y + h - 10, 1, fore, &ss10pt);
    }

    bool pressed = false, clicked = false, released = false, prev;
    unsigned long tu = millis(), td = tu;
    void onclicked(void (*f)()) {
      prev = pressed;
      pressed = touched && (tx >= x && tx < x + w && ty >= y && ty < y + h);
      if (!prev && pressed ) { //pressed down
        if (millis() - td > 700 && millis() - tu > 400) {
          clicked = true;
          draw(x, y);
          (*f)();
          clicked = false;
        }
        td = millis();
      }
      if (prev && !pressed) { //released up
        if (millis() - tu > 700 && millis() - td > 400) {
          clicked = false;
        }
        tu = millis();
      }
    }
};

class SimpleBtn {     //Button
  public:
    String btnText;
    uint16_t border = BLACK, back = BLACK, fore = WHITE, backInvert = YELLOW;
    int x, y, w, h, xText, yText;
    uint8_t textSize = 3;

    void init(String s, int xx, int yy, int ww = 200, int hh = 30) {
      btnText = s;
      x = xx; y = yy;
      w = ww;
      h = hh;
    }

    void draw(bool invert = false) {
      tft.fillRect(x, y, w, h, invert ? backInvert : back);
      tft.drawRect(x, y, w, h, border);
      text(btnText, x + xText, y + yText, textSize, fore/*, &ss10pt*/);
    }

    bool pressed = false, prev = false;
    unsigned long tu = millis(), td = tu;
    bool clicked() {
      bool temp = false;
      prev = pressed;
      pressed = touched && (tx >= x && tx < x + w && ty >= y && ty < y + h);
      if (!prev && pressed) { //pressed down
        if (millis() - td > 100 && millis() - tu > 100) {
          draw(true);
          temp = true;
        }
        td = millis();
      }
      if (prev && !pressed) { //released up
        //if (millis() - tu > 100 && millis() - td > 100) {
        draw();
        //}
        tu = millis();
      }
      return temp;
    }
};

class Keypad {
  public:
    SimpleBtn button[12];
    int x, y, w, h, wBtn, hBtn, spacing;
    void init(int xx, int yy, int ww = 180, int hh = 180, int spc = 2) {
      x = xx;
      y = yy;
      w = ww;
      h = hh;
      wBtn = w / 3;
      hBtn = h / 4;
      spacing = spc;
      for (int i = 0; i < 12; i ++) {
        button[i].btnText = KeypadSource[i];
      }

      button[11].back = 0x7000;
      int i = 0;
      for (int r = 0; r < 4; r ++) {
        for (int c = 0; c < 3; c ++) {
          button[i].x = x + c * wBtn + spacing / 2;
          button[i].y = y + r * hBtn + spacing / 2;
          button[i].w = wBtn - spacing;
          button[i].h = hBtn - spacing;
          button[i].yText = button[i].h / 2 - 7;
          button[i].xText = button[i].w / 2 - 6;
          i++;
        }
      }
    }
    void draw() {
      uint8_t padding = 2;
      tft.drawRect(x - padding, y - padding, w + 2 * padding, h + 2 * padding, BLACK);
      for (int i = 0; i < 12; i ++) {
        button[i].draw();
      }
    }

    String getPressed() {
      String key = "";
      for (int i = 0; i < 12; i ++) {
        if (button[i].clicked()) {
          if (i == 11) key = "del";
          else key = button[i].btnText;
        }
      }
      return key;
    }
};

class KeyboardTouch {
  public:
    int x = 10, y = 100, w = 460, h = 200, wBtn, hBtn, padding = 2;
    uint8_t mode = 0;
    bool caps = false;
    SimpleBtn key[30], cmdKey[5];

    void init() {
      wBtn = w / 10;
      hBtn = h / 4;

      int i = 0;
      for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 10; c++) {
          char kt;
          switch (mode) {
            case 0:
              if (caps) kt = pgm_read_byte(&CapsLeterKeys[r][c]);
              else kt = pgm_read_byte_near(&LeterKeys[r][c]);
              break;
            case 1:
              kt = pgm_read_byte(&NumKeys[r][c]);
              break;
            case 2:
              kt = pgm_read_byte(&SymKeys[r][c]);
              break;
          }

          key[i].x = x + c * wBtn + padding;
          key[i].y = y + r * hBtn + padding;
          key[i].w = wBtn - 2 * padding;
          key[i].h = hBtn - 2 * padding;
          key[i].xText = wBtn / 2 - 8;
          key[i].yText = hBtn / 2 - 8;
          key[i].btnText = kt;
          key[i].border = BLACK;
          key[i].back = WHITE;
          key[i].fore = BLACK;
          i++;
        }
      }

      int xk = 0;
      for (int i = 0; i < 5; i++) {
        cmdKey[i].textSize = 2;
        cmdKey[i].btnText = KeyboardCmd[i];
        cmdKey[i].y = y + 3 * hBtn + padding;
        cmdKey[i].w = KeyboardCmdW[i] * wBtn - 2 * padding;
        if (i > 0) xk += cmdKey[i - 1].w + 2 * padding;
        cmdKey[i].x = x + xk + padding;
        cmdKey[i].h = hBtn - 2 * padding;
        cmdKey[i].xText = cmdKey[i].w / 2 - cmdKey[i].btnText.length() * 6;
        cmdKey[i].yText = hBtn / 2 - 5;
      }
      cmdKey[2].back = WHITE;
      cmdKey[2].fore = BLACK;
    }

    void draw() {
      for (int i = 0; i < 30; i++) {
        key[i].draw();
      }
      for (int i = 0; i < 5; i++) {
        cmdKey[i].draw();
      }
    }

    void redraw() {
      init();
      for (int i = 0; i < 30; i++) {
        key[i].draw();
      }
    }

    String getPressed() {
      String kt = "";
      if (cmdKey[2].clicked()) {
        return " ";
      }
      if (cmdKey[3].clicked()) {
        return "del";
      }
      if (cmdKey[4].clicked()) {
        return "ok";
      }
      if (cmdKey[0].clicked()) {
        mode = mode == 2 ? 0 : mode + 1;
        redraw();
      }
      if (cmdKey[1].clicked()) {
        caps = !caps;
        if (mode == 0) redraw();
      }

      for (int i = 0; i < 30; i++) {
        if (key[i].clicked()) {
          kt = key[i].btnText;
          break;
        }
      }

      return kt;
    }
};

class Input {
  public:
    int x, y, w = 100, h = 35, xText = 10, yText = 25;
    String value = "";
    uint16_t backColor = WHITE, foreColor = BLACK, borderColor = BLACK;
    bool selected = false;

    void draw(int xx, int yy) {
      x = xx;
      y = yy;
      tft.fillRect(x, y, w, h, backColor);
      tft.drawRect(x, y, w, h, borderColor);
      if (value != "") {
        text(value, x + xText, y + yText, 1, foreColor, &ss13pt);
      }
    }

    void redraw(bool refresh = false) {
      if (refresh) tft.fillRect(x + 1, y + 1, w - 2, h - 2, backColor);
      if (value != "") {
        text(value, x + 10, y + 25, 1, foreColor, &ss13pt);
      }
    }

    void typeIn(String s) {
      if (s == "del") {
        if (value != "") {
          value.remove(value.length() - 1, 1);
          redraw(true);
        }
      } else {
        value = value + s;
        redraw();
      }
    }

    void select() {
      selected = true;
      tft.drawRect(x - 3, y - 3, w + 6, h + 6, WHITE);
    }

    void deselect() {
      selected = false;
      tft.drawRect(x - 3, y - 3, w + 6, h + 6, Backcolor);
    }

    bool pressed = false, prev;
    unsigned long tu = millis(), td = tu;
    void onclicked(void (*f)()) {
      prev = pressed;
      pressed = touched && (ty >= y && ty < y + h && tx >= x && tx < x + w);
      if (!prev && pressed ) { //pressed down
        if (millis() - td > 500 && millis() - tu > 200) {
          Serial.println("input selected");
          select();
          (*f)();
        }
        td = millis();
      }
      if (prev && !pressed) { //released up
        //        if (millis() - tu > 500 && millis() - td > 200) {
        //        }
        tu = millis();
      }
    }
};

//class KeyboardTouch {
//  public:
//
//}

class Menu {
    class Element {
      public:
        uint8_t id = 0;
        String lb;
        int ystart = 115, y, h = 45;
        bool last = false;
        Color border = {156, 198, 255}, selectedColor{10, 82, 102};
        bool defaultFont = false;
        bool fontSize = 16;

        void draw(bool eraseBackground) {
          y = ystart + id * h;
          if (eraseBackground) {
            tft.fillRect(0, y + 1, wscr, h - 2, Backcolor);
          }
          GradLineH(0, y, wscr, 1, border, Backcolor24).draw();
          if (defaultFont) {
            text(lb, 60, y + h / 3, 2, WHITE);
          } else {
            text(lb, 60, y + 2 * h / 3, 1, WHITE, &ss16pt);
          }
          if (last) GradLineH(0, y + h, wscr, 1, border, Backcolor24).draw();
        }

        bool pressed = false, clicked = false, prev;
        unsigned long tu = millis(), td = tu;
        bool onclicked(void (*f)()) {
          bool temp = false;
          prev = pressed;
          pressed = touched && (ty >= y && ty < y + h);
          if (!prev && pressed ) { //pressed down
            if (millis() - td > 500 && millis() - tu > 200) {
              clicked = true;
              tft.drawRect(0, y + 1, wscr, h - 2, Backcolor);
              if (defaultFont) {
                text(lb, 60, y + h / 3, 2, 0x7e0);
              } else {
                text(lb, 60, y + 2 * h / 3, 1, 0x7e0, &ss16pt);
              }
              temp = true;
              (*f)();
              clicked = false;
            }
            td = millis();
          }
          if (prev && !pressed) { //released up
            if (millis() - tu > 500 && millis() - td > 200) {
              clicked = false;
            }
            tu = millis();
          }
          return temp;
        }
    };
  public:
    Element list[10];
    uint8_t n;

    Menu(uint8_t nn) {
      n = nn;
      for (int i = 0; i < n; i++) {
        list[i].id = i;
      }
      list[n - 1].last = true;
    }
    void draw() {
      for (int i = 0; i < n; i++) {
        list[i].draw(false);
      }
    }
};

class Header {
  public:
    String lb;
    Image backBtn = Image("goback.bmp"), exitBtn = Image("exit.bmp");
    int xlb = 180;
    Header(String lbl) {
      lb = lbl;
    }
    void draw() {
      xlb = wscr / 2 - 9 * lb.length();;
      backBtn.init();
      exitBtn.init();
      text(lb, xlb, 80, 1 , WHITE, &ss20pt);
      if (!ignoreIconHeader()) {
        backBtn.draw(30, 50, true);
        exitBtn.draw(420, 50, true);
      } else {
        backBtn.x = 30;
        backBtn.y = 50;
        exitBtn.x = 420;
        exitBtn.y = 50;
      }
    }
    void onclicked() {
      exitBtn.onclicked([] {screen.reset();});
      backBtn.onclicked([] {screen.pop(); });
    }
};

//--------------------------------------------------------------Declaration and functions----------------------------------------------------
GradRectH gr;
GradCircle grc;
Image cordycepsBox("box.bmp"), tIcon("tprt.bmp"), hIcon("Humidity.bmp"), lIcon("light.bmp"), automode("auto.bmp", 100), manualmode("manual.bmp", 100) ;
Input tInput, hInput, pwInput, hourInput, minuteInput;
Btn inputOk("Ok");

Header header("");

//auto timer = timer_create_default();

uintptr_t task;
Wifi wifi;

String deviceStatus = "0000000";

bool getData(void *) {
  Serial1.print("get;");
  Serial1.print("getStatus;");
  return true; // repeat? true
}

void text(String s , int x, int y, int txtSize = 1, uint16_t cl = WHITE, const GFXfont *txtFont = NULL) {
  if (txtFont != NULL) tft.setFont(txtFont);
  else tft.setFont();
  tft.setCursor(x, y);
  tft.setTextColor(cl);
  tft.setTextSize(txtSize);
  tft.println(s);
}

String trimFloat(float f, int n = 2) {
  String s = String(f, n);
  uint8_t comma = s.lastIndexOf('.'), zero = -1;
  if (comma != -1) {
    for (int i = s.length() - 1; i > comma; i--) {
      if (s.charAt(i) == '0') zero = i;
      else break;
    }
    if (zero != -1) {
      if (zero == comma + 1) zero = comma;
      s = s.substring(0, zero);
    }
  }
  return s;
}

void switchMode() {
  switch (mode) {
    case AUTO:
      mode = MANUAL;
      break;
    case MANUAL:
      mode = AUTO;
      break;
  }
}

bool c2b(char ch) {
  return ch == '1' ? true : false;
}

// -----------------------screens-------------
void clr() {
  if (ignoreIconHeader()) {
    tft.fillRect(70, gr.p[2], wscr - 140, 130, Backcolor);
    tft.fillRect(0, 110, wscr, hscr - 110, Backcolor);
  } else {
    tft.fillRect(0, gr.p[2], wscr, hscr - gr.p[2], Backcolor);
  }
}

bool ignoreIconHeader() {
  return hasHeader(screen.prev) && hasHeader(screen.current());
}

bool hasHeader(uint8_t scr) {
  return (scr == SCREEN__TIMER_SET || scr == SCREEN__TIMER_LIST || scr == SCREEN__LIGHT_SCREEN__SETUP || scr == SCREEN__SETUP || scr == SCREEN__ENVIR || scr == SCREEN__HT_INPUT || scr == SCREEN__WIFI_LIST || scr == SCREEN__DEVICE_STATUS);
}

void drawTime(bool drawDay) {
  tft.fillRect(9, 5, 200, 28, topBarBackground);
  text(clk.getTimeString(), 10, 8, 2); // top bar time
  if (screen.current() == SCREEN__MAIN) {
    tft.fillRect(320, 53, 150, 50, Backcolor);
    text(clk.getTimeString(), 330, 90, 1, WHITE, &Roboto_Light24pt7b);
    if (drawDay) {
      tft.fillRect(320, 106, 150, 20, Backcolor);
      text(clk.getDateString(), 330, 120, 1, WHITE, &Roboto_Black8pt7b);
    }
  }
}

void drawTopBar() {
  gr.x = 0;  gr.len = wscr;
  gr.n = 3;
  gr.p[0] = 30;   gr.p[1] = 33; gr.p[2] = 35;
  gr.color[0] = {24, 78, 93};
  gr.color[1] = {75, 141, 160};
  gr.color[2] = {8, 44, 54};
  tft.fillRect(0, 0, wscr, gr.p[0], tft.color565(24, 78, 93));
  gr.draw();
  drawTime(true);
  wifi.drawSignal(false);
}



void screenMain() {     //draw main screen
  GroupBox thongso("Th\u00a6ng s\u00a8:", 10, 55, 280, 310), groupBox2("", 295, 150, 470, 310);
  Btn gotoSetupBtn("T\u00aei b\u0082ng \u001fi\u0097u khi\u0099n");
  Message accident;
  Btn sttOpen("");

  sttOpen.x = 300;
  sttOpen.y = 170;
  sttOpen.w = 170;
  sttOpen.h = 130;
  sttOpen.visible = false;

  groupBox2.hideLine = 0;
  tft.fillRect(0, gr.p[2], wscr, hscr - gr.p[2], Backcolor);
  //clock
  int xt = 300, yt = 90;
  drawTime(true);
  tft.drawFastVLine(xt - 5, yt - 40, 85, WHITE);
  tft.drawFastHLine(xt - 5, yt + 45, 170, WHITE);
  //GradLineH(295, 125, 180, 1, {255, 255, 255}, {8, 44, 54}).draw();
  thongso.draw();
  xt = thongso.x + 70, yt = thongso.y + 60;
  tft.fillCircle(xt - 31, yt - 15, 20, WHITE);
  lIcon.draw(xt - 45, yt - 29);
  text(String("\u001cnh s\u0081ng: ") + String(light ? "B\u0090t" : "T\u0087t"), xt, yt, 1, WHITE, &test); //LIGHT
  yt += 50;
  tft.fillCircle(xt - 31, yt - 15, 20, WHITE);
  hIcon.draw(xt - 41, yt - 30);
  text(String("\u001e\u00ab \u008em: ") + (!(humidity >= 0 && humidity <= 100) ? ".." : String( round(humidity)) ) + String("%"), xt, yt, 1, WHITE, &test);  //HUMIDITY
  yt += 50;
  tft.fillCircle(xt - 31, yt - 15, 20, WHITE);
  tIcon.draw(xt - 45, yt - 29);
  text(String("Nhi\u009bt \u001f\u00ab: ") + (!(temperature > -10 && temperature < 60) ? ".." : String( round(temperature)) ) + String("\u001d"), xt, yt, 1, WHITE, &test); //TEMPERATURE

  GradLineH(20, 240, 250, 1, {255, 255, 255}, {8, 44, 54}).draw();
  gotoSetupBtn.draw(45, 260);

  groupBox2.draw();
  xt = groupBox2.x + 40;
  yt = groupBox2.y;

  accident.font = &ss10pt;
  accident.w = 150;
  accident.h = 30;
  accident.yText = 20;
  accident.backcolor = Backcolor;
  //automode.draw(xt, yt + 15);
  if (!(humidity >= 0 && humidity <= 100) || !(temperature >= -10 && temperature <= 60)) {
    accident.xText = 6;
    accident.color = RED;
    accident.draw("L\u00aai c\u0082m bi\u0098n!", 315, 275);
  } else if (outOfWater) {
    accident.xText = 9;
    accident.color = RED;
    accident.draw("H\u0098t n\u00b8\u00aec!", 315, 275);
  } else {
    accident.xText = 7 ;
    accident.color = GREEN;
    accident.draw("Ho\u0084t \u001f\u00abng t\u00a8t", 315, 275);
  }

  int x = 306, y = 180;
  text("\u002b Chi\u0098u s\u0081ng", x, y, 1, WHITE, &ss10pt);
  y += 25;
  text("\u002b L\u0080m m\u0081t", x, y, 1, WHITE, &ss10pt);
  y += 25;
  text("\u002b T\u0084o \u008em", x, y, 1, WHITE, &ss10pt);
  y += 25;
  text("\u002b Gia nhi\u009bt", x, y, 1, WHITE, &ss10pt);

  lastTouchParamMainScreen = millis();
  bool prev_touch = false;
  while (true) {
    xt = 30, yt = 80;
    checkTouch();

    int x = 450, y = 175, radius = 8;
    tft.drawCircle(x, y, radius + 1, WHITE);
    tft.fillCircle(x, y, radius, c2b(deviceStatus.charAt(0)) ? BLUE : GRAY);
    y += 25;
    tft.drawCircle(x, y, radius + 1, WHITE);
    tft.fillCircle(x, y, radius, c2b(deviceStatus.charAt(1)) ? BLUE : GRAY);
    y += 25;
    tft.drawCircle(x, y, radius + 1, WHITE);
    tft.fillCircle(x, y, radius, c2b(deviceStatus.charAt(2)) ? BLUE : GRAY);
    y += 25;
    tft.drawCircle(x, y, radius + 1, WHITE);
    tft.fillCircle(x, y, radius, c2b(deviceStatus.charAt(3)) ? BLUE : GRAY);

    if (!(humidity >= 0 && humidity <= 100) || !(temperature >= -10 && temperature <= 60)) {
      accident.xText = 6;
      accident.color = RED;
      accident.draw("L\u00aai c\u0082m bi\u0098n!", 315, 275);
    } else if (prev_outOfWater != outOfWater) {
      outOfWater = prev_outOfWater;
      if (outOfWater) {
        accident.xText = 0;
        accident.color = RED;
        accident.draw("H\u0098t n\u00b8\u00aec!", 315, 275);
      } else {
        accident.xText = 0 ;
        accident.color = GREEN;
        accident.draw("Ho\u0084t \u001f\u00abng t\u00a8t", 315, 275);
      }
    }
    if (touched && (prev_touch != touched) && (millis() - lastTouchParamMainScreen > 500)) { //
      Serial.println("touched");
      lastTouchParamMainScreen = millis();
      if (tx >= xt && ty >= yt && tx < xt + 240 && ty < yt + 40) {
        light = !light;
        tft.fillRect(200 - 3, 115 - 40, 70, 50, Backcolor);
        text(String(light ? "B\u0090t" : "T\u0087t"), 200, 115, 1, WHITE, &test);
        Serial2.print(String("info.") + String(temperature) + String(',') + String(humidity)  + String(',')  + (light ? "1" : "0") + ';');
        Serial1.print(String("setpoint.") + temperatureSP + "," + String(humiditySP) + String(',') + String(light ? "1" : "0") + ";");
        Serial.print(String("to mini: setpoint.") + temperatureSP + "," + String(humiditySP) + String(',') + String(light ? "1" : "0") + ";");
        //Serial2.print(String("info.") + String(temperature) + String(',') + String(humidity)  + String(',')  + (light ? "1" : "0") + ';');
      }
      yt += 50;
      if (tx >= xt && ty >= yt && tx < xt + 240 && ty < yt + 92) {
        screen.add(SCREEN__HT_INPUT);
      }
    }
    prev_touch = touched;

    gotoSetupBtn.onclicked([] {screen.add(SCREEN__SETUP);});
    sttOpen.onclicked([] {screen.add(SCREEN__DEVICE_STATUS);});
    if (screen.hasJustChanged()) break;
  }
}

void screenSetup() {     //setup screen
  Menu menuSetup(3);
  menuSetup.list[1].lb = F("C\u0080i \u001f\u008at m\u0084ng");
  menuSetup.list[0].lb = F("Ch\u009enh th\u00a6ng s\u00a8 m\u00a6i tr\u00b8\u00adng");
  menuSetup.list[2].lb = F("Tr\u0084ng th\u0081i t\u00b5");

  clr();
  header.lb = "C\u001cI \u001e\u001dT";
  header.draw();
  menuSetup.draw();

  while (true) {
    checkTouch();
    header.onclicked();
    menuSetup.list[1].onclicked([] {
      if (!wifi.stt) {
        wifi.receivedWifiList = false;
        wifi.printedWifiList = false;
        screen.add(SCREEN__WIFI_LIST);
      } else {
        screen.add(SCREEN__WIFI_STATUS);
      }
    });
    menuSetup.list[0].onclicked([] {screen.add(SCREEN__ENVIR);});
    menuSetup.list[2].onclicked([] {screen.add(SCREEN__DEVICE_STATUS);});
    if (screen.hasJustChanged()) break;
  }
}


void screenEnvirScreen() {  // environment parameter setup
  Menu menuEnvir(2);

  menuEnvir.list[0].lb = "\u001e\u0091n chi\u0098u s\u0081ng";
  menuEnvir.list[1].lb = "Nhi\u009bt \u001f\u00ab, \u001e\u00ab \u008em";

  clr();
  header.lb = "M\u00a6i tr\u00b8\u00adng";
  header.draw();
  menuEnvir.draw();

  while (true) {
    checkTouch();
    header.onclicked();
    menuEnvir.list[0].onclicked([] {
      screen.add(SCREEN__LIGHT_SCREEN__SETUP);
    });

    menuEnvir.list[1].onclicked([] {screen.add(SCREEN__HT_INPUT);});
    if (screen.hasJustChanged()) break;
  }
}

void screenHTInputs() {       //humidity and temperature input
  Keypad keypad;

  keypad.init(280, 120);
  clr();
  header.lb = "T,H";
  header.draw();
  tInput.value = trimFloat(temperatureSP, 2);
  hInput.value = trimFloat(humiditySP, 2);
  int yt = 150;
  text("Nhi\u009bt \u001f\u00ab:", 10, yt, 1, WHITE, &ss13pt);
  tInput.draw(130, yt - 25);
  tInput.select();
  text("\u00b0\u0043", 235, yt, 1, WHITE, &ss13pt);
  yt += 50;
  text("\u001e\u00ab \u008em:", 10, yt, 1, WHITE, &ss13pt);
  hInput.draw(130, yt - 25);
  text("\u0025", 235, yt, 1, WHITE, &ss13pt);
  inputOk.w = 60;
  inputOk.xText = 15;
  inputOk.draw(160, yt + 40);
  keypad.draw();

  while (true) {
    checkTouch();
    header.onclicked();
    tInput.onclicked([] {hInput.deselect();});
    hInput.onclicked([] {tInput.deselect();});
    String key = keypad.getPressed();
    if (key != "") {
      if (tInput.selected) tInput.typeIn(key);
      if (hInput.selected) hInput.typeIn(key);
      tft.fillRect(30, 230, 80, 100, Backcolor);
    }
    inputOk.onclicked([] {
      float t = tInput.value.toFloat();
      float h = hInput.value.toFloat();
      if (h > 100) {
        h = 100;
        hInput.value = trimFloat(h);
        hInput.redraw(true);
      }
      if (t > 30) {
        t = 30;
        tInput.value = trimFloat(t);
        tInput.redraw(true);
      }
      if (t < 10) {
        t = 10;
        tInput.value = trimFloat(t);
        tInput.redraw(true);
      }

      temperatureSP = t;
      humiditySP = h;
      tft.fillRect(30, 230, 80, 100, Backcolor);
      text("\u001e\u0083 l\u00b8u!", 30, 260, 1, GREEN, &ss10pt);
      Serial1.print(String("setpoint.") + t + "," + String(h) + String(',') + String(light ? "1" : "0") + ";");
      Serial.println(String("setpoint.") + t + "," + String(h) + String(',') + String(light ? "1" : "0") + ";");
    });

    if (screen.hasJustChanged()) break;
  }
}

void screenWifiList() {
  wifi.printedWifiList = false;  // chua in ra
  if (!wifi.receivedWifiList) { // neu chua nhan duoc wifi thi gui lenh
    Serial2.print("scan;");
    Serial.println("sent scan cmd");
  }
  Menu menuWifi(1);

  clr();
  header.lb = "Wifi";
  header.draw();

  if (wifi.receivedWifiList) {  // neu da nhan dc wifi thi in ra
    menuWifi = Menu(wifi.n);
    for (int i = 0; i < wifi.n; i++) {
      menuWifi.list[i].lb = String(i + 1) + ". " + wifi.ssid[i] + "  " + wifi.rssi[i];
      menuWifi.list[i].h = 30;
      menuWifi.list[i].defaultFont = true;
    }
    menuWifi.draw();
    wifi.printedWifiList = true;  // da in ra
  }

  while (true) {
    checkTouch();
    header.onclicked();

    if (!wifi.receivedWifiList) {   // neu chua nhan dc thi dang quet
      msg.draw("Scanning...", 200, 200);
    } else {
      if (!wifi.printedWifiList) break;  // neu da nhan dc thi: neu chua in thi break, da in thi thoi
    }

    for (int i = 0; i < wifi.n; i++) {
      if (menuWifi.list[i].onclicked([] {
      screen.add(SCREEN__WIFI_PASSWORD);
      })) {
        wifi.name = wifi.ssid[i];
        break;
      }
    }

    if (screen.hasJustChanged()) break;
  }
}

void screenWifiPassword() {
  KeyboardTouch keyboard;
  keyboard.init();

  clr();
  keyboard.draw();
  text("Password:", 20, 80, 1, WHITE, &ss13pt);
  pwInput.draw(150, 50);

  while (true) {
    checkTouch();

    String key = keyboard.getPressed();
    if (key != "") {
      if (key == "ok") {
        wifi.password = pwInput.value;
        Serial2.print("connect." + wifi.name + "::" + wifi.password + ";");
        screen.pop(); screen.pop();
        screen.add(SCREEN__WIFI_STATUS);
        break;
      }

      pwInput.typeIn(key);
    }
    if (screen.hasJustChanged()) break;
  }
}

void screenWifiStatus() {
  Btn disconnect("Ng\u0087t k\u0098t n\u00a8i");

  clr();
  header.lb = "Wifi";
  header.draw();
  bool flag = false;

  unsigned long t = millis();

  if (wifi.stt) {
    text("\u001e\u0083 k\u0098t n\u00a8i:", 50, 140, 1, WHITE, &ss13pt);
    text(wifi.name, 70, 170, 1, WHITE, &ss13pt);
    flag = true;
    disconnect.w = 150;
    disconnect.draw(100, 270);
  }

  bool failed =  false;

  while (true) {
    checkTouch();
    header.onclicked();

    if (!flag) {
      if (failed) {
        msg.draw("Failed.", 200, 200);    // failed to connect
      } else {
        if (!wifi.stt) {
          msg.draw("Connecting...", 200, 200);  // connecting
        } else {
          msg.draw("Connected.", 200, 200);      // connected
          Serial.println("WiFi connected.");
          break;
        }
      }
    }

    disconnect.onclicked([] {
      Serial2.print("disconnect;");
      screen.pop();
    });

    if (screen.hasJustChanged()) break;
  }
}

void screenDeviceStatus() {
  clr();
  header.lb = "Tr\u0084ng th\u0081i";
  header.draw();

  int x = 70, y = 140;
  text("\u002b H\u009b th\u00a8ng chi\u0098u s\u0081ng", x, y, 1, GREEN, &ss10pt);
  y += 25;
  text("\u002b B\u00ab ph\u0090n l\u0080m m\u0081t", x, y, 1, GREEN, &ss10pt);
  y += 25;
  text("\u002b B\u00ab ph\u0090n t\u0084o s\u00b8\u00acng", x, y, 1, GREEN, &ss10pt);
  y += 25;
  text("\u002b B\u00ab ph\u0090n gia nhi\u009bt", x, y, 1, GREEN, &ss10pt);



  while (true) {
    checkTouch();
    header.onclicked();
    int x = 320, y = 135, radius = 8;
    tft.drawCircle(x, y, radius + 1, WHITE);
    tft.fillCircle(x, y, radius, c2b(deviceStatus.charAt(0)) ? BLUE : GRAY);
    y += 25;
    tft.drawCircle(x, y, radius + 1, WHITE);
    tft.fillCircle(x, y, radius, c2b(deviceStatus.charAt(1)) ? BLUE : GRAY);
    y += 25;
    tft.drawCircle(x, y, radius + 1, WHITE);
    tft.fillCircle(x, y, radius, c2b(deviceStatus.charAt(2)) ? BLUE : GRAY);
    y += 25;
    tft.drawCircle(x, y, radius + 1, WHITE);
    tft.fillCircle(x, y, radius, c2b(deviceStatus.charAt(3)) ? BLUE : GRAY);
    y += 25;
    if (screen.hasJustChanged()) break;
  }

}



/*
  void screenEnvirScreen() {  // environment parameter setup
  Menu menuEnvir(2);

  menuEnvir.list[0].lb = String("\u001e\u0091n chi\u0098u s\u0081ng: ") + (light ? String("B\u0090t") : String("T\u0087t"));
  menuEnvir.list[1].lb = "Nhi\u009bt \u001f\u00ab, \u001e\u00ab \u008em";

  clr();
  header.lb = "M\u00a6i tr\u00b8\u00adng";
  header.draw();
  menuEnvir.draw();

  while (true) {
    checkTouch();
    header.onclicked();
    menuEnvir.list[0].onclicked([] {
      light = !light;
      Serial2.print(String("info.") + String(temperature) + String(',') + String(humidity)  + String(',')  + (light ? "1" : "0") + ';');
      Serial.println(String("info.") + String(temperature) + String(',') + String(humidity)  + String(',')  + (light ? "1" : "0") + ';');
    });
    if (prev_light != light) {
      prev_light = light;
      menuEnvir.list[0].lb = String("\u001e\u0091n chi\u0098u s\u0081ng: ") + (light ? String("B\u0090t") : String("T\u0087t"));
      menuEnvir.list[0].draw(true);
      Serial2.print(String("setpoint.") + temperatureSP + "," + String(humiditySP) + String(',') + String(light ? "1" : "0") + ";");
    }
    menuEnvir.list[1].onclicked([] {screen.add(SCREEN__HT_INPUT);});
    if (screen.hasJustChanged()) break;
  }
  }
*/

class TimerRTC {
  public:
    bool enabled = true;
    bool sw = true;
    int hour, minute;
    bool flag = true;
    void (*exec)() = [] {};
    set(int h, int m) {
      this->hour = h;
      this->minute = m;
    }

    void enable() {
      this->enabled = true;
    }
    void disable() {
      this->enabled = false;
    }
    void tick() {
      if (!enabled) return;
      if (clk.hour == this->hour && clk.minute == this->minute) {
        if (flag) {
          if (screen.current() == SCREEN__MAIN) {
            if (light != sw) {
              int xt = 200, yt = 115;
              tft.fillRect(xt - 3, yt - 40, 70, 50, Backcolor);
              text(String(light ? "B\u0090t" : "T\u0087t"), xt, yt, 1, WHITE, &test);
            }
          }
          light = sw;
          //Serial2.print(String("info.") + String(temperature) + String(',') + String(humidity)  + String(',')  + (light ? "1" : "0") + ';');
          Serial.println(String("info.") + String(temperature) + String(',') + String(humidity)  + String(',')  + (light ? "1" : "0") + ';');
          Serial1.print(String("setpoint.") + temperatureSP + "," + String(humiditySP) + String(',') + String(light ? "1" : "0") + ";");

          flag = false;
        }
      } else {
        flag = true;
      }
    }
    String getTimeString() {
      return form(hour) + String(":") + form(minute);
    }

    String form(int x) {
      if (x < 10) {
        return String("0") + String(x);
      } else {
        return String(x);
      }
    }
};

class {
  public:
    int maxSize = 8;
    int size = 0;
    TimerRTC timer[8];

    void add(int h, int m, bool sw) {
      timer[size].set(h, m);
      timer[size].sw = sw;
      size++;
    }
    void remove(int k) {
      for (int i = k; i < size - 1; i++) {
        timer[i] = timer[i + 1];
      }
      size--;
    }
    void change(int k, int h, int m, bool sw) {
      timer[k].set(h, m);
      timer[k].sw = sw;
    }

    void tick() {
      for (int i = 0; i < size; i++) {
        timer[i].tick();
      }

    }
} timerArr;

void screenLightSetup() {
  Menu menuLight(2);

  menuLight.list[0].lb = String("Tr\u0084ng th\u0081i hi\u009bn t\u0084i: ") + (light ? String("B\u0090t") : String("T\u0087t"));
  menuLight.list[1].lb = String("H\u0095n gi\u002d b\u0090t t\u0087t") ;

  clr();
  header.lb = "\u00c1nh s\u0081ng";
  header.draw();

  menuLight.draw();

  while (true) {
    checkTouch();
    header.onclicked();

    menuLight.list[0].onclicked([] {
      light = !light;
      Serial1.print(String("setpoint.") + temperatureSP + "," + String(humiditySP) + String(',') + String(light ? "1" : "0") + ";");
      //Serial2.print(String("info.") + String(temperature) + String(',') + String(humidity)  + String(',')  + (light ? "1" : "0") + ';');
      //Serial.println(String("info.") + String(temperature) + String(',') + String(humidity)  + String(',')  + (light ? "1" : "0") + ';');
    });
    if (prev_light != light) {
      prev_light = light;
      menuLight.list[0].lb = String("Tr\u0084ng th\u0081i hi\u009bn t\u0084i: ") + (light ? String("B\u0090t") : String("T\u0087t"));
      menuLight.list[0].draw(true);
    }

    menuLight.list[1].onclicked([] {
      screen.add(SCREEN__TIMER_LIST);
    });

    if (screen.hasJustChanged()) break;
  }
}

signed int selectedTimer = 0, tempSelectedTimer = 0;
void screenTimerList() {
  Menu menuTimer(timerArr.size + 1);
  int count = 0;
  menuTimer.list[count].lb = "<<them hen gio>>";
  menuTimer.list[count].defaultFont = true;
  menuTimer.list[count].h = 24;
  for (int i = 0; i < timerArr.maxSize; i++) {
    count ++;
    menuTimer.list[count].defaultFont = true;
    menuTimer.list[count].lb = timerArr.timer[i].getTimeString() + String("  => ") + String(timerArr.timer[i].sw ? "Bat chieu sang" : "Tat chieu sang");
    menuTimer.list[count].h = 24;
  }

  clr();
  header.lb = "H\u0095n gi\u002d";
  header.draw();
  menuTimer.draw();

  while (true) {
    checkTouch();
    header.onclicked();
    menuTimer.list[0].onclicked([] {
      if (timerArr.size == 8) return;
      selectedTimer = timerArr.size;
      Serial.println(String("selected timer: ") + selectedTimer + String("(new timer)"));
      screen.add(SCREEN__TIMER_SET);
    });
    if (screen.hasJustChanged()) break;
    for (int i = 0; i < timerArr.size; i++) {

      tempSelectedTimer = i;
      menuTimer.list[i + 1].onclicked([] {
        selectedTimer = tempSelectedTimer;
        Serial.println(String("selected timer: ") + selectedTimer);
        screen.add(SCREEN__TIMER_SET);
      });
    }

    if (screen.hasJustChanged()) break;
  }
}
bool bsw = true;
bool changeTimerActionState = false;
void screenTimerSet() {
  Keypad keypad;
  Btn del("X\u00a2a h\u0095n gi\u002d n\u0080y"), sw("");


  keypad.init(280, 120);
  clr();
  header.lb = "\u001e\u008at gi\u002d";
  header.draw();
  Serial.print(String("enter timer setup with selected: ") + selectedTimer);
  hourInput.w = minuteInput.w = 50;
  if (selectedTimer != timerArr.size) {
    hourInput.value = timerArr.timer[selectedTimer].hour;
    minuteInput.value = timerArr.timer[selectedTimer].minute;
    bsw = timerArr.timer[selectedTimer].sw;
  } else {
    hourInput.value = "";
    minuteInput.value = "";
  }
  int yt = 130, xt = 40;
  text("Th\u002di \u001fi\u0099m:", 10, yt, 1, WHITE, &ss13pt);
  yt += 40;
  hourInput.draw(xt, yt - 25);
  hourInput.select();
  minuteInput.deselect();
  xt += 57;
  text("gi\u002d", xt, yt, 1, WHITE, &ss13pt);

  xt += 50;
  minuteInput.draw(xt, yt - 25);
  xt += 57;
  text("ph\u00b3t", xt, yt, 1, WHITE, &ss13pt);
  yt += 60;
  text("T\u00bd \u001f\u00abng", 10, yt, 1, WHITE, &ss13pt);
  xt = 115;
  sw.x = xt - 5;
  sw.y = yt - 20;
  sw.w = 100;
  sw.h = 30;
  tft.fillRect(xt - 5, yt - 20, 100, 30, WHITE);
  text(bsw ? "b\u0090t \u001f\u0091n" : "t\u0087t \u001f\u0091n", xt, yt, 1, BLACK, &ss13pt);

  inputOk.w = 60;
  inputOk.xText = 15;
  inputOk.draw(20, 280);

  del.w = 160;
  del.xText = 10;
  del.draw(90, 280);
  keypad.draw();

  while (true) {
    checkTouch();
    header.onclicked();
    hourInput.onclicked([] {minuteInput.deselect();});
    minuteInput.onclicked([] {hourInput.deselect();});
    String key = keypad.getPressed();
    if (key != "" && key != ".") {
      if (hourInput.selected) hourInput.typeIn(key);
      if (minuteInput.selected) minuteInput.typeIn(key);
    }
    inputOk.onclicked([] {
      if (hourInput.value == "" ||  minuteInput.value == "" ) return;
      int h = hourInput.value.toInt();
      int m = minuteInput.value.toInt();
      if (h > 23) {
        h = 23;
        hourInput.value = String(h);
        hourInput.redraw(true);
      }
      if (m > 59) {
        m = 59;
        minuteInput.value = String(m);
        minuteInput.redraw(true);
      }
      if (selectedTimer == timerArr.size) {
        timerArr.add(h, m, bsw);
        Serial.println(String("added timer ") + selectedTimer + "," + bsw);
      } else {
        timerArr.change(selectedTimer, h, m, bsw);
        Serial.println(String("edited timer: ") + selectedTimer + "," + bsw);
      }

      EEPROM.write(ADDR_TIMER - 1, 1);
      EEPROM.put(ADDR_TIMER, timerArr);
      text("\u001e\u0083 l\u00b8u!", 30, 270, 1, GREEN, &ss10pt);

      screen.pop();
    });
    del.onclicked([] {
      if (selectedTimer != timerArr.size) {
        timerArr.remove(selectedTimer);
        EEPROM.write(ADDR_TIMER - 1, 1);
        EEPROM.put(ADDR_TIMER, timerArr);
        Serial.println(String("deleted timer ") + selectedTimer);
      }
      screen.pop();
    });
    sw.onclicked([] {
      changeTimerActionState = true;
    });
    if (changeTimerActionState) {
      if (bsw == true) bsw = false;
      else bsw = true;

      Serial.print(String("timer action toggled") + String(bsw));
      tft.fillRect(xt - 5, yt - 20, 100, 30, WHITE);
      text(bsw ? "b\u0090t \u001f\u0091n" : "t\u0087t \u001f\u0091n", xt, yt, 1, BLACK, &ss13pt);
      changeTimerActionState = false;
    }
    if (screen.hasJustChanged()) break;
  }
}


//-----------------------------------------------------------------Main program--------------------------------------------------------------
void setup(void) {
  pinMode(11, INPUT);
  pinMode(12, INPUT);
  pinMode(13, INPUT);
  pinMode(53, OUTPUT);
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  Serial2.print("flush;");

  uint16_t identifier = tft.readID();
  tft.begin(identifier);

  // Init SD_Card
  pinMode(10, OUTPUT);
  if (!SD.begin(10 )) {
    tft.setCursor(0, 0);
    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.println("SD Card Init fail.");
    Serial.println("SD Card Init fail.");
  }

  if (EEPROM.read(ADDR_TIMER - 1) == 1) {
    EEPROM.get(ADDR_TIMER, timerArr);
  }

  cordycepsBox.init();
  tIcon.init();
  hIcon.init();
  lIcon.init();
  automode.init();
  manualmode.init();

  pwInput.w = 250;

  clk.set(0, 0, 1, 1, 1111);
  tft.setRotation(1);
  serial();
  drawTopBar();
  // screen.add(SCREEN__MAIN);
  screen.add(SCREEN__MAIN);
  screen.hasJustChanged();

  timer.every(1000, getData);
}

void loop(void) {
  // tap events
  switch (screen.current()) {
    case SCREEN__MAIN:
      screenMain();
      break;
    case SCREEN__SETUP:
      screenSetup();
      break;
    case SCREEN__ENVIR:
      screenEnvirScreen();
      break;
    case SCREEN__WIFI_PASSWORD:
      screenWifiPassword();
      break;
    case SCREEN__HT_INPUT:
      screenHTInputs();
      break;
    case SCREEN__WIFI_LIST:
      screenWifiList();
      break;
    case SCREEN__WIFI_STATUS:
      screenWifiStatus();
      break;
    case SCREEN__DEVICE_STATUS:
      screenDeviceStatus();
      break;
    case SCREEN__LIGHT_SCREEN__SETUP:
      screenLightSetup();
      break;
    case SCREEN__TIMER_LIST:
      screenTimerList();
      break;
    case SCREEN__TIMER_SET:
      screenTimerSet();
      break;
  }
}

void checkTouch() {
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);

  //pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  //pinMode(YM, OUTPUT);

  touched = (p.z > MINPRESSURE);

  if (touched) {
    ty = map(p.x, TS_MINX, TS_MAXX, tft.height(), 0);
    tx = map(p.y, TS_MINY, TS_MAXY, tft.width(), 0);
  }
  serial();
  if (wifi.stt) {
    if (millis() - lastPing > 2000) {
      wifi.stt = false;
      wifi.drawSignal(true);
      if (screen.current() == SCREEN__WIFI_STATUS) {
        screen.pop();
      }
    }
  }

  wifi.onclicked([] {
    screen.add(wifi.stt ? SCREEN__WIFI_STATUS : SCREEN__WIFI_LIST);
  });

  timerArr.tick();
}

void serial() {
  int icmd = cmd.read();
  timer.tick();
  switch (icmd) {
    case 0:  // receive wifi list
      {
        Serial.println("Receive list of wifi:");
        String s = cmd.param;
        wifi.n = 0;
        if (s == "") break;
        for (int i = 0; i < 10; i++) {
          int sp = s.indexOf("\\\\");
          String s2;
          if (sp != -1) {
            s2 = s.substring(0, sp);
          } else {
            s2 = s;
          }
          int sp2 = s.indexOf("::");
          wifi.ssid[i] = s2.substring(0, sp2);
          wifi.rssi[i] = s2.substring(sp2 + 2, s2.length()).toInt();
          wifi.n = wifi.n + 1;
          if (sp != -1) s = s.substring(sp + 2, s.length());
          else break;
        }

        wifi.receivedWifiList = true;

        for (int i = 0; i < wifi.n; i++) {
          Serial.print(wifi.ssid[i]);
          Serial.println(wifi.rssi[i]);
        }
      }
      break;
    case 1:
      {
        String s = cmd.param;

        if (s == "1") {
          wifi.stt = true;
          Serial.println("connected");
          lastPing = millis();
        } else {
          wifi.stt = false;
          Serial.println("disconnected");
        }
        wifi.drawSignal(true);
      }
      break;
    case 2:  // time.<h>:<m>,<w>,<d>/<m>/<y>
      {
        String s = cmd.param;

        int sp1 = s.indexOf(':');
        uint8_t m = s.substring(0, sp1).toInt();
        int sp2 = s.indexOf(',', sp1 + 1);
        clk.hour = s.substring(sp1 + 1, sp2).toInt();
        int sp3 = s.indexOf(',', sp2 + 1);
        clk.dayOfWeek = s.substring(sp2 + 1, sp3).toInt();
        int sp4 = s.indexOf('/', sp3 + 1);
        uint8_t d = s.substring(sp3 + 1, sp4).toInt();
        int sp5 = s.indexOf('/', sp4 + 1);
        clk.month = s.substring(sp4 + 1, sp5).toInt();
        clk.year = s.substring(sp5 + 1, s.length()).toInt();
        if (clk.minute != m) {
          clk.minute = m;
          bool drawDay = (clk.day != d);
          if (drawDay) clk.day = d;
          drawTime(drawDay);
        }
      }
      break;
    case 3:
      {
        String s = cmd.param;
        int str = s.toInt();
        lastPing = millis();
        if (!wifi.stt) {
          Serial2.print("get.wifi;");
          Serial.print("get.wifi;");
        } else {
          wifi.setStrength(str);
          wifi.drawSignal(false);
        }
      }
      break;
    case 4: //connected
      {
        String s = cmd.param;
        int sp = s.indexOf("::");

        wifi.stt = true;
        lastPing = millis();
        wifi.name = s.substring(0, sp);
        wifi.password = s.substring(sp + 2, s.length());
        wifi.drawSignal(true);
      }
      break;
    case 5:  // setpoint
      {
        String s = cmd.param;
        Serial.println("Receive setpoint: " + s);
        int sp = s.indexOf(':');
        String pt = s.substring(0, sp);
        String svalue = s.substring(sp + 1, s.length());
        float value = svalue.toFloat();
        if (pt == "t") {
          temperatureSP = value;
          if (screen.current() == SCREEN__HT_INPUT) {
            tInput.value = trimFloat(temperatureSP);
            hInput.redraw(true);
          }
        } else if (pt == "h") {
          humiditySP = value;
          if (screen.current() == SCREEN__HT_INPUT) {
            hInput.value = trimFloat(humiditySP);
            hInput.redraw(true);
          }
        } else {
          light = value == 0 ? false : true;
          if (screen.current() == SCREEN__MAIN) {
            int xt = 200, yt = 115;
            //if (t_light != light) {
            //light = t_light;
            tft.fillRect(xt - 3, yt - 40, 70, 50, Backcolor);
            text(String(light ? "B\u0090t" : "T\u0087t"), xt, yt, 1, WHITE, &test);
            //}
          }
        }

        Serial1.print(String("setpoint.") + temperatureSP + "," + String(humiditySP) + String(',') + String(light ? "1" : "0") + ";");
        Serial.println(String("setpoint.") + pt + ":" + svalue + ";");
      }
      break;
  }

  switch (cmd2.read()) {
    case 0:  //info
      {
        String s = cmd2.param;
        int comma1 = s.indexOf(',');
        float t_temperature = s.substring(0, comma1).toFloat();

        int comma2 = s.indexOf(',', comma1 + 1);
        float t_humidity = s.substring(comma1 + 1, comma2).toFloat();

        int comma3 = s.indexOf(',', comma2 + 1);
        bool t_light = s.substring(comma2 + 1, comma3) == "1" ? true : false;

        int comma4 = s.indexOf(',', comma3 + 1);
        temperatureSP = s.substring(comma3 + 1, comma4).toFloat();

        int comma5 = s.indexOf(',', comma4 + 1);
        humiditySP = s.substring(comma4 + 1, comma5).toFloat();

        if (screen.current() == SCREEN__MAIN) {
          int xt = 200, yt = 115;

          yt += 50;
          xt -= 36;
          if (humidity != t_humidity) {
            humidity = t_humidity;
            tft.fillRect(xt - 3, yt - 35, 100, 45, Backcolor);
            text( (!(humidity >= 0 && humidity <= 100) ? ".." : String( round(humidity)) ) + String("%"), xt, yt, 1, WHITE, &test);
          }
          yt += 50;
          xt += 29;
          if (temperature != t_temperature) {
            temperature = t_temperature;
            tft.fillRect(xt - 3, yt - 35, 80, 45, Backcolor);
            text((!(temperature > -10 && temperature < 60) ? ".." : String( round(temperature)) ) + String("\u001d"), xt, yt, 1, WHITE, &test);
          }

          if (light != t_light) {
            int xt = 200, yt = 115;
            light = t_light;
            tft.fillRect(xt - 3, yt - 40, 70, 50, Backcolor);
            text(String(light ? "B\u0090t" : "T\u0087t"), xt, yt, 1, WHITE, &test);
          }
        }


        Serial2.print(String("info.") + String(temperature) + String(',') + String(humidity)  + String(',')  + "nope" + ';');
        Serial2.print(String("setpointReverse.") + String(temperatureSP) + String(',') + String(humiditySP) + ';');
        //Serial.print(String("info.") + String(temperature) + String(',') + String(humidity)  + String(',')  + "nope" + ';');
      }
      break;
    case 1: //water
      {
        String s = cmd2.param;
        if (s == "1") {
          prev_outOfWater = false;
        } else {
          prev_outOfWater = true;
        }

        Serial2.print(String("water.") + s + String(";"));

      }
      break;
    case 2: //device status
      {
        deviceStatus = cmd2.param;
        Serial.println(String("status:")  + deviceStatus);
      }
      break;

  }
}
