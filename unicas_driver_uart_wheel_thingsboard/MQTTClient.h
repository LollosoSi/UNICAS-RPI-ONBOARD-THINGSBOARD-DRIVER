/*
 * MQTTClient.h
 *
 *  Created on: 11 Aug 2025
 *      Author: Andrea Roccaccino
 */

// This class uses libmosquitto, to compile you need to sudo apt install libmosquitto-dev
#pragma once

#include <mosquitto.h>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

class MqttClient {
	private:
		struct mosquitto *mosq = nullptr;
		std::string client_id;
		std::string host;
		int port = 1883;
		std::string username;
		bool connected = false;

	public:
		std::string topic_one = "";

		MqttClient(const std::string &mqtt_host, int mqtt_port = 1883,
				const std::string &id = "cpp_client", const std::string &user =
						"", std::string topic = "v1/devices/me/telemetry") : host(mqtt_host), port(
				mqtt_port), client_id(id), username(user), topic_one(topic) {
			mosquitto_lib_init();

			mosq = mosquitto_new(client_id.c_str(), true, nullptr);
			if (!mosq) {
				throw std::runtime_error(
						"Failed to create mosquitto client instance");
			}

			if (!username.empty()) {
				mosquitto_username_pw_set(mosq, username.c_str(), nullptr);
			}
		}

		~MqttClient() {
			disconnect();
			if (mosq) {
				mosquitto_destroy(mosq);
				mosq = nullptr;
			}
			mosquitto_lib_cleanup();
		}

		bool connect() {
			int rc = mosquitto_connect(mosq, host.c_str(), port, 60);
			if (rc != MOSQ_ERR_SUCCESS) {
				std::cerr << "MQTT connect failed: " << mosquitto_strerror(rc)
						<< "\n";
				connected = false;
			} else {
				connected = true;
				mosquitto_loop_start(mosq); // Avvia loop in background per gestire rete e ack
			}
			return connected;
		}

		void disconnect() {
			if (connected) {
				mosquitto_disconnect(mosq);
				mosquitto_loop_stop(mosq, true);
				connected = false;
			}
		}

		bool publish(const std::string &topic, const std::string &message,
				int qos = 0, bool retain = false) {
			if (!connected) {
				std::cerr << "MQTT not connected, cannot publish\n";
				return false;
			}

			int rc = mosquitto_publish(mosq, nullptr, topic.c_str(),
					(int) message.size(), message.data(), qos, retain);

			if (rc != MOSQ_ERR_SUCCESS) {
				std::cerr << "MQTT publish failed: " << mosquitto_strerror(rc)
						<< "\n";
				return false;
			}
			return true;
		}

		bool sendMessage(std::string topic, std::string tag, int value) {
			char buffer[128];
			// per intero
			int n = snprintf(buffer, sizeof(buffer), R"({"%s":%d})",
					tag.c_str(), value);
			if (n > 0 && n < (int) sizeof(buffer)) {
				std::cout << "Messaggio intero: " << buffer << "\n";
			}

			return publish(topic, buffer, 1, false);

		}

		bool sendMessage(std::string topic, std::string tag, double value) {
			char buffer[128];
			// per decimale (con 2 cifre decimali)
			int n = snprintf(buffer, sizeof(buffer), R"({"%s":%.2f})",
					tag.c_str(), value);
			if (n > 0 && n < (int) sizeof(buffer)) {
				std::cout << "Messaggio decimale: " << buffer << "\n";
			}

			return publish(topic, buffer, 1, false);

		}

		bool sendMessage(std::string tag, int value) {
			char buffer[128];
			// per intero
			int n = snprintf(buffer, sizeof(buffer), R"({"%s":%d})",
					tag.c_str(), value);
			if (n > 0 && n < (int) sizeof(buffer)) {
				//   std::cout << "Messaggio intero: " << buffer << "\n";
			}

			return publish(topic_one, buffer, 1, false);

		}

		bool sendMessage(std::string tag, double value) {
			char buffer[128];
			// per decimale (con 2 cifre decimali)
			int n = snprintf(buffer, sizeof(buffer), R"({"%s":%.2f})",
					tag.c_str(), value);
			if (n > 0 && n < (int) sizeof(buffer)) {
				//    std::cout << "Messaggio decimale: " << buffer << "\n";
			}

			return publish(topic_one, buffer, 1, false);

		}
};
