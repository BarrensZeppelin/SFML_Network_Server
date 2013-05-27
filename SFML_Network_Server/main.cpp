#include <iostream>
#include <time.h>
#include <sstream>
#include <string>
#include <cstdio>
#include <fstream>

#include "IniTools\IniReader.h"
#include "IniTools\IniWriter.h"

#include <SFML\Network.hpp>


#include "config.h"

#include "client.h"
#include "server.h"																								
#include "ui.h"

sf::Mutex logMutex;
Config config;

std::deque<std::wstring> output;

const sf::Uint16 MAX_SERVERNAME_DISPLAY_LENGTH = 25;
const sf::Uint32 MAX_LOG_LENGTH = 200;
const sf::Uint16 LOG_INDENT = 3;

const sf::Uint16 MAX_CLIENTNAME_LENGTH = 32;


int main(int argc, char * argv[]) {
	std::ifstream configFile("config.ini");
	if(!configFile) { // Really hackish way of creating a file but w/e
		FILE* ptr = new FILE();
		fopen_s(&ptr, "config.ini", "w");
		fclose(ptr);

		CIniWriter iniWriter(".\\config.ini");
		iniWriter.WriteInteger("IPConfig", "port", 0);
		iniWriter.WriteInteger("IPConfig", "max_connections", 6);

		iniWriter.WriteString("Server", "name", "SFML Network Server");
		iniWriter.WriteString("Server", "password", "");

		iniWriter.WriteInteger("UI", "verbose_level", 1);
		iniWriter.WriteBoolean("UI", "logToFile", false);
		std::cout << "Created config.ini" << std::endl;
	} else {configFile.close();}
	
	{
		CIniReader iniReader(".\\config.ini");
		config.port = iniReader.ReadInteger("IPConfig", "port", 0);
		config.max_connections = iniReader.ReadInteger("IPConfig", "max_connections", 6);
		//config.timeout = iniReader.ReadInteger("IPConfig", "timeout", 1000);

		config.name = iniReader.ReadString("Server", "name", "Server");
		config.password = iniReader.ReadString("Server", "password", "");

		config.verbose_level = iniReader.ReadInteger("UI", "verbose_level", 1);
		config.logToFile = iniReader.ReadBoolean("UI", "logToFile", false);
		if(config.logToFile) {config.logFile.open("output.txt", std::fstream::app);}
	}
	
	//Retrieve IP-adress
	config.PublicIP = sf::IpAddress::getPublicAddress(sf::milliseconds(500));
	if(config.LocalIP == sf::IpAddress::None) {config.LocalIP = sf::IpAddress::getLocalAddress();}

	if(Server::init()) {
		//sf::sleep(sf::milliseconds(1000));

		sf::Thread uiThread(uiLoop);
		uiThread.launch();

		
		logMutex.lock();
		output.push_front(L"Server successfully started!");
		for(sf::Uint16 i=0; i<4; i++) {output.push_front(L"");}
		logMutex.unlock();



		// This blocks the 'main' thread untill the uiThread quits
		uiThread.wait();

	} else {
		writeToLog(L"Error, shutting down. Press any key to continue.");
	}
	
	Server::terminate();
	
	return 0;
}