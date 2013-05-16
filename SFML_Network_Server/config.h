#include <SFML\Network.hpp>

struct Config {
	// Run-time config
	sf::IpAddress IP;

	//IPConfig
	sf::Uint16 port;

	sf::Uint16 max_connections;

	sf::Uint32 timeout;

	//Server
	std::wstring name;
	
	std::wstring password;

	Config() : port(0), max_connections(0), timeout(), name(), password(), IP(sf::IpAddress::None) {}
};

extern Config config;


extern const sf::Uint16 MAX_NAME_DISPLAY_LENGTH;
extern const sf::Uint32 MAX_LOG_LENGTH;