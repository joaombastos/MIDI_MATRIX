#include <Arduino.h>
#include "Pinos.h"
#include <U8g2lib.h>
#include <Wire.h>
#include <Control_Surface.h>
#include "RoutingMatrix.h"
#include "UI.h"
#include <WiFi.h>
#include <AsyncUDP.h>
#include <OSCMessage.h>
#include <Adafruit_TinyUSB.h>
#include <USB.h>

#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")

const char* OSC_SSID = "ESP32_OSC";
const char* OSC_PASSWORD = "12345678";
const int OSC_LOCAL_PORT = 8000;   // Recebe OSC do TouchOSC
const int OSC_MAC_PORT = 9000;     // Reenvia para Mac
IPAddress OSC_MAC_IP(192, 168, 4, 2);

AsyncUDP osc_udp;

// ===== FORWARD DECLARATIONS =====
void processOSCMessage(OSCMessage &msg, IPAddress remoteIP, uint16_t remotePort);
void sendOSCMessage(const char* address, int value);
void sendOSCMessage(const char* address, float value);




BluetoothMIDI_Interface ble_midi;

Adafruit_USBD_MIDI usb_midi;

HardwareSerial SerialDIN1(1); 
HardwareSerial SerialDIN2(2); 
HardwareSerial SerialDIN3(0); 

RoutingMatrix routing;
UIController ui;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

struct MIDIByteParser {
	uint8_t status = 0;
	uint8_t data1 = 0;
	uint8_t byteCount = 0;
	
	void processByte(uint8_t b) {
		if (b >= 0xF8) {
			ble_midi.sendRealTime(b);
			usb_midi.write(b);  // Envia também para USB
			return;
		}
		
		if (b & 0x80) {
			if (byteCount > 0 && status != 0) {
				sendToBleMIDI();
			}
			status = b;
			byteCount = 1;
			data1 = 0;
		}
		else if (status != 0) {
			byteCount++;
			if (byteCount == 2) {
				data1 = b;
			} else if (byteCount == 3) {
				sendChannelMessage(status, data1, b);
				byteCount = 1;  // Reset para próxima mensagem (running status)
			}
		}
	}
	
	void sendToBleMIDI() {
		if (byteCount == 2) {
			sendChannelMessage(status, data1, 0);
		}
		byteCount = 0;
	}
	
	void sendChannelMessage(uint8_t stat, uint8_t d1, uint8_t d2) {
		uint8_t msgType = stat & 0xF0;
		
		if (msgType >= 0x80 && msgType <= 0xEF) {
			ble_midi.send(ChannelMessage(stat, d1, d2));
			ble_midi.sendNow();
			
			usb_midi.write(stat);
			usb_midi.write(d1);
			usb_midi.write(d2);
		}
	}
};

MIDIByteParser dinParser;

inline void sendToOutput(uint8_t outIndex, uint8_t b) {
	switch (outIndex) {
		case 0: SerialDIN1.write(b); break;
		case 1: SerialDIN2.write(b); break;
		case 2: SerialDIN3.write(b); break;
	}
}

inline void routeByteFromInput(uint8_t inIndex, uint8_t b) {
	for (uint8_t outIndex = 0; outIndex < 3; ++outIndex) {
		if (routing.get(inIndex, outIndex)) {
			sendToOutput(outIndex, b);
		}
	}
}


inline void routeChannelMessage(uint8_t inIndex, uint8_t header, uint8_t data1, uint8_t data2) {
	for (uint8_t outIndex = 0; outIndex < 3; ++outIndex) {
		if (routing.get(inIndex, outIndex)) {
			sendToOutput(outIndex, header);
			sendToOutput(outIndex, data1);
			uint8_t msgType = header & 0xF0;
			if ((msgType >= 0x80 && msgType <= 0xBF) || (msgType >= 0xE0 && msgType <= 0xEF)) {
				sendToOutput(outIndex, data2);
			}
		}
	}
}

inline void routeRealTimeMessage(uint8_t inIndex, uint8_t message) {
	for (uint8_t outIndex = 0; outIndex < 3; ++outIndex) {
		if (routing.get(inIndex, outIndex)) {
			sendToOutput(outIndex, message);
		}
	}
}


void initOSC() {

	WiFi.softAP(OSC_SSID, OSC_PASSWORD);
	IPAddress ip = WiFi.softAPIP();


	if (osc_udp.listen(OSC_LOCAL_PORT)) {

		osc_udp.onPacket([](AsyncUDPPacket packet) {
			OSCMessage msg;
			msg.fill(packet.data(), packet.length());
			
			if (!msg.hasError()) {
				processOSCMessage(msg, packet.remoteIP(), packet.remotePort());
			}
			
			osc_udp.writeTo(packet.data(), packet.length(), OSC_MAC_IP, OSC_MAC_PORT);
		});
	} else {

	}
}

