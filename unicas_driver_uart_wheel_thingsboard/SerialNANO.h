/*
 * SerialNANO.h
 *
 *  Created on: 11 Aug 2025
 *      Author: Andrea Roccaccino
 */

#pragma once

#include "SerialDevice.h"

#include "MQTTClient.h"


#include "can.h"

class NanoSerial : public SerialDevice {
	public:
		MqttClient *mqtt_client = nullptr;
		NanoSerial(MqttClient *mqtt_client) : SerialDevice("usb2.0-serial") {
			this->mqtt_client = mqtt_client;
		}

		void processIncoming(const std::vector<uint8_t> &data) override {
			// Parsing frame CAN
			std::cout << "[NANO] Frame ricevuto, len=" << data.size() << "\n";

			if (data.size() == sizeof(can_frame)) {
				can_frame msg;
				memcpy(&msg, data.data(), sizeof(can_frame));


				// Send battery percentage
				// mqtt_client->sendMessage("battery", 100);

				// Send voltage
				// mqtt_client->sendMessage("voltage", 19);

				// Send speed
				// mqtt_client->sendMessage("speed", 130);

				// Send RPM
				// mqtt_client->sendMessage("rpm", 1300);

				// Send amperage
				// mqtt_client->sendMessage("amperage", 130);



				// Do something with the can frame!
				switch(msg.can_id){
					default:
						std::cout << "Ricevuto can e non so cosa farci: ID: " + msg.can_id + " Dati: " + msg.can_dlc + std::endl;
						break;
				}
			}

		}

		std::vector<uint8_t> formatOutgoing() {
			// Qui costruisci un frame CAN e lo restituisci
			return {0x01, 0x02, 0x03}; // esempio
		}
};
