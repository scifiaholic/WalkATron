#include <Arduino.h>
#include <BleCombo.h>

BleComboKeyboard bleComboKeyboard("Walker", "Grotts-and-Sons", 100);

uint8_t rowPins[] = {2, 16, 4, 21, 22, 25}; // row pin
uint8_t C12pins[] = {5, 23};                // column pin
uint8_t ABCpins[] = {26, 18, 19};           // multiplexer output pin
uint8_t XYJpins[] = {34, 35, 36, 39};       // analog joystick pin

int16_t midPoints[4] = {0, 0, 0, 0};
#define THH 100                             // Joystick threshold for key presses shmit trigger high
#define THL 80                              // Joystick threshold for key presses shmit trigger low
#define JDV 25                              // Joystick Division Value scalar to slow down mouse moves
#define SDV 500                             // Joystick Division Value scalar to slow down scroll wheel

#define LORANGE 1200                        // Joystick lower than this is the slowest range for mouse
#define HIRANGE 1800                        // Joystick higher than this is the fastest range for mouse
#define LOFACTOR 4                          // Joystick lower range slowness divisor
#define HIFACTOR 4                          // Joystick higher range slowness multiplier

unsigned long scrollTime = 0;               // track the time since the last scroll movement
unsigned long mouseTime = 0;                // track the time since the last mouse movement
unsigned long keyTime = 0;                  // track the time since the last keyboard press 
unsigned long arrowLRTime = 0;              // track the time since the last arrow key joystick up/down movement
unsigned long arrowUDTime = 0;              // track the time since the last arrow key joystick left/right movement

const long mouseWaitTime = 50;              // delay to wait before triggering another mouse event
const long scrollWaitTime = 100;            // delay to wait before triggering another scroll event
const long keyWaitTime = 10;                // delay to wait before triggering another keyboard event
const long arrowWaitTime = 10;              // delay to wait before triggering another arrow key event

uint8_t CAPSLOCK_state = LOW;               // toggle everytime CAPSLOCK is pressed
uint8_t arrows[4] = {LOW, LOW, LOW, LOW};   // arrow keys: UP, DOWN, LEFT, RIGHT
uint8_t state[2][8][6] = {                  // multiplexer pins
  {// 0    1    2    3    4    5    on U1
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO0
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO1
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO2
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO3
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO4
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO5
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO6
    {LOW, LOW, LOW, LOW, LOW, LOW}  // IO7
  },
  {// 0    1    2    3    4    5    on U2
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO0
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO1
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO2
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO3
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO4
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO5
    {LOW, LOW, LOW, LOW, LOW, LOW}, // IO6
    {LOW, LOW, LOW, LOW, LOW, LOW}  // IO7
  }
};

