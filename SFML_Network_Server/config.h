#include <fstream>
#include <windows.h>

#include <SFML\Network.hpp>

struct Config {
	// Run-time config
	sf::IpAddress LocalIP;
	sf::IpAddress PublicIP;

	std::fstream logFile;

	COORD bufferSize;


	//IPConfig
	sf::Uint16 port;

	sf::Uint16 max_connections;

	sf::Uint32 timeout;

	//Server
	std::wstring name;
	
	std::wstring password;


	//UI
	sf::Uint16 verbose_level;

	bool logToFile;

	Config() : port(0), max_connections(0), timeout(), name(), password(), LocalIP(sf::IpAddress::None), PublicIP(sf::IpAddress::None), verbose_level(0), logToFile(false), logFile(), bufferSize() {}
};

extern Config config;


extern const sf::Uint16 MAX_SERVERNAME_DISPLAY_LENGTH;
extern const sf::Uint32 MAX_LOG_LENGTH;
extern const sf::Uint16 LOG_INDENT;

extern const sf::Uint16 MAX_CLIENTNAME_LENGTH;