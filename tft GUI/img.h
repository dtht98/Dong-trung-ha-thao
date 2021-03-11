const String KeypadSource[] = {
  "7", "8", "9",
  "4", "5", "6",
  "1", "2", "3",
  ".", "0" , "Xo\u0081"
};

const String KeyboardCmd[] = {"?123", "aA", " ", "<<", "Ok"}; //X\u00a2a
const uint8_t KeyboardCmdW[] = {1, 1, 5, 1, 2};

const char CapsLeterKeys[3][10] PROGMEM = {
  {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
  { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';'},
  { 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/'},
};

const char LeterKeys[3][10] PROGMEM = {
  {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
  { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';'},
  { 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/'},
};

const char NumKeys[3][10] PROGMEM = {
  { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
  { '-', '/', ':', ';', '(', ')', '$', '&', '@', '\''},
  { '.', ',', '?', '!', '\'', '"', 0, 0, 0, 0}
};

const char SymKeys[3][10] PROGMEM = {
  { '[', ']', '{', '}', '#', '%', '^', '*', '+', '='},
  { '_', '\\', '|', '~', '<', '>', 0, 0, 0, 0},
  { '.', ',', '?', '!', 0, 0, 0, 0, 0, 0}
};

const char textLimit = 25;
char MyBuffer[textLimit];