uint8_t keys[2][6][6] = { // Key codes to send on presses - keys[left/right][column][row]
  {                                                                             // chip, addr
    {KEY_RIGHT_SHIFT, '/', '.', ',', 'm', 'n'},                                      //0 - 0, 3 Right 1
    {'\'', ';', 'l', 'k', 'j', 'h'},                                            //1 - 0, 5 Right 2
    {'\\', 'p', 'o', 'i', 'u', 'y'},                                            //2 - 0, 7 Right 3
    {'-', '0', '9', '8', '7', '6'},                                             //3 - 0, 6 Right 4
    {'=', ']', '[', KEY_F9, KEY_F8, KEY_F7},                        //4 - 0, 4 Right 5
    {KEY_BACKSPACE, KEY_DELETE, KEY_END, KEY_HOME, KEY_PAGE_DOWN, KEY_PAGE_UP}, //5 - 1, 4 Right Thumb
  },
  {//    SPACE
    {KEY_LEFT_GUI, KEY_RETURN, ' ', KEY_LEFT_ALT, KEY_LEFT_CTRL, KEY_ESC},             //0 - 1, 6 Left Thumb
    {KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6},                           //1 - 0, 0 Left 5
    {'`', '1', '2', '3', '4', '5'},                                             //2 - 0, 1 Left 4
    {KEY_TAB, 'q', 'w', 'e', 'r', 't'},                                         //3 - 0, 2 Left 3
    {KEY_CAPS_LOCK, 'a', 's', 'd', 'f', 'g'},                                   //4 - 1, 7 Left 2
    {KEY_LEFT_SHIFT, 'z', 'x', 'c', 'v', 'b'}                                   //5 - 1, 5 Left 1
  }
};
String keystrings[2][6][6] = { // Human readable Key codes keystrings[left/right][column][row]
  {                                                                             // chip, addr
    {"RIGHT_SHIFT", "/", ".", ",", "m", "n"},                                         //0 - 0, 3 Right 1
    {"\'", ";", "l", "k", "j", "h"},                                            //1 - 0, 5 Right 2
    {"\\", "p", "o", "i", "u", "y"},                                            //2 - 0, 7 Right 3
    {"-", "0", "9", "8", "7", "6"},                                             //3 - 0, 6 Right 4
    {"=", "]", "[", "F9", "F8", "F7"},                                    //4 - 0, 4 Right 5
    {"BACKSPACE", "DELETE", "END", "HOME", "PAGE_DOWN", "PAGE_UP"},             //5 - 1, 4 Right Thumb
  },
  {
    {"LEFT_GUI", "RETURN", "SPACE", "LEFT_ALT", "LEFT_CTRL", "ESC"},                 //0 - 1, 6 Left Thumb
    {"F1", "F2", "F3", "F4", "F5", "F6"},                                       //1 - 0, 0 Left 5
    {"`", "1", "2", "3", "4", "5"},                                             //2 - 0, 1 Left 4
    {"TAB", "q", "w", "e", "r", "t"},                                           //3 - 0, 2 Left 3
    {"CAPS_LOCK", "a", "s", "d", "f", "g"},                                     //4 - 1, 7 Left 2
    {"LEFT_SHIFT", "z", "x", "c", "v", "b"}                                     //5 - 1, 5 Left 1
  }
};

uint8_t keyMapSides[2][8] = { // map multiplexer U1 and U2 to the Right or Left hand keys
   //0, 1, 2, 3, 4, 5, 6, 7 //C1 = 0
    {1, 1, 1, 0, 0, 0, 0, 0},
   //0, 1, 2, 3, 4, 5, 6, 7 //C2 = 1
    {3, 2, 2, 3, 0, 1, 1, 1} // 3 = disconnected, 2 = thumb sticks
};
uint8_t keyMapColumns[2][8] = { // map multiplexer address to columns
   //0, 1, 2, 3, 4, 5, 6, 7 //C1 = 0
    {1, 2, 3, 0, 4, 1, 3, 2},
   //0, 1, 2, 3, 4, 5, 6, 7 //C2 = 1
    {0, 0, 0, 0, 5, 5, 0, 4}
};

// Read the keyboard
void handleReading(uint8_t chip, uint8_t addr, uint8_t row, uint8_t reading) {
  uint8_t side = keyMapSides[chip][addr];
  uint8_t col = keyMapColumns[chip][addr];
  //compare current reading to last known state, if different handle it
  if(reading != state[chip][addr][row]){
    if(reading == HIGH) {
      Serial.printf("key[%i][%i][%i]: pressed ", chip, addr, row);
      if(chip == 1 && addr == 1){
        Serial.printf("MOUSE_LEFT_CLICK \n");
        Mouse.press(MOUSE_LEFT);
      }
      else if(chip == 1 && addr == 2){
        Serial.printf("MOUSE_RIGHT_CLICK \n");
        Mouse.press(MOUSE_RIGHT);
      }
      else {
        // toggle our CAPSLOCK tracker
        if(keys[side][col][row] == KEY_CAPS_LOCK){
          CAPSLOCK_state = !CAPSLOCK_state;
          Serial.printf("%s CAPSLOCK_STATE: %d \n", keystrings[side][col][row], CAPSLOCK_state);
        } else {
          Serial.printf("%s \n", keystrings[side][col][row]);
        }
        Keyboard.press(keys[side][col][row]);
      }
    }
    else {
      Serial.printf("key[%i][%i][%i]: released ", chip, addr, row);
      if(chip == 1 && addr == 1){
        Serial.printf("MOUSE_LEFT_CLICK \n");
        Mouse.release(MOUSE_LEFT);
      }
      else if(chip == 1 && addr == 2){
        Serial.printf("MOUSE_RIGHT_CLICK \n");
        Mouse.release(MOUSE_RIGHT);
      }
      else {
        Serial.printf("%s \n", keystrings[side][col][row]);
        Keyboard.release(keys[side][col][row]);
      }
    }
    state[chip][addr][row] = reading;
  }
}

