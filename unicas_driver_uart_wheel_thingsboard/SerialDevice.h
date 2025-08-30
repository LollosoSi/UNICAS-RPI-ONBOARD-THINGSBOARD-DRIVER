/*
 * SerialDevice.h
 *
 *  Created on: 11 Aug 2025
 *      Author: Andrea Roccaccino
 */
#pragma once

#include <iostream>
#include <deque>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <algorithm>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <string.h>

class SerialDevice {
	protected:
		std::string matchPattern;   // parte del nome /dev/serial/by-id
		std::string devicePath;
		int fd = -1;
		int baud;
		bool running = false;
		std::thread ioThread;
		std::deque<std::vector<uint8_t>> inQueue;
		std::deque<std::vector<uint8_t>> outQueue;
		std::mutex mtx;
		std::condition_variable cv;

		int packet_bytes_max = 11;

		std::string findDevice() {
			namespace fs = std::filesystem;
			for (const auto &entry : fs::directory_iterator("/dev/serial/by-id")) {
				std::string path = entry.path();
				std::string lower = path;
				std::transform(lower.begin(), lower.end(), lower.begin(),
						::tolower);
				if (lower.find(matchPattern) != std::string::npos) {
					return path;
				}
			}
			return {};
		}

		int openSerial(const std::string &dev) {
			int fd = open(dev.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
			if (fd < 0)
				return -1;

			struct termios tty { };
			if (tcgetattr(fd, &tty) != 0) {
				close(fd);
				return -1;
			}
			cfsetospeed(&tty, baud);
			cfsetispeed(&tty, baud);
			tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
			tty.c_iflag &= ~IGNBRK;
			tty.c_lflag = 0;
			tty.c_oflag = 0;
			tty.c_cc[VMIN] = 0;
			tty.c_cc[VTIME] = 1;
			tty.c_iflag &= ~(IXON | IXOFF | IXANY);
			tty.c_cflag |= (CLOCAL | CREAD);
			tty.c_cflag &= ~(PARENB | PARODD);
			tty.c_cflag &= ~CSTOPB;
			tty.c_cflag &= ~CRTSCTS;
			if (tcsetattr(fd, TCSANOW, &tty) != 0) {
				close(fd);
				return -1;
			}
			return fd;
		}

		void ioLoop() {
			while (running) {

				// Lettura
				uint8_t buf[256];
				int n = read(fd, buf, sizeof(buf));
				if (n > 0) {
					size_t offset = 0;
					while (offset < static_cast<size_t>(n)) {
						size_t chunkSize = std::min(
								static_cast<size_t>(this->packet_bytes_max),
								static_cast<size_t>(n) - offset);

						std::vector<uint8_t> data(buf + offset,
								buf + offset + chunkSize);
						{
							std::lock_guard < std::mutex > lock(mtx);
							inQueue.push_back(data);
						}
						processIncoming(data);

						offset += chunkSize;
					}
				}

				// Scrittura
				/*std::unique_lock<std::mutex> lock(mtx);
				 if (!outQueue.empty()) {
				 auto &msg = outQueue.front();
				 write(fd, msg.data(), msg.size());
				 outQueue.pop_front();
				 } else {
				 cv.wait_for(lock, std::chrono::milliseconds(10));
				 }*/
			}
		}

	public:
		SerialDevice(std::string matchPattern, int baud = B115200) : matchPattern(
				std::move(matchPattern)), baud(baud) {
		}

		virtual ~SerialDevice() {
			stop();
		}

		void set_max_bytes(int bmax) {
			packet_bytes_max = bmax;
		}

		void start() {
			while (devicePath.empty()) {
				devicePath = findDevice();
				if (devicePath.empty()) {
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}
			}
			fd = openSerial(devicePath);
			if (fd < 0)
				throw std::runtime_error("Errore apertura seriale");
			running = true;
			ioThread = std::thread(&SerialDevice::ioLoop, this);
		}

		void stop() {
			running = false;
			if (ioThread.joinable())
				ioThread.detach();
			if (fd >= 0)
				close(fd);
		}

		void send(const std::vector<uint8_t> &data) {
			write(fd, data.data(), data.size());

			//std::lock_guard<std::mutex> lock(mtx);
			//outQueue.push_back(data);
			//cv.notify_all();
		}

		virtual void processIncoming(const std::vector<uint8_t> &data) = 0; // implementata nelle derivate
		//virtual std::vector<uint8_t> formatOutgoing() = 0;                  // implementata nelle derivate
};
