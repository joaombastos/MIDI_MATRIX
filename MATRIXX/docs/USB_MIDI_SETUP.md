# Como Configurar USB MIDI com Adafruit TinyUSB no ESP32-S3

Este documento explica como o **Adafruit TinyUSB** está configurado neste projeto para funcionar como dispositivo USB MIDI no ESP32-S3, com exemplo funcional.

---

## 📋 Requisitos

### Hardware
- **ESP32-S3** (com suporte nativo USB)
- O ESP32-S3 tem USB nativo via GPIO19/GPIO20, sem necessidade de chip externo

### Software / Bibliotecas
```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.usb_mode = 1  # ⚠️ CRÍTICO: Habilita USB OTG
lib_deps = 
    adafruit/Adafruit TinyUSB Library @ ^2.4.0
```

---

## ⚙️ Configuração Essencial

### 1. **PlatformIO.ini - USB Mode**

```ini
board_build.usb_mode = 1
```

**O que faz:**
- `0` = USB CDC/JTAG (debug serial via USB)
- `1` = USB OTG (permite USB MIDI, MSC, HID, etc.)

⚠️ **Sem esta flag, o USB MIDI NÃO funciona!**

### 2. **Build Flags (opcional mas recomendado)**

```ini
build_flags = 
    "-DUSB_PRODUCT=\"MIDI_MATRIX\""
    "-DUSB_MANUFACTURER=\"Joao Bastos\""
```

Define como o dispositivo aparece no sistema operativo.

---

## 💻 Código de Exemplo Funcional

### Setup Básico

```cpp
#include <Adafruit_TinyUSB.h>
#include <USB.h>

// Cria instância global do USB MIDI
Adafruit_USBD_MIDI usb_midi;

void setup() {
  // 1. Inicializa o subsistema USB do ESP32-S3
  USB.begin();
  
  // 2. Define nome do dispositivo (aparece no OS)
  usb_midi.setStringDescriptor("MIDITRIX2000");
  
  // Pronto! O dispositivo USB MIDI está ativo
  delay(1000); // Aguarda enumeração USB
}
```

### Enviar Mensagens MIDI via USB

```cpp
void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  uint8_t status = 0x90 | (channel & 0x0F); // Note On
  usb_midi.write(status);
  usb_midi.write(note);
  usb_midi.write(velocity);
}

void loop() {
  // Exemplo: Envia Dó central (60) no canal 1
  sendNoteOn(0, 60, 127);
  delay(1000);
}
```

### Receber Mensagens MIDI via USB

```cpp
void loop() {
  // Verifica se há dados disponíveis
  if (usb_midi.available()) {
    uint8_t status = usb_midi.read();
    
    if (usb_midi.available()) {
      uint8_t data1 = usb_midi.read();
      
      if (usb_midi.available()) {
        uint8_t data2 = usb_midi.read();
        
        // Processa mensagem (status, data1, data2)
        processMIDI(status, data1, data2);
      }
    }
  }
}
```

---

## 🔧 Implementação Completa (MIDITRIX2000)

### Estrutura do Projeto

```
src/
├── main.cpp                 # Inicialização USB + loop principal
├── MIDIMatrixRouter.cpp     # Processa USB MIDI input/output
└── DINPort.cpp              # Interface MIDI DIN hardware

include/
├── MIDIMatrixRouter.h
└── Config.h
```

### main.cpp - Inicialização

```cpp
#include <Adafruit_TinyUSB.h>
#include <USB.h>

Adafruit_USBD_MIDI usb_midi;
MIDIMatrixRouter* midiRouter = nullptr;

void setup() {
  // PASSO 1: Inicializa USB
  USB.begin();
  usb_midi.setStringDescriptor("MIDITRIX2000");
  
  // PASSO 2: Cria router e passa ponteiro do usb_midi
  midiRouter = new MIDIMatrixRouter(&usb_midi);
  midiRouter->initialize();
}

void loop() {
  // PASSO 3: Processa mensagens no loop
  if (midiRouter) {
    midiRouter->processMessages();
  }
}
```

### MIDIMatrixRouter.cpp - Processamento

```cpp
#include "MIDIMatrixRouter.h"

MIDIMatrixRouter::MIDIMatrixRouter(Adafruit_USBD_MIDI* usb) 
  : usb_midi(usb) {
  // Guarda ponteiro para usar no processamento
}

void MIDIMatrixRouter::processMessages() {
  // Lê mensagens USB MIDI
  if (usb_midi && usb_midi->available()) {
    uint8_t status = usb_midi->read();
    
    if (usb_midi->available()) {
      uint8_t data1 = usb_midi->read();
      
      if (usb_midi->available()) {
        uint8_t data2 = usb_midi->read();
        
        // Roteia para outputs DIN conforme matriz
        processInputMessage(INPUT_IN1, status, data1, data2);
      }
    }
  }
}

void MIDIMatrixRouter::sendToOutput(MIDIOutput output, 
                                    uint8_t status, 
                                    uint8_t data1, 
                                    uint8_t data2) {
  // Exemplo: enviar de volta via USB
  if (usb_midi) {
    usb_midi->write(status);
    usb_midi->write(data1);
    usb_midi->write(data2);
  }
}
```

