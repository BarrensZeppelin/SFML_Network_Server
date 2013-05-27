#include <SFML\Network.hpp>

#include "config.h"

#include "client.h"
#include "server.h"

#include "packetHandling.h"

#include "ui.h"


enum UDPPacketTypeIn	{CONNECT, PING};
enum UDPPacketTypeOut	{CONNECTSUCCESS, CONNECTFAILURE, PINGRESPONSE};

enum TCPPacketTypeIn	{TCONNECT, TPING};
enum TCPPacketTypeOut	{CONNECTRESPONSE, TPINGRESPONSE};



struct udpPacket {
	sf::Packet packet;
	sf::IpAddress ip;
	sf::Uint16 port;

	sf::Uint16 retries;

	udpPacket(sf::Packet Packet, sf::IpAddress IP, sf::Uint16 Port) : ip(IP), port(Port), retries(0) {packet = Packet;}
};


sf::Mutex PQMutex = sf::Mutex();
std::vector<udpPacket> packetQueue = std::vector<udpPacket>();

void addPacketToQueue(sf::Packet packet, sf::IpAddress ip, sf::Uint16 port) {
	udpPacket pack(packet, ip, port);

	PQMutex.lock();
	packetQueue.push_back(pack);
	PQMutex.unlock();
}

void Server::handlePacketQueue() {
	while(Run) {
		if(packetQueue.empty()) {
			sf::sleep(sf::milliseconds(5));
		} else {
			PQMutex.lock();
			udpPacket pack = packetQueue[0];
			packetQueue.erase(packetQueue.begin());
			PQMutex.unlock();

			sf::Socket::Status status = Socket.send(pack.packet, pack.ip, pack.port);
			if(status == sf::Socket::Done) {
				if(config.verbose_level == 2) {
					std::string ip = pack.ip.toString();
					sf::Packet tmp = pack.packet;
					sf::Uint16 type; tmp >> type;
					std::wstringstream ss; ss << L"{RW--Debug:} {GDSuccessfully} sent packet of type: {YD" << type << L"} to {YD" << std::wstring(ip.begin(), ip.end()) << L"}";
					writeToLog(ss.str());
				}
			} else {
				if(pack.retries < 10) {
					pack.retries++;

					PQMutex.lock();
					packetQueue.push_back(pack);
					PQMutex.unlock();
				} else {
					if(config.verbose_level == 2) {
						std::string ip = pack.ip.toString();
						std::wstringstream ss; ss << L"{RW--Debug:} {RDDumping packet to} {YD" << std::wstring(ip.begin(), ip.end()) << L"} - Error no.: {YD" << sf::Uint16(status) << L"}";
						writeToLog(ss.str());
					}
				}
			}
		}
	}
}



bool Client::handleTCPPacket(sf::Packet& packet) {
	if(packet.getDataSize() <= 0 || !packet) {
		return false;
	}
	
	sf::Uint16 type;
	packet >> type;
	
	sf::Packet send;
	sf::Socket::Status s(sf::Socket::Status::Done);
	switch(type) {
		case TCPPacketTypeIn::TCONNECT:
			send << (sf::Uint16)TCPPacketTypeOut::CONNECTRESPONSE << (sf::Uint16)(Socket.getLocalPort()-Server::Port-1); // Send slot no. to client
			s = Socket.send(send);
			break;

		case TCPPacketTypeIn::TPING:
			sf::Uint16 p;
			packet >> p;

			cMutex.lock();
			ping = p;
			cMutex.unlock();


			send << (sf::Uint16)TCPPacketTypeOut::TPINGRESPONSE;
			s = Socket.send(send);
			break;
	}
	
	// Disconnect socket if send fails
	if(s == sf::Socket::Status::Disconnected) {
		disconnect();
	} else if(s == sf::Socket::Status::Error) {
		std::wstringstream ss; ss << "Error on socket {YDno. " << Socket.getLocalPort()-Server::Port << "}. {RDDisconnecting it}.";
		writeToLog(ss.str());
		
		disconnect();

		return false;
	}
	
	return true;
}



bool Server::handleUDPPacket(sf::Packet& packet, const sf::IpAddress& ip, const sf::Uint16 port) {
	if(packet.getDataSize() <= 0 || !packet) {
		return false;
	}

	sf::Uint16 type;
	packet >> type;
	
	sf::Packet send;
	sf::Socket::Status status(sf::Socket::Done);
	switch(type) {
		case UDPPacketTypeIn::CONNECT: // A guy is trying to connect to the public(server) port, guide him to an open slot.
			{
			std::wstring password;
			packet >> password;
			
			std::string ipString = ip.toString();

			if(password.compare(config.password) == 0 || config.password.compare(L"") == 0) {
				if(isFull()) {
					writeToLog(std::wstring(L"{RDConnect error: Server full} - {YD") + std::wstring(ipString.begin(), ipString.end()) + L"}"); 
					send << (sf::Uint16)UDPPacketTypeOut::CONNECTFAILURE << std::wstring(L"Server is full.");
				} else {
					send << (sf::Uint16)UDPPacketTypeOut::CONNECTSUCCESS << (sf::Uint16)Listener.getLocalPort();
				}
			} else {
				writeToLog(std::wstring(L"{RDConnect error: Wrong password} - {YD") + std::wstring(ipString.begin(), ipString.end()) + L"}"); 
				send << (sf::Uint16)UDPPacketTypeOut::CONNECTFAILURE << std::wstring(L"Connect error: Wrong password.");
			}
			
			
			addPacketToQueue(send, ip, port);
			}
			break;

		case UDPPacketTypeIn::PING:
			{
			send << (sf::Uint16)UDPPacketTypeOut::PINGRESPONSE;
			
			addPacketToQueue(send, ip, port);

			sf::Uint16 slot, ping;
			packet >> slot >> ping;
			
			clientAccess.lock();
			Clients[slot]->setPing(ping);
			clientAccess.unlock();
			}
			break;
	}
	
	return true;
}