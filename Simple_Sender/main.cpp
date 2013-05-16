#include <iostream>
#include <string>
#include <sstream>

#include <SFML\Network.hpp>

enum UdpPacketTypeOut {CONNECT};
enum UdpPacketTypeIn {CONNECTSUCCESS, CONNECTFAILURE};

enum TcpPacketTypeOut {TCONNECT, PING};
enum TcpPacketTypeIn {CONNECTRESPONSE, PINGRESPONSE};


sf::Uint16 ping = 0;

void Ping(bool* run) {
	while(*run) {
		
	}
}

int main(int argc, char* argv[]) {
	sf::Uint16 port = 0;
	if(argc != 2) {
		port = 7120;
	} else {
		port = atoi(argv[1]);
	}	
	
	sf::Uint16 slot;

	
	sf::UdpSocket udpSock;
	udpSock.bind(sf::Socket::AnyPort);
	
	sf::Packet send;
	send << sf::Uint16(UdpPacketTypeOut::CONNECT) << std::wstring(L"yolol");
	udpSock.send(send, sf::IpAddress::LocalHost, port);

	sf::Packet receive; sf::IpAddress ip; sf::Uint16 Port;
	sf::Socket::Status s = udpSock.receive(receive, ip, Port);
	
	sf::Uint16 OpenSlot(0);
	if(s == sf::Socket::Status::Done) {
		sf::Uint16 type;
		receive >> type;

		switch(type) {
			case UdpPacketTypeIn::CONNECTSUCCESS:
				receive >> OpenSlot;
				std::cout << "Slot open on " << OpenSlot << std::endl;
				break;

			case UdpPacketTypeIn::CONNECTFAILURE:
				std::wstring err;
				receive >> err;
				std::cout << std::string(err.begin(), err.end()) << std::endl;
				break;
		}
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
			sf::Packet packet;
			packet << (sf::Uint16)TcpPacketTypeOut::TCONNECT;
			socket.send(packet);

			sf::Packet receive;
			status = socket.receive(receive);
			while(status == sf::Socket::Status::Done) {
				sf::Uint16 type;
				receive >> type;

				switch(type) {
					case TcpPacketTypeIn::CONNECTRESPONSE:
						receive >> slot; // The client now knows which slot it belongs to
						std::cout << "I'm slot " << slot << std::endl;
						break;
				}

				status = socket.receive(packet);
			}
			
			socket.disconnect();
		} else {
			std::cout << "Failed to connect." << std::endl;
		}
	} else {
		std::cout << "No open slots." << std::endl;
	}

	std::cout << "Press enter to continue." <<std::endl;
	std::cin.get();


	udpSock.unbind();

	return 0;
}