#include <iostream>

#include <SFML\Network.hpp>

#include <vector>
#include <deque>
#include <sstream>

#include "config.h"

#include "client.h"
#include "server.h"

#include "packetHandling.h"

#include "ui.h"



ClientData::ClientData() {
	name = new wchar_t[MAX_CLIENTNAME_LENGTH];
}



Client::Client() : Socket(), Dead(true), listenThread(0), dataMutex(), cMutex(), ping(0), data() {
	//Socket.setBlocking(false); If I want a non-blocking socket, I will have to make a thread which sends packets in a non-blocking way (NOTREADY handling)
}

Client::~Client() {
	if(listenThread!=0) listenThread->wait();
	delete listenThread;

	Socket.disconnect();
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

void Client::setPing(sf::Uint16 Ping) {
	cMutex.lock();
	ping = Ping;
	cMutex.unlock();
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
	cMutex.lock();
	bool d = Dead;
	cMutex.unlock();
	
	
	while(!d) {
		sf::Packet received;
		sf::Socket::Status status = Socket.receive(received);
		
		if(status == sf::Socket::Disconnected) {
			disconnect();
		} else if(status == sf::Socket::Error) {
			std::wstringstream ss; ss << "Error on socket {YDno. " << Socket.getLocalPort()-Server::Port << "}. {RDDisconnecting it}.";
			writeToLog(ss.str());
			
			disconnect();
		} else if(status == sf::Socket::Done) {
			
			if(handleTCPPacket(received)) {
				//Success
			} else {
				std::wstringstream ss; ss << L"Packet error on slot {YDno. " << Socket.getLocalPort()-Server::Port << L"}.";
				writeToLog(ss.str());
			}
		}

		cMutex.lock();
		d = Dead;
		cMutex.unlock();
	}
}
