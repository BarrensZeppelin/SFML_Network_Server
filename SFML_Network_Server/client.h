

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
	enum PacketTypeIn {TCONNECT, PING};
	enum PacketTypeOut {CONNECTRESPONSE, PINGRESPONSE};

	bool Dead;

	sf::TcpSocket Socket;
	sf::Thread * listenThread;

	bool isDead();

	sf::Uint16 getPing();

	ClientData& getData();
	void freeDataAccess();

	void listen();

	Client();
	~Client();
};

