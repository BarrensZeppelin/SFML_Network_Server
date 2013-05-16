#include <iostream>
#include <sstream>

#include <conio.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <SFML\Network.hpp>

extern sf::Mutex logMutex;
extern sf::Mutex inputMutex;

void writeToLog(const std::wstring str);

struct inputThreadArgs;

enum Colors;

short int fColors[];
short int bColors[];

void initColors();

void uiLoop();


Colors charToColor(const wchar_t& c);
void parseStringToBuffer(CHAR_INFO* buffer, COORD bufferSize, COORD pos, const std::wstring string, bool wrap = false);