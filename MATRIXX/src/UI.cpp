#include "UI.h"
#include "Pinos.h"

void UIController::begin(U8G2 &display) {
  disp = &display;
  disp->setFont(UIFonts::SMALL);
}

void UIController::render(RoutingMatrix &routing) {
  if (!disp) return;
  disp->clearBuffer();
  for (uint8_t j = 0; j < 3; ++j) {
    char label[4];
    sprintf(label, "%s%u", UIStrings::LABEL_OUT, j+1);
    disp->drawStr(UILayout::OUT_LABEL_X0 + j*UILayout::COL_SPACING, UILayout::OUT_LABEL_Y, label);
  }
  for (uint8_t i = 0; i < 5; ++i) {
    char inLabel[6];
    if (i < 3) {
      sprintf(inLabel, "%s%u", UIStrings::LABEL_IN, i+1);
    } else if (i == 3) {
      sprintf(inLabel, "BLE");
    } else {
      sprintf(inLabel, "USB");
    }
    disp->drawStr(UILayout::IN_LABEL_X, UILayout::IN_LABEL_Y0 + i*UILayout::ROW_SPACING, inLabel);
    for (uint8_t j = 0; j < 3; ++j) {
      bool on = routing.get(i, j);
      uint8_t x = UILayout::CELL_X0 + j*UILayout::COL_SPACING;
      uint8_t y = UILayout::CELL_Y0 + i*UILayout::ROW_SPACING;
      uint8_t sz = UILayout::CELL_SIZE;
      if (on) disp->drawBox(x, y, sz, sz); else disp->drawFrame(x, y, sz, sz);
      if (i == selIn && j == selOut) {
        disp->drawFrame(x-UILayout::CURSOR_PAD, y-UILayout::CURSOR_PAD, sz+2*UILayout::CURSOR_PAD, sz+2*UILayout::CURSOR_PAD);
      }
    }
  }
  disp->sendBuffer();
}

void UIController::handleInput(RoutingMatrix &routing) {
  int step = readEncoderStep();
  if (step != 0) {
    if (selectInMode) selIn = (uint8_t)((int)selIn + step + 5) % 5;
    else selOut = (uint8_t)((int)selOut + step + 3) % 3;
    render(routing);
  }
  switch (readButtonEvent()) {
    case BTN_CLICK:
      routing.toggle(selIn, selOut);
      render(routing);
      break;
    case BTN_LONG:
      selectInMode = !selectInMode;
      render(routing);
      break;
    case BTN_NONE:
      break;
  }
}

int UIController::readEncoderStep() {
  static int lastClk = HIGH;
  static int lastDt = HIGH;
  static int accumulator = 0;
  static const int THRESHOLD = 4; // Quantos ticks antes de contar como 1 passo
  
  int clk = digitalRead(ENC_CLK);
  int dt  = digitalRead(ENC_DT);
  int step = 0;
  
  // Detetar mudança em CLK
  if (clk != lastClk) {
    // Gray code decoder: inverter completamente
    int direction = (dt == clk) ? -1 : 1;
    accumulator += direction;
    
    // Só retorna um passo quando acumular o threshold
    if (accumulator >= THRESHOLD) {
      step = 1;
      accumulator = 0;
    } else if (accumulator <= -THRESHOLD) {
      step = -1;
      accumulator = 0;
    }
  }
  
  lastClk = clk;
  lastDt = dt;
  return step;
}

UIController::ButtonEvent UIController::readButtonEvent() {
  static int last = HIGH;
  static unsigned long pressStart = 0;
  static unsigned long lastEvent = 0;
  unsigned long now = millis();
  
  // Debounce de botão: ignorar mudanças rápidas
  if ((now - lastEvent) < 20) return BTN_NONE;
  
  int cur = digitalRead(ENC_SW);
  ButtonEvent ev = BTN_NONE;
  
  if (cur != last) {
    lastEvent = now;
    last = cur;
    
    if (cur == LOW) {
      // Pressionado
      pressStart = now;
    } else {
      // Libertado
      unsigned long dur = now - pressStart;
      ev = (dur >= 600) ? BTN_LONG : BTN_CLICK;
    }
  }
  
  return ev;
}
