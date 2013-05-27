enum UDPPacketTypeIn;
enum UDPPacketTypeOut;

enum TCPPacketTypeIn;
enum TCPPacketTypeOut;


struct udpPacket;

extern sf::Mutex PQMutex;
extern std::vector<udpPacket> packetQueue;

void addPacketToQueue(sf::Packet packet, sf::IpAddress ip, sf::Uint16 port);