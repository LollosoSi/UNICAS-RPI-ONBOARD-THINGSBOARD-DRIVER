/*
 * SerialUNO.h
 *
 *  Created on: 11 Aug 2025
 *      Author: Andrea Roccaccino
 */

#pragma once

#include "SerialDevice.h"
#include "VirtualKeyboard.h"

#pragma pack(push, 1)
struct uart_message {
		uint8_t enum_value_action;
		uint32_t content;
};
#pragma pack(pop)

constexpr size_t UART_MESSAGE_SIZE = sizeof(uart_message);

enum UART_MESSAGES {
	PROFILE = 0,
	HORN,
	PTT,
	PIT,
	REVERSE,
	ACCEL_X,
	ACCEL_Y,
	ACCEL_Z,
	GYRO_X,
	GYRO_Y,
	GYRO_Z,
	AMPERAGE,
	BATTERY_VOLTAGE,
	THROTTLE,
	BRAKE
};

class UnoSerial : public SerialDevice {
	public:
		UnoSerial() : SerialDevice("arduino_uno_wifi_r4") {
		}

		void processIncoming(const std::vector<uint8_t> &data) override {
			if (data.size() >= UART_MESSAGE_SIZE) {
				uart_message msg;
				memcpy(&msg, data.data(), UART_MESSAGE_SIZE);
				std::cout << "[UNO] Evento " << int(msg.enum_value_action)
						<< " -> " << msg.content << std::endl;


				if(msg.enum_value_action == PTT){
					sendCommandToScript(msg.content != 0 ? "resume" : "pause");
				}
			}
		}

		std::vector<uint8_t> formatOutgoing(uint8_t action, uint32_t value) {
			uart_message msg { action, value };
			std::vector<uint8_t> bytes(sizeof(msg));
			memcpy(bytes.data(), &msg, sizeof(msg));
			return bytes;
		}
};
