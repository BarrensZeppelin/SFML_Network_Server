#include "config.h"

#include "server.h"
#include "ui.h"

//// CLIENT

Client::Client() : Socket(), Dead(true), listenThread(0), mutex(), ping(0) {
}

Client::~Client() {
	if(listenThread!=0) listenThread->wait();
	delete listenThread;
}


bool Client::isDead() {
	return Dead;
}

sf::Uint16 Client::getPing() {
	return ping;
}

void Client::disconnect() {
	mutex.lock();
	Dead = true;
	mutex.unlock();

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


//// SERVER

sf::UdpSocket Server::Socket = sf::UdpSocket();
sf::TcpListener Server::Listener = sf::TcpListener();


sf::Thread * Server::AccepterThread = 0;
sf::Thread * Server::MainThread = 0;

bool Server::Run = true;
bool Server::Listening = false;


std::vector<Client *> Server::Clients = std::vector<Client *>();

sf::Uint16 Server::Port = sf::Uint16();


bool Server::init() {
	
	Socket.setBlocking(false);

	if(config.port==0) {
		Socket.bind(sf::Socket::AnyPort);
		Port = Socket.getLocalPort();
	} else {
		Port = config.port;
		if(Socket.bind(config.port) != sf::Socket::Done) {

			std::wstringstream ss; ss << L"{RDError:} Could not bind to port {YD" << config.port << L"}.";
			writeToLog(ss.str());

			return false;
		}
	}

	std::wstringstream ss; ss << L"Server assigned to port {YD" << Port << L"}";
	writeToLog(ss.str());


	Clients.resize(config.max_connections);
	for(sf::Uint16 i = 0; i<Clients.size(); i++) {
		Clients[i] = new Client();
	}

	AccepterThread = new sf::Thread(&Server::runAccepter);
	
	MainThread = new sf::Thread(&Server::run);
	MainThread->launch();

	return true;
}


void Server::terminate() {
	Run = false;

	for(sf::Uint16 i = 0; i<Clients.size(); i++) {
		delete Clients[i];
	}

	AccepterThread->terminate();
	MainThread->wait();

	if(config.logToFile) {config.logFile << std::endl; config.logFile.close();}
}


void Server::runAccepter() {
	if(!isFull()) {
		sf::Uint16 OpenSlot = 0;
		for(sf::Uint16 i = 1; OpenSlot == 0; i++) {
			if(isSlotOpen(i)) OpenSlot = i;
		}


		{
		std::wstringstream ss; ss << L"Started listen on port {YD" << Port+OpenSlot << L"}.";
		writeToLog(ss.str());
		}
		
		if(Listener.listen(Port+OpenSlot) != sf::Socket::Done) {
			{
			std::wstringstream ss; ss << "{RDFailed to listen on port} {YD" << Port+OpenSlot << L"}";
			writeToLog(ss.str());
			}
		} else {
			delete Clients[OpenSlot-1];
			Clients[OpenSlot-1] = new Client();
			if(Listener.accept(Clients[OpenSlot-1]->Socket) != sf::Socket::Done) {
				{
				std::wstringstream ss; ss << L"{RDFailed to accept incomming connection on slot} {YDno. " << OpenSlot << L"}.";
				writeToLog(ss.str());
				}
			} else {
				{
				std::wstringstream ss; ss << L"{GDClient connected} on port {YD" << OpenSlot+Port << L"}.";
				writeToLog(ss.str());
				}


				Clients[OpenSlot-1]->Dead = false;
				Clients[OpenSlot-1]->listenThread = new sf::Thread(&Client::listen, Clients.at(OpenSlot-1));
				Clients[OpenSlot-1]->listenThread->launch();
			}

			Listener.close();
		}
	}

	Listening = false;
}

void Server::run() {
	writeToLog(L"Main Network Thread started.");

	sf::Clock clock;
	while(Run) {
		if(!isFull() && !Listening) {
			Listening = true;
			AccepterThread->launch();
		}

		sf::Packet packet; sf::IpAddress ip; sf::Uint16 port;
		if(Socket.receive(packet, ip, port) == sf::Socket::Done) {
			sf::Uint16 type;
			packet >> type;

			switch(type) {
				case Server::PacketTypeIn::CONNECT: // A guy is trying to connect to the public(server) port, guide him to an open slot.
					std::wstring password;
					packet >> password;
					
					sf::Packet send;

					std::string ipString = ip.toString();

					if(password.compare(config.password) == 0 || config.password.compare(L"") == 0) {
						if(isFull()) {
							writeToLog(std::wstring(L"{RDConnect error: Server full} - {YD") + std::wstring(ipString.begin(), ipString.end()) + L"}"); 
							send << (sf::Uint16)Server::PacketTypeOut::CONNECTFAILURE << std::wstring(L"Server is full.");
						} else {
							send << (sf::Uint16)Server::PacketTypeOut::CONNECTSUCCESS << (sf::Uint16)Listener.getLocalPort();
						}
					} else {
						writeToLog(std::wstring(L"{RDConnect error: Wrong password} - {YD") + std::wstring(ipString.begin(), ipString.end()) + L"}"); 
						send << (sf::Uint16)Server::PacketTypeOut::CONNECTFAILURE << std::wstring(L"Connect error: Wrong password.");
					}
					

					Socket.send(send, ip, port);
					break;

				//default:
					break;
			}
		}
		
		if(clock.getElapsedTime().asSeconds()>1) {
			//if(rand()%2 == 0) {std::cout << "tick" << std::endl;} else {std::cout << "tock" << std::endl;}
			//std::cout << Listener.getLocalPort() << std::endl;
			clock.restart();
		}
	}
}


sf::Uint16 Server::calcAvgPing() { //Weighted average
	sf::Uint16 temp(0), count(0);
	for(sf::Uint16 i = 0; i < Clients.size(); i++) {
		if(!Clients.at(i)->isDead()) {temp+=Clients.at(i)->getPing()*Clients.at(i)->getPing(); count++;}
	}
	return temp/(count*count);
}


bool Server::isSlotOpen(sf::Uint16 slot) {
	if(slot>0 && slot<=config.max_connections) {
		return Clients[slot-1]->isDead();
	} else {
		writeToLog(L"{RDInvalid slot request.}");

		return false;
	}
}

sf::Uint16 Server::getSlotsOpen() {
	sf::Uint16 open = 0;
	
	for(sf::Uint16 i = 0; i < config.max_connections; i++) {
		if(isSlotOpen(i+1)) open++;
	}

	return open;
}

bool Server::isFull() {
	return (getSlotsOpen() == 0);
}