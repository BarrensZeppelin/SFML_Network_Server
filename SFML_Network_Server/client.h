

struct ClientData {
	wchar_t * name;

	ClientData();
};



class Client {

	void disconnect();

	sf::Mutex dataMutex;
	sf::Mutex cMutex;

	sf::Uint16 ping;

	ClientData data;

public:
	bool Dead;
	sf::Uint16 getPing();
	void setPing(sf::Uint16 Ping);
	
	bool handleTCPPacket(sf::Packet& packet);

	sf::TcpSocket Socket;
	sf::Thread * listenThread;

	bool isDead();

	
	ClientData& getData();
	void freeDataAccess();

	void listen();

	Client();
	~Client();
};

