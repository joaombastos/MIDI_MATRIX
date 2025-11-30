#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include <U8g2lib.h>
#include "Pinos.h"

class OLED {
private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C* display;
  bool initialized;
  
public:
  OLED();
  ~OLED();
  
  bool begin();
  void stop();
  bool isInitialized() const { return initialized; }
  
  void clear();
  void update();
  void drawString(uint8_t x, uint8_t y, const char* text);
  void drawStringCentered(uint8_t y, const char* text);  // Centraliza horizontalmente
  void setFont(const uint8_t* font);

  U8G2 &u8g2();
  
};

#endif