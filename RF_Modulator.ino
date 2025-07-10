#include "Arduino.h"
#include "TM1650.h"
#include "Wire.h"
#include "heltec.h"

#define MOD_ADDRESS 0x65 
TM1650 d;

// GPIO piny pro ovládání modulátoru
#define BTN_1   0   // +kanál
#define BTN_2   12  // -kanál
#define BTN_3   13  // změna zvukové frekvence
#define BTN_4   14  // testovací signál

const byte mod_sound[4] = { 0x00, 0x08, 0x10, 0x18 };

byte Channel = 21, SoundIdx = 1, Test = 1;
unsigned long keyTime = 0;
unsigned long lastKeyPressTime = 0;  
bool displayOn = true;  
//začátek-nastavení displeje, pinů a modulátoru
void setup() {
  Wire.begin();
  Wire.setClock(400000);
  d.init();
  
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
  pinMode(BTN_4, INPUT_PULLUP);

  mod_start(Channel, SoundIdx, Test);
  DisplayUpdate(Channel, SoundIdx, Test);
}
//čtení tlačítek
byte readKeypad() {
  if (!digitalRead(BTN_1)) return BTN_1;
  if (!digitalRead(BTN_2)) return BTN_2;
  if (!digitalRead(BTN_3)) return BTN_3;
  if (!digitalRead(BTN_4)) return BTN_4;
  return 255;
}

void keypadAction() {
  unsigned long currentTime = millis();
  if (currentTime - keyTime < 300) return;

  byte key = readKeypad();
  if (key == 255) return;

  keyTime = currentTime;
  lastKeyPressTime = millis(); 
  displayOn = true; 

  switch (key) {
    //kanál 21 až 69
    case BTN_1:
      if (Channel < 69) Channel++;
      mod_changeChannel(Channel, Test);
      break;
    case BTN_2:
      if (Channel > 21) Channel--;
      mod_changeChannel(Channel, Test);
      break;
      //zvuková frekvence
    case BTN_3:
      SoundIdx = (SoundIdx + 1) % 4;
      mod_setSoundCarrier(SoundIdx);
      break;
      //testovací signál
    case BTN_4:
      Test = !Test;
      mod_changeChannel(Channel, Test);
      break;
  }

  DisplayUpdate(Channel, SoundIdx, Test);
}
//funkce pro správné zobrazení na displejí
void DisplayUpdate(byte channel, byte soundIdx, boolean testMode) {
  static byte lastChannel = 0;
  static byte lastSoundIdx = 0;
  static boolean lastTestMode = false;

  if (channel != lastChannel || soundIdx != lastSoundIdx || testMode != lastTestMode) {
    d.displayOn();
    d.setBrightness(1);
    d.setDot(2, true);
    
    d.displayChar(0, (channel / 10) + '0', false);
    d.displayChar(1, (channel % 10) + '0', false);
    d.displayChar(2, testMode ? 'T' : ' ', false);

    const char soundLetters[] = {'N', 'G', 'I', 'D'};
    d.displayChar(3, soundLetters[soundIdx], false);

    lastChannel = channel;
    lastSoundIdx = soundIdx;
    lastTestMode = testMode;
  }
}
//správné nastavení dat pro I2C
void mod_setFreqBytes(byte &fm, byte &fl, byte channel, boolean testMode) {
  if (channel < 21 || channel > 69) channel = 21;
  uint16_t freq = 32 * channel + 1213;
  fm = freq >> 6;
  if (testMode) fm |= 0x40;
  fl = (freq & 0x3F) << 2;
}
//start modulátoru
void mod_start(byte channel, byte eSoundIdx, boolean testMode) {
  byte fm, fl;
  mod_setFreqBytes(fm, fl, channel, testMode);

  Wire.beginTransmission(MOD_ADDRESS);
  Wire.write(0x88);  
  Wire.write(0x40 | mod_sound[eSoundIdx]);
  Wire.write(fm);
  Wire.write(fl);
  Wire.endTransmission();
}
//změna kanálu
void mod_changeChannel(byte channel, boolean testMode) {
  byte fm, fl;
  mod_setFreqBytes(fm, fl, channel, testMode);
  
  Wire.beginTransmission(MOD_ADDRESS);
  Wire.write(fm);
  Wire.write(fl);
  Wire.endTransmission();
}
//změna frekvence zvuku
void mod_setSoundCarrier(byte idx) {
  Wire.beginTransmission(MOD_ADDRESS);
  Wire.write(0x88);
  Wire.write(0x40 | mod_sound[idx]);
  Wire.endTransmission();
}
//smyčka hlídá klávesnici
void loop() {
  keypadAction();

  // displej se vypne po 5 sekundách->předchází se rušení
  if (displayOn && millis() - lastKeyPressTime > 5000) {
    d.displayOff();
    displayOn = false;
  }
}

