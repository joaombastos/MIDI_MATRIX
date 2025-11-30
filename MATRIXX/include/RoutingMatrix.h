#ifndef ROUTING_MATRIX_H
#define ROUTING_MATRIX_H

#include <stdint.h>

struct RoutingMatrix {
  bool route[5][3];

  void setDefaultDiagonal() {
    for (uint8_t i = 0; i < 5; ++i) {
      for (uint8_t j = 0; j < 3; ++j) {
        if (i < 3) {
          route[i][j] = (i == j);  // DIN diagonal
        } else {
          route[i][j] = (j == 0);  // BLE e USB -> DIN1 por padrão
        }
      }
    }
  }

  void toggle(uint8_t inIndex, uint8_t outIndex) {
    if (inIndex < 5 && outIndex < 3) route[inIndex][outIndex] = !route[inIndex][outIndex];
  }

  bool get(uint8_t inIndex, uint8_t outIndex) const {
    if (inIndex < 5 && outIndex < 3) return route[inIndex][outIndex];
    return false;
  }
};

#endif