void Joy2KeyHigh(int16_t val, uint8_t arrow_index, uint8_t midPoint_index, String text, uint8_t key) {
  if((val > THH) && (arrows[arrow_index] == LOW)){
    arrows[arrow_index] = HIGH;
    Serial.printf("%s pressed, val: %d\n", text, val);
    Keyboard.press(key);
  } 
  else if((val <= THL) && (arrows[arrow_index] == HIGH)){
    arrows[arrow_index] = LOW;
    Serial.printf("%s released, val: %d\n", text, val);
    Keyboard.release(key);
  }
}
void Joy2KeyLow(int16_t val, uint8_t arrow_index, uint8_t midPoint_index, String text, uint8_t key) {
  if((val < -THH) && (arrows[arrow_index] == LOW)){
    arrows[arrow_index] = HIGH;
    Serial.printf("%s pressed, val: %d\n", text, val);
    Keyboard.press(key);
  } 
  else if((val >= -THL) && (arrows[arrow_index] == HIGH)){
    arrows[arrow_index] = LOW;
    Serial.printf("%s released, val: %d\n", text, val);
    Keyboard.release(key);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting!");

  Keyboard.begin();
  Mouse.begin();

  for(uint8_t i=0; i<6; i++) pinMode(rowPins[i], INPUT);
  for(uint8_t i=0; i<2; i++) pinMode(C12pins[i], OUTPUT);
  for(uint8_t i=0; i<3; i++) pinMode(ABCpins[i], OUTPUT);
  for(uint8_t i=0; i<4; i++) pinMode(XYJpins[i], INPUT);

  // center joysticks
  int sampleSize = 100;
  int16_t samples[4] = {0, 0, 0, 0};
  for (int i = 0; i < sampleSize; i ++) {
    for (int j = 0; j < 4; j++) {
      samples[j] += analogRead(XYJpins[j])-2048;
    }
  }

  Serial.printf("Thresholds: ");
  for (int j = 0; j < 4; j++) {
    midPoints[j] = samples[j]/sampleSize;
    Serial.printf("%d ", midPoints[j]);
  }
  Serial.println();
}

void loop() {
  uint8_t reading = 0;                                    // value read from the current key digital pin
  unsigned long currentTime = millis();                   // get the current time of the chip clock

  int16_t XX = analogRead(XYJpins[0])-2048-midPoints[0];  // Left  (X) Joystick Horizontal (X) axis
  int16_t XY = analogRead(XYJpins[1])-2048-midPoints[1];  // Left  (X) Joystick Vertical   (Y) axis
  int16_t YY = analogRead(XYJpins[2])-2048-midPoints[2];  // Right (Y) Joystick Horizontal (X) axis
  int16_t YX = analogRead(XYJpins[3])-2048-midPoints[3];  // Right (Y) Joystick Vertical   (Y) axis

  if(abs(XY) <= THH) XY = 0;                              // Less than +/- higher threshold, ignore
  int8_t signXY = (XY == 0) ? 0 : XY/abs(XY);             // calculate sign +/-, avoid DIVBYZERO
  int8_t signYY = (YY == 0) ? 0 : YY/abs(YY);             // calculate sign +/-, avoid DIVBYZERO
  int8_t signYX = (YX == 0) ? 0 : YX/abs(YX);             // calculate sign +/-, avoid DIVBYZERO
  int16_t scrollXY = 0;

  if(currentTime - arrowLRTime >= arrowWaitTime){         // wait between arrow key presses to regulate speed
    Joy2KeyHigh(XX, 0, 0, "RIGHT", KEY_RIGHT_ARROW);      // RIGHT Arrow Key Presses using Joystick
    Joy2KeyLow(XX, 1, 0, "LEFT", KEY_LEFT_ARROW);         // LEFT Arrow Key Presses using Joystick
    arrowLRTime = currentTime;                            // update the time we last used the left and right arrow keys
  }

  if(CAPSLOCK_state){                                     // if CAPSLOCK OFF then...
    if(currentTime - scrollTime >= scrollWaitTime){       // wait between mouse moves to regulate speed
      scrollXY = signXY + XY/SDV;                         // use scroll wheel with XY axis
      scrollTime = currentTime;                           // update the time we last used the scroll wheel
    }
  }
  else {
    if(currentTime - arrowUDTime >= arrowWaitTime){       // wait between arrow key presses to regulate speed
      Joy2KeyHigh(XY, 2, 1, "DOWN", KEY_DOWN_ARROW);      // UP Arrow Key Presses using Joystick
      Joy2KeyLow(XY, 3, 1, "UP", KEY_UP_ARROW);           // DOWN Arrow Key Presses using Joystick
      arrowUDTime = currentTime;                          // update the time we last used the up and down arrow keys
    }
  }
  
  if(((abs(YX) > THH) || (abs(YY) > THH) || scrollXY) && (currentTime - mouseTime >= mouseWaitTime)){ // if joysticks are active
    if((abs(YX) < LORANGE)) YX/=LOFACTOR;                    // in the middle of the joystick range, reduce speed by a factor of 2 or so
    if((abs(YY) < LORANGE)) YY/=LOFACTOR;                    // in the middle of the joystick range, reduce speed by a factor of 2 or so
    if((abs(YX) > HIRANGE)) YX*=HIFACTOR;                    // in the outside of the joystick range, increase speed by a factor of 2 or so
    if((abs(YY) > HIRANGE)) YY*=HIFACTOR;                    // in the outside of the joystick range, increase speed by a factor of 2 or so

    Serial.printf("Mouse YX:%i:%i, YY:%i:%i, XY:%i:%i\n", YX, signYX + YX/JDV, YY, signYY + YY/JDV, XY, scrollXY);
    Mouse.move(signYX + YX/JDV, signYY + YY/JDV, -scrollXY); // use scroll wheel with XY axis
    mouseTime = currentTime;
  }

  if(currentTime - keyTime >= keyWaitTime){
    for(uint8_t col=0; col<2; col++){                         // loop through the two sets of columns
      for(uint8_t addr=0; addr<8; addr++){                    // loop through the eight sets of output addresses per column
        digitalWrite(ABCpins[0], HIGH && (addr & B00000001)); // set address pins if they match the loop counter using a mask
        digitalWrite(ABCpins[1], HIGH && (addr & B00000010)); // bitwise AND a bit mask for each pin, then logically AND it with HIGH
        digitalWrite(ABCpins[2], HIGH && (addr & B00000100));

        if(col == 1 && (addr == 1 || addr == 2)){             // joystick buttons are inputs not outputs on U1 and IO1/IO2 
          pinMode(C12pins[1], INPUT);                         // convert U1 common pin to input
          reading = digitalRead(C12pins[1]);                  // read pin as input
          pinMode(C12pins[1], OUTPUT);                        // convert back to output for other keys
          handleReading(col, addr, 0, reading);               // compare current reading to last known state, if different handle it
        } else {
          digitalWrite(C12pins[0], HIGH && !col);             // set column pins high if they match the loop counter using a mask
          digitalWrite(C12pins[1], HIGH && col);              // since there are only two columns, we can simply NOT the column counter 

          for(uint8_t row=0; row<6; row++){                   // loop through each row and read if any buttons in this column is pressed
            reading = digitalRead(rowPins[row]);              // read each row output value
            handleReading(col, addr, row, reading);           // compare current reading to last known state, if different handle it
          }

          digitalWrite(C12pins[0], LOW);                      // reset the column pins to low before switching to next address
          digitalWrite(C12pins[1], LOW);
          keyTime = currentTime;                              // let the pins settle
        }
      }
    }
  }
}
