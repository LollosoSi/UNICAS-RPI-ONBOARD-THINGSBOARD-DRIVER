/*
 * UdpActivityReceiver.h
 *
 *  Created on: 11 Aug 2025
 *      Author: Andrea Roccaccino
 */

#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <iostream>
#include <chrono>

#include "SerialUNO.h"  // deve contenere l'interfaccia che riceve il booleano
#include "MQTTClient.h"

class UdpActivityReceiver {
	private:
		int sock_fd = -1;
		struct sockaddr_in local_addr;
		std::thread recv_thread;
		std::atomic<bool> running { false };
		UnoSerial &uno; // riferimento all'oggetto UnoSerial su cui agire
		MqttClient &client; // MQTT Client for updating link status

	public:
		UdpActivityReceiver(MqttClient &cc, UnoSerial &u,
				const std::string &listen_address, uint16_t listen_port) : uno(
				u), client(cc) {
			sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
			if (sock_fd < 0) {
				throw std::runtime_error(
						"UdpActivityReceiver: Cannot create UDP Socket");
			}



			memset(&local_addr, 0, sizeof(local_addr));
			local_addr.sin_family = AF_INET;
			local_addr.sin_port = htons(listen_port);
			if (inet_pton(AF_INET, listen_address.c_str(), &local_addr.sin_addr)
					<= 0) {
				throw std::runtime_error(
						"UdpActivityReceiver: Invalid listen address: "
								+ listen_address);
			}

			local_addr.sin_addr.s_addr = INADDR_ANY;
			if (bind(sock_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
					sizeof(local_addr)) < 0) {
				close(sock_fd);
				throw std::runtime_error(
						"UdpActivityReceiver: Cannot bind to port");
			}
		}

		~UdpActivityReceiver() {
			stop();
		}

		void start() {
			if (running)
				return;
			running = true;

			// timeout 1s
			struct timeval tv = {1, 0};
			setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

			recv_thread = std::thread(&UdpActivityReceiver::recvLoop, this);
		}

		void stop() {
			running = false;
			if (sock_fd >= 0) {
				close(sock_fd);
				sock_fd = -1;
			}
			if (recv_thread.joinable()) {
				recv_thread.detach();
			}
		}

	private:
		void recvLoop() {
		    uint8_t buf;
		    struct sockaddr_in src_addr;
		    socklen_t addr_len = sizeof(src_addr);

		    using clock = std::chrono::steady_clock;
		    auto lastTrueTime = clock::now();
		    bool lastSentValue = false;

		    while (running) {
		        ssize_t n = recvfrom(sock_fd, &buf, sizeof(buf), 0,
		                             reinterpret_cast<struct sockaddr*>(&src_addr),
		                             &addr_len);

		        if (n == sizeof(buf)) {
		            bool value = (buf != 0);

		            if (value) {
		                lastTrueTime = clock::now();
		                if (!lastSentValue) {
		                    client.sendMessage("link", true);
		                    lastSentValue = true;
		                }
		            }
		            // Ignore false directly here – we'll handle timeout below
		        }
		        else if (n < 0) {
		            if (errno != EAGAIN && errno != EWOULDBLOCK) {
		                perror("UdpActivityReceiver: recvfrom error");
		            }
		        }
		        else {
		            printf("[UDP] Ricevuti %zd byte inattesi\n", n);
		        }

		        // Check if 1 second passed without a true
		        if (lastSentValue &&
		            std::chrono::duration_cast<std::chrono::milliseconds>(
		                clock::now() - lastTrueTime).count() >= 400) {
		            client.sendMessage("link", false);
		            lastSentValue = false;
		        }
		    }
		}


};
