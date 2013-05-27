#include <iostream>

#include <SFML\Network.hpp>

#include <vector>
#include <deque>
#include <sstream>

extern sf::Mutex logMutex;
extern std::deque<std::wstring> output;


//Pure static
class Server {
private:
	static sf::UdpSocket Socket; // The socket which recieves game info

	static sf::TcpListener Listener; // Accepts new clients

	static sf::Thread * AccepterThread;
	static sf::Thread * MainThread;
	static sf::Thread * PacketQueueThread;

	static bool Run;
	static bool Listening;

	static sf::Mutex clientAccess;

	static void runAccepter();
	static void run();

	static void handlePacketQueue();
public:
	static bool handleUDPPacket(sf::Packet& packet, const sf::IpAddress& ip, const sf::Uint16 port);

	static sf::Uint16 Port;

	static std::vector<Client *> Clients;

	static bool init();
	static void terminate();

	

	static sf::Uint16 calcAvgPing();

	static bool isSlotOpen(sf::Uint16 slot);

	static sf::Uint16 getSlotsOpen();

	static bool isFull();
};