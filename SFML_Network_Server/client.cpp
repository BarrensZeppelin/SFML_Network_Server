#include <iostream>

#include <SFML\Network.hpp>

#include <vector>
#include <deque>
#include <sstream>

#include "config.h"

#include "client.h"
#include "server.h"
#include "ui.h"



ClientData::ClientData() {
	name = new wchar_t[MAX_CLIENTNAME_LENGTH];
}



Client::Client() : Socket(), Dead(true), listenThread(0), dataMutex(), cMutex(), ping(0), data() {
}

Client::~Client() {
	if(listenThread!=0) listenThread->wait();
	delete listenThread;
}



ClientData& Client::getData() {
	dataMutex.lock();
	return data;
}

void Client::freeDataAccess() {
	dataMutex.unlock();
}



bool Client::isDead() {
	sf::Lock lock(cMutex);
	return Dead;
}

sf::Uint16 Client::getPing() {
	sf::Lock lock(cMutex);
	return ping;
}

void Client::disconnect() {
	cMutex.lock();
	Dead = true;
	cMutex.unlock();

	std::wstringstream ss; ss << "Socket {YDno. " << Socket.getLocalPort()-Server::Port << "} {RDdisconnected}.";
	writeToLog(ss.str());

	Socket.disconnect();
}

void Client::listen() {
	while(!Dead) {
		sf::Packet received;
		sf::Socket::Status status = Socket.receive(received);
		
		if(status == sf::Socket::Status::Disconnected) {
			disconnect();
		} else if(status == sf::Socket::Status::Error) {
			std::wstringstream ss; ss << "Error on socket {YDno. " << Socket.getLocalPort()-Server::Port << "}. {RDDisconnecting it}.";
			writeToLog(ss.str());
			
			disconnect();
		} else if(status == sf::Socket::Done) {
			
			/////////// Handle received packet.
			sf::Uint16 type;
			received >> type;
			
			sf::Packet send;
			sf::Socket::Status s(sf::Socket::Status::Done);
			switch(type) {
				case Client::PacketTypeIn::TCONNECT:
					send << (sf::Uint16)Client::PacketTypeOut::CONNECTRESPONSE << (sf::Uint16)(Socket.getLocalPort()-Server::Port-1); // Send slot no. to client
					s = Socket.send(send);
					break;

				case Client::PacketTypeIn::PING:
					send << (sf::Uint16)Client::PacketTypeOut::PINGRESPONSE;
					s = Socket.send(send);
					break;
			}
			
			// Disconnect socket if send fails
			if(s == sf::Socket::Status::Disconnected) {
				disconnect();
			} else if(status == sf::Socket::Status::Error) {
				std::wstringstream ss; ss << "Error on socket {YDno. " << Socket.getLocalPort()-Server::Port << "}. {RDDisconnecting it}.";
				writeToLog(ss.str());
				
				disconnect();
			}
		}
	}
}
