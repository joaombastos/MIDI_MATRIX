#ifndef UI_H
#define UI_H

#include <U8g2lib.h>
#include "RoutingMatrix.h"

namespace UIStrings {
  static const char LABEL_IN[]   = "In";     // prefixo para entradas (curto para caber)
  static const char LABEL_OUT[]  = "Out";    // prefixo para saídas
}

namespace UIFonts {
  static const uint8_t *SMALL = u8g2_font_micro_tr;

}

namespace UILayout {
  static const uint8_t TITLE_X = 0;
  static const uint8_t TITLE_Y = 6;

  // Output labels: Out1, Out2, Out3 centrados
  static const uint8_t OUT_LABEL_X0 = 28;   // X da primeira coluna (deslocada mais para direita)
  static const uint8_t OUT_LABEL_Y  = 12;   // Y dos labels de OUT
  static const uint8_t COL_SPACING  = 26;   // espaçamento entre colunas

  // Input labels: In1-5 (com espaço livre para não colidir com quadrados)
  static const uint8_t IN_LABEL_X   = 10;   // X dos labels de IN (deslocada mais para direita)
  static const uint8_t IN_LABEL_Y0  = 19;   // Y da primeira linha
  static const uint8_t ROW_SPACING  = 9;    // espaçamento entre linhas

  // Células (quadrados) - deslocadas para a direita
  static const uint8_t CELL_X0      = 35;   // X da primeira célula (mais à direita para centralizar)
  static const uint8_t CELL_Y0      = 13;   // Y da primeira célula
  static const uint8_t CELL_SIZE    = 6;    // tamanho da célula quadrada
  static const uint8_t CURSOR_PAD   = 1;    // espessura da moldura do cursor
}

class UIController {
public:
  void begin(U8G2 &display);
  void render(RoutingMatrix &routing);
  void handleInput(RoutingMatrix &routing);
  uint8_t selectedIn() const { return selIn; }
  uint8_t selectedOut() const { return selOut; }
  bool isSelectingIn() const { return selectInMode; }

private:
  U8G2 *disp = nullptr;
  uint8_t selIn = 0;
  uint8_t selOut = 0;
  bool selectInMode = false; // false: OUT, true: IN

  int readEncoderStep();
  enum ButtonEvent { BTN_NONE, BTN_CLICK, BTN_LONG };
  ButtonEvent readButtonEvent();
};

#endif
