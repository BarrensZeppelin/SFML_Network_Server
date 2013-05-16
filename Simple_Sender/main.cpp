#include <iostream>
#include <string>
#include <sstream>

#include <SFML\Network.hpp>

int main(int argc, char* argv[]) {
	sf::Uint16 port = 0;
	if(argc != 2) {
		port = 7120;
	} else {
		port = atoi(argv[1]);
	}	
	

	
	sf::UdpSocket udpSock;
	udpSock.bind(sf::Socket::AnyPort);
	
	sf::Packet send;
	send << sf::Uint16(0);
	udpSock.send(send, sf::IpAddress::LocalHost, port);

	sf::Packet receive; sf::IpAddress ip; sf::Uint16 Port;
	sf::Socket::Status s = udpSock.receive(receive, ip, Port);
	
	sf::Uint16 OpenSlot(0);
	if(s == sf::Socket::Status::Done) {
		receive >> OpenSlot;
		std::cout << "Slot open on " << OpenSlot << std::endl;
	}

	sf::Uint16 retries = 0;

	
	if(OpenSlot != 0) {
		sf::TcpSocket socket;
		sf::Socket::Status status = sf::Socket::Status::NotReady;
		while(retries < 5 && status != sf::Socket::Status::Done) {
			status = socket.connect(sf::IpAddress::LocalHost, OpenSlot);
			retries++;
		}

		if(status == sf::Socket::Status::Done) {
			std::cout << "Connected." << std::endl;

			// Send a message to the connected host
			sf::Packet packet = sf::Packet();
			std::string message("Hi, I am a client");
			packet << message;
			socket.send(packet);

			packet = sf::Packet();
			status = socket.receive(packet);
			while(status == sf::Socket::Status::Done) {
				std::string str;
				packet >> str;
				std::cout << str << std::endl;
				status = socket.receive(packet);
			}
			
			socket.disconnect();
		} else {
			std::cout << "Failed to connect." << std::endl;
		}
	} else {
		std::cout << "No open slots." << std::endl;
	}

	std::cin.get();

	udpSock.unbind();

	receive.clear();

	return 0;
}