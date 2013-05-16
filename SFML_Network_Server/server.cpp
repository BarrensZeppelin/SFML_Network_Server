#include "config.h"

#include "server.h"

//// CLIENT

Client::Client() : Socket(), Dead(true), listenThread(0) {
	sf::Socket::Status status = sf::Socket::Status::Done;
}

Client::~Client() {
	if(listenThread!=0) listenThread->wait();
	delete listenThread;
}

sf::Socket::Status Client::getStatus() {
	return status;
}

bool Client::isDead() {
	return Dead;
}

void Client::listen() {
	while(!Dead) {
		sf::Packet received;
		status = Socket.receive(received);
		
		if(status == sf::Socket::Status::Disconnected) {
			Dead = true;
			logMutex.lock();
			std::wstringstream ss; ss << "Socket {YDno. " << Socket.getLocalPort()-Server::Port << "} {RDdisconnected}.";
			log.push_front(ss.str());
			logMutex.unlock();
			Socket.disconnect();
		} else if(status == sf::Socket::Status::Error) {
			Dead = true;
			logMutex.lock();
			std::wstringstream ss; ss << "Error on socket {YDno. " << Socket.getLocalPort()-Server::Port << "}. {RDDisconnecting it}.";
			log.push_front(ss.str());
			logMutex.unlock();
			Socket.disconnect();
		} else if(status == sf::Socket::Done) {
			logMutex.lock();
			std::string str;
			received >> str; std::wstring wstr = std::wstring(str.begin(), str.end());
			log.push_front(wstr);
			logMutex.unlock();
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
			logMutex.lock();
			std::wstringstream ss; ss << L"{RDError:} Could not bind to port {YD" << config.port << L"}.";
			log.push_front(ss.str());
			logMutex.unlock();
			return false;
		}
	}

	logMutex.lock();
	std::wstringstream ss; ss << L"Server assigned to port {YD" << Port << L"}";
	log.push_front(ss.str());
	logMutex.unlock();


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
}


void Server::runAccepter() {
	if(!isFull()) {
		sf::Uint16 OpenSlot = 0;
		for(sf::Uint16 i = 1; OpenSlot == 0; i++) {
			if(isSlotOpen(i)) OpenSlot = i;
		}

		logMutex.lock();
		{
		std::wstringstream ss; ss << L"Started listen on port {YD" << Port+OpenSlot << L"}.";
		log.push_front(ss.str());
		}
		logMutex.unlock();
		
		if(Listener.listen(Port+OpenSlot) != sf::Socket::Done) {
			logMutex.lock();
			{
			std::wstringstream ss; ss << "{RDFailed to listen on port} {YD" << Port+OpenSlot << L"}";
			log.push_front(ss.str());
			}
			logMutex.unlock();
		} else {
			delete Clients[OpenSlot-1];
			Clients[OpenSlot-1] = new Client();
			if(Listener.accept(Clients[OpenSlot-1]->Socket) != sf::Socket::Done) {
				logMutex.lock();
				{
				std::wstringstream ss; ss << L"{RDFailed to accept incomming connection on slot} {YDno. " << OpenSlot << L"}.";
				log.push_front(ss.str());
				}
				logMutex.unlock();
			} else {
				logMutex.lock();
				{
				std::wstringstream ss; ss << L"{GDClient connected} on port {YD" << OpenSlot+Port << L"}.";
				log.push_front(ss.str());
				}
				logMutex.unlock();


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
	logMutex.lock();
	log.push_front(L"Main Network Thread started.");
	logMutex.unlock();

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
				case Server::PacketType::CONNECT: // A guy is trying to connect to the public(server) port, guide him to an open slot.
					sf::Packet send;
					send << (sf::Uint16)Listener.getLocalPort();

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


bool Server::isSlotOpen(sf::Uint16 slot) {
	if(slot>0 && slot<=config.max_connections) {
		return Clients[slot-1]->isDead();
	} else {
		logMutex.lock();
		log.push_front(L"{RDInvalid slot request.}");
		logMutex.unlock();
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