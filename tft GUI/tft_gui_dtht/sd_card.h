#ifndef _SD_CARD_
#define _SD_CARD_

/*******************************************************************************
* Definitions
******************************************************************************/

uint16_t read16(File f)
{
  uint16_t d;
  uint8_t b;
  b = f.read();
  d = f.read();
  d <<= 8;
  d |= b;
  return d;
}

uint32_t read32(File f)
{
  uint32_t d;
  uint16_t b;

  b = read16(f);
  d = read16(f);
  d <<= 16;
  d |= b;
  return d;
}

class Image {
  public:
    String fileName;
    uint16_t w, h, bufflen;
    boolean down;
    uint8_t bitDepth, bmpOffset;
    int x, y;

    Image(String fname, uint16_t buffl = 0) {
      bufflen = buffl;
      fileName = fname;
    }
    void init() {
      file = SD.open(fileName);
      if (!file) {
        Serial.println("Cannot find " + fileName);
      } else {
        //--- read header--
        read16(file); //magic byte
        read32(file);// read file size
        read32(file);// read and ignore creator bytes
        bmpOffset = read32(file); //where to start to read color data
        // read DIB header
        read32(file);          //img size
        w = read32(file);             //get w, h
        h = read32(file);
        read16(file);
        bitDepth = read16(file);//bit depth
      }
      file.close();
    }

    void draw(int xx, int yy, bool downn = false) {
      down = downn;
      x = xx; y = yy;
      file = SD.open(fileName);
      if (!file) {
        Serial.println("Cannot find at the second time" + fileName);
      } else {
        if (bufflen == 0) bufflen = w;
        int bufflenB = 2 * bufflen;

        uint8_t sdbuffer[200];   // 50 px  max
        if (!down) {
          file.seek(bmpOffset);
          for (int i = h - 1; i >= 0; i--) { //reversed
            for (int j = 0; j + bufflenB <= 2 * w; j += bufflenB) {
              file.read(sdbuffer, bufflenB);
              uint8_t offset_x = j >> 1; // j/2   ^^
              //uint16_t cl;
              for (int k = 0; k < bufflen; k++) {
                //cl = (sdbuffer[(k << 1) + 1 ] << 8) | sdbuffer[k << 1];
                tft.drawPixel(x + k + offset_x, y + i, (sdbuffer[(k << 1) + 1 ] << 8) | sdbuffer[k << 1]);
              }
            }
          }
        } else {
          uint32_t endpoint = bmpOffset + 2 * w * h;
          for (int i = 0; i < h; i++) {
            for (int j = 0; j + bufflenB <= 2 * w; j += bufflenB) {
              file.seek(endpoint - bufflenB * (i + 1));
              file.read(sdbuffer, bufflenB);
              uint8_t offset_x = j >> 1; // j/2   ^^
              //uint16_t cl;
              for (int k = 0; k < bufflen; k++) {
                //cl = (sdbuffer[(k << 1) + 1 ] << 8) | sdbuffer[k << 1];
                tft.drawPixel(x + k + offset_x, y + i, (sdbuffer[(k << 1) + 1 ] << 8) | sdbuffer[k << 1]);
              }
            }
          }
        }
        file.close();
      }
    }

    bool pressed = false, clicked = false, released = false, prev;
    unsigned long t = millis(), td = t;
    void onclicked(void (*f)()) {
      prev = pressed;
      pressed = touched && (tx >= x && tx < x + w && ty >= y && ty < y + h);
      if (!prev && pressed) { //pressed
        td = millis();
      }
      if (prev && !pressed) { //released
        if (millis() - t > 500 && millis() - td > 20) {
          (*f)();
        }
        t = millis();
      }
    }
};

class ImageFromSRAM {
  public:
    uint16_t* arr;
    uint16_t w, h;

    ImageFromSRAM(uint16_t* dt, uint16_t ww, uint16_t hh);
};

//class Logger {
//  public:
//    String fileName = "log.txt";
//
//    void init() {
//      SD.remove(fileName);
//      log_file = SD.open(fileName, FILE_WRITE);
//      if (!log_file) {
//        Serial.print("error opening log file");
//        return;
//      } else {
//        log_file.println("\u001fi\u009bn");     //          //  194
//      }
//      log_file.close();
//    }
//    void read() {
//      log_file = SD.open(fileName);
//      if (!log_file) {
//        Serial.print("error reading log file");
//        return;
//      } else {
//        log_file.seek(0);
//        Serial.println((uint8_t)log_file.read());
//        Serial.println((uint8_t)log_file.read());
//        Serial.println((uint8_t)log_file.read());
//        Serial.println((uint8_t)log_file.read());
//        Serial.println((uint8_t)log_file.read());
//        Serial.println((uint8_t)log_file.read());
//        Serial.println((uint8_t)log_file.read());
//      }
//      log_file.close();
//    }
//};
//Logger logger;


/*******************************************************************************
* API
******************************************************************************/




#endif /* _SD_CARD_ */
