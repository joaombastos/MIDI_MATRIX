#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include "Pinos.h"

class Encoder {
private:
  int clkPin;
  int dtPin;
  int swPin;
  
  int lastClkState;
  int lastDtState;
  int lastSwState;
  unsigned long lastSwPress;
  unsigned long lastRotationTime;
  int accum; // acumulador de passos
  int stepThreshold; // passos necessários por clique lógico
  int pos; // posição interna de quadratura (0..3)
  int lastCode; // último código de 2 bits (CLK,DT)
  
  static const unsigned long DEBOUNCE_DELAY = 50;
  static const unsigned long ROTATION_DEBOUNCE = 3;  // ms entre leituras de rotação (um pouco maior)
  static const unsigned long MIN_STEP_INTERVAL = 8;  // ms entre passos lógicos para suavidade
  
  int rotationState;  // 0 = idle, 1-4 = estados de transição
  
public:
  Encoder(int clk = ENC_CLK, int dt = ENC_DT, int sw = ENC_SW);
  void setStepThreshold(int threshold);
  
  void begin();
  
  int readRotation();
  
  bool readButton();
  
  bool isButtonPressed();
};

#endif

