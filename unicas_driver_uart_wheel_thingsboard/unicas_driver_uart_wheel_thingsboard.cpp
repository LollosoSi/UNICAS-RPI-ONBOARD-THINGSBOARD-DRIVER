#include <iostream>

#include "SerialUNO.h"
#include "SerialNANO.h"
#include "UDPReceiver.h"
#include "MQTTClient.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

bool running = true;

void handle_close_c(int s) {
	printf("Caught signal %d\n", s);
	running = false;
}

MqttClient *mqtt_client = nullptr;

int main() {

	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = handle_close_c;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);

	// Configurazione
	const std::string host = "127.0.0.1";
	const int port = 1883;
	const std::string client_id = "cpp_client"; // può essere qualsiasi stringa unica
	const std::string username = "unicar"; // username = device access token in ThingsBoard
	const std::string topic = "v1/devices/me/telemetry";
	const std::string message = R"({"%s":%d})";  // JSON valido

	MqttClient client(host, port, client_id, username, topic);
			mqtt_client = &client;

			if (!client.connect()) {
				std::cerr << "Failed to connect to MQTT broker\n";
				return 1;
			}


	UnoSerial uno;
	uno.set_max_bytes(sizeof(uart_message));
	NanoSerial nano(mqtt_client);
	nano.set_max_bytes(sizeof(can_frame));

	UdpActivityReceiver receiver(*mqtt_client, uno, "127.0.0.1", 55001);

	//uno.start();
	//nano.start();
	receiver.start();

	while (running) {
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}

	int i = 0;
	while (i++ < 500) {
		if (mqtt_client->sendMessage("battery", i - 250)) {
			std::cout << "Messaggio pubblicato con successo" << std::endl;
		} else {
			std::cerr << "Errore durante la pubblicazione" << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));

	}



	mqtt_client->disconnect();


	receiver.stop();
	uno.stop();
	nano.stop();
}

void startserial() {


	// Invia qualcosa
	/*uno.send(uno.formatOutgoing(UART_MESSAGES::PTT, 1));
	uno.send(uno.formatOutgoing(UART_MESSAGES::PTT, 0));
	uno.send(uno.formatOutgoing(UART_MESSAGES::AMPERAGE, 10));
	uno.send(uno.formatOutgoing(UART_MESSAGES::BRAKE, 100));
	uno.send(uno.formatOutgoing(UART_MESSAGES::THROTTLE, 0));*/

	//nano.send(nano.formatOutgoing());

	std::this_thread::sleep_for(std::chrono::seconds(5));


}