// ===== ENVIAR OSC MESSAGE =====
void sendOSCMessage(const char* address, int value) {
	// Simplicidade: apenas registar no Serial
}

void sendOSCMessage(const char* address, float value) {
	// Simplicidade: apenas registar no Serial

}

void processOSCMessage(OSCMessage &msg, IPAddress remoteIP, uint16_t remotePort) {
	if (msg.match("/matrix/route")) {
		if (msg.size() >= 3) {
			int input = msg.getInt(0);
			int output = msg.getInt(1);
			int value = msg.getInt(2);
			
			if (input >= 0 && input < 5 && output >= 0 && output < 3) {
				if (value) {
					routing.toggle(input, output);
				}
			}
		}
	}
	else if (msg.match("/midi/note")) {
		if (msg.size() >= 3) {
			int input = msg.getInt(0);   // Qual entrada "virtual"
			int note = msg.getInt(1);
			int velocity = msg.getInt(2);
			
			uint8_t header = 0x90;  // Note On channel 1
			routeChannelMessage(input, header, note, velocity);
		}
	}
}

void setup() {
	Wire.begin(OLED_SDA, OLED_SCL);
	u8g2.begin();

	pinMode(DIN1_RX, INPUT_PULLUP);
	pinMode(DIN2_RX, INPUT_PULLUP);
	pinMode(DIN3_RX, INPUT_PULLUP);
	pinMode(DIN1_TX, OUTPUT);
	pinMode(DIN2_TX, OUTPUT);
	pinMode(DIN3_TX, OUTPUT);
	pinMode(ENC_CLK, INPUT_PULLUP);
	pinMode(ENC_DT, INPUT_PULLUP);
	pinMode(ENC_SW, INPUT_PULLUP);

	
	SerialDIN1.begin(MIDI_BAUD_RATE, SERIAL_8N1, DIN1_RX, DIN1_TX, false, 256);
	SerialDIN2.begin(MIDI_BAUD_RATE, SERIAL_8N1, DIN2_RX, DIN2_TX, false, 256);
	SerialDIN3.begin(MIDI_BAUD_RATE, SERIAL_8N1, DIN3_RX, DIN3_TX, false, 256);


	USB.begin();
	delay(1000);  // Aguarda enumeração USB

	routing.setDefaultDiagonal();
	ui.begin(u8g2);

	u8g2.clearBuffer();
	u8g2.setFont(u8g2_font_6x10_tf);
	u8g2.drawStr(0, 10, "MIDI Merdas MKII");
	u8g2.sendBuffer();
	delay(2000);

	ui.render(routing);

	ble_midi.setName("MIDI_MATRIX_BLE");
	Control_Surface.begin();

	initOSC();
}
void loop() {
	for (uint8_t round = 0; round < 3; ++round) {
		MIDIReadEvent evt;
		while ((evt = ble_midi.read()) != MIDIReadEvent::NO_MESSAGE) {
			if (evt == MIDIReadEvent::CHANNEL_MESSAGE) {
				auto msg = ble_midi.getChannelMessage();
				uint8_t header = msg.header;
				uint8_t data1 = msg.data1;
				uint8_t data2 = msg.data2;
				

				if ((header & 0xF0) == 0x90) {  // Note On
					char oscAddr[32];
					sprintf(oscAddr, "/midi/ble/note/%d", data1);
					sendOSCMessage(oscAddr, data2);
				}
				
		
				for (uint8_t outIndex = 0; outIndex < 3; ++outIndex) {
					if (routing.get(3, outIndex)) {  // 3 = BLE input
						sendToOutput(outIndex, header);
						sendToOutput(outIndex, data1);
						uint8_t msgType = header & 0xF0;
						if ((msgType >= 0x80 && msgType <= 0xBF) || (msgType >= 0xE0 && msgType <= 0xEF)) {
							sendToOutput(outIndex, data2);
						}
					}
				}
				
			} else if (evt == MIDIReadEvent::REALTIME_MESSAGE) {
				auto msg = ble_midi.getRealTimeMessage();
				routeRealTimeMessage(3, msg.message);
			}
		}

		Control_Surface.loop();

		if (SerialDIN1.available()) {
			uint8_t b = SerialDIN1.read();
			routeByteFromInput(0, b);
			dinParser.processByte(b);
		}

		if (SerialDIN2.available()) {
			uint8_t b = SerialDIN2.read();
			routeByteFromInput(1, b);
			dinParser.processByte(b);
		}

		if (SerialDIN3.available()) {
			uint8_t b = SerialDIN3.read();
			routeByteFromInput(2, b);
			dinParser.processByte(b);
		}

		if (usb_midi.available()) {
			uint8_t status = usb_midi.read();
			
			if (usb_midi.available()) {
				uint8_t data1 = usb_midi.read();
				
				if (usb_midi.available()) {
					uint8_t data2 = usb_midi.read();
					routeChannelMessage(4, status, data1, data2);
				}
			}
		}
	}

	ui.handleInput(routing);
}