---

## 🐛 Troubleshooting

### Dispositivo não aparece no sistema

**Problema:** O ESP32-S3 não é reconhecido como MIDI.

**Soluções:**
1. ✅ Confirma `board_build.usb_mode = 1` no `platformio.ini`
2. ✅ Verifica se `USB.begin()` é chamado **antes** de qualquer operação USB
3. ✅ Adiciona `delay(1000)` após `USB.begin()` para enumeração
4. ✅ Testa com cabo USB diferente (alguns só carregam)
5. ✅ No macOS: verifica em "Audio MIDI Setup" → "MIDI Studio"
6. ✅ No Windows: verifica em "Device Manager" → "Sound, video and game controllers"

### Compilação falha com "USB was not declared"

**Problema:** `#include <USB.h>` não encontrado.

**Solução:**
- Framework Arduino ESP32 >= 2.0.5
- Atualiza platform: `platformio platform update espressif32`

### Mensagens MIDI corrompidas

**Problema:** Bytes trocados ou perdidos.

**Solução:**
- Sempre lê **3 bytes** por mensagem (status + data1 + data2)
- Ignora SysEx (`if (status >= 0xF0 && status <= 0xF7) return;`)
- Valida `available()` antes de cada `read()`

### TinyUSB conflito com Serial

**Problema:** `Serial.println()` não funciona após USB MIDI.

**Explicação:**
- Com `usb_mode=1`, o Serial CDC deixa de funcionar via USB
- Use Serial1 em GPIO43/44 (UART) para debug
- Ou use JTAG/OpenOCD para debug

---

## 📊 Comparação: USB CDC vs USB OTG

| Recurso | `usb_mode=0` (CDC) | `usb_mode=1` (OTG) |
|---------|-------------------|-------------------|
| Serial Monitor USB | ✅ Sim | ❌ Não |
| USB MIDI | ❌ Não | ✅ Sim |
| Upload via USB | ✅ Sim | ✅ Sim |
| Debug via USB | ✅ Sim | ❌ Não (usar UART) |

---

## 🎯 Casos de Uso

### 1. Controlador MIDI USB simples
```cpp
void loop() {
  int potValue = analogRead(A0);
  uint8_t ccValue = map(potValue, 0, 4095, 0, 127);
  
  usb_midi.write(0xB0); // CC no canal 1
  usb_midi.write(1);    // CC número 1 (Mod Wheel)
  usb_midi.write(ccValue);
  delay(10);
}
```

### 2. Bridge MIDI DIN ↔ USB
```cpp
void loop() {
  // DIN → USB
  if (dinPort->isAvailable()) {
    uint8_t status = dinPort->readByte();
    uint8_t data1 = dinPort->readByte();
    uint8_t data2 = dinPort->readByte();
    
    usb_midi.write(status);
    usb_midi.write(data1);
    usb_midi.write(data2);
  }
  
  // USB → DIN
  if (usb_midi.available()) {
    uint8_t status = usb_midi.read();
    uint8_t data1 = usb_midi.read();
    uint8_t data2 = usb_midi.read();
    
    dinPort->writeMessage(status, data1, data2);
  }
}
```

### 3. Matriz de Roteamento MIDI (projeto atual)
```cpp
// USB entra como fonte, roteia para 3 outputs DIN
if (usb_midi.available()) {
  uint8_t status = usb_midi.read();
  uint8_t data1 = usb_midi.read();
  uint8_t data2 = usb_midi.read();
  
  // Consulta matriz de roteamento
  uint8_t outputs = routingMatrix->getRouting(INPUT_USB);
  
  if (outputs & OUTPUT_OUT1) dinPort1->write(status, data1, data2);
  if (outputs & OUTPUT_OUT2) dinPort2->write(status, data1, data2);
  if (outputs & OUTPUT_OUT3) dinPort3->write(status, data1, data2);
}
```

---

## 📚 Referências

- [Adafruit TinyUSB Docs](https://github.com/adafruit/Adafruit_TinyUSB_Arduino)
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [MIDI 1.0 Specification](https://www.midi.org/specifications)

---

## ✅ Checklist para Novo Projeto

1. ⬜ Adicionar `adafruit/Adafruit TinyUSB Library` no `lib_deps`
2. ⬜ Definir `board_build.usb_mode = 1` no `platformio.ini`
3. ⬜ Incluir `<Adafruit_TinyUSB.h>` e `<USB.h>` no código
4. ⬜ Criar instância global: `Adafruit_USBD_MIDI usb_midi;`
5. ⬜ Chamar `USB.begin()` no início do `setup()`
6. ⬜ Opcional: `usb_midi.setStringDescriptor("Nome do Dispositivo")`
7. ⬜ No loop: verificar `usb_midi.available()` e processar bytes
8. ⬜ Testar com DAW (Ableton, Logic, etc.) ou `aseqdump -p` (Linux)

---

**Autor:** MIDITRIX2000 Project  
**Plataforma:** ESP32-S3 + PlatformIO  
**Última atualização:** Novembro 2025
