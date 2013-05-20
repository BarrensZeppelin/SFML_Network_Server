#include "config.h"

#include "client.h"
#include "server.h"
#include "ui.h"

sf::Mutex inputMutex;


enum Colors {
	D, //Black
	B, //Blue
	R, //Red
	M, //Magenta
	G, //Green
	C, //Cyan
	Y, //Yellow
	W  //White
};

short int fColors[Colors::W +1];
short int bColors[Colors::W +1];

void initColors() {
	fColors[ D ] = 0;
	fColors[ B ] = FOREGROUND_BLUE;
	fColors[ R ] = FOREGROUND_RED | FOREGROUND_INTENSITY;
	fColors[ M ] = FOREGROUND_BLUE | FOREGROUND_RED;
	fColors[ G ] = FOREGROUND_GREEN;
	fColors[ C ] = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
	fColors[ Y ] = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
	fColors[ W ] = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;

	bColors[ D ] = 0;
	bColors[ B ] = BACKGROUND_BLUE;
	bColors[ R ] = BACKGROUND_RED;
	bColors[ M ] = BACKGROUND_BLUE | BACKGROUND_RED;
	bColors[ G ] = BACKGROUND_GREEN;
	bColors[ C ] = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
	bColors[ Y ] = BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
	bColors[ W ] = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
}


void writeToLog(const std::wstring str) {
	logMutex.lock();
	/*if(str.length() > config.bufferSize.X-LOG_INDENT*2) {
		std::wstring s = str;
		while(s.length() > 0) {
			output.push_front(std::wstring(s.begin(), (s.length() > config.bufferSize.X-LOG_INDENT*2 ? s.begin()+config.bufferSize.X-LOG_INDENT*2 : s.end())));
			s.erase(s.begin(), (s.length() > config.bufferSize.X-LOG_INDENT*2 ? s.begin()+config.bufferSize.X-LOG_INDENT*2 : s.end()));
		}
	} else {*/
		output.push_front(str);
	//}

	if(config.logToFile) {std::string s = std::string(str.begin(), str.end()); config.logFile << s << std::endl;}
	logMutex.unlock();
}


Colors charToColor(const wchar_t& c) {
	switch(c) {
		case L'D':
			return D; break;

		case L'B':
			return B; break;
			
		case L'R':
			return R; break;

		case L'M':
			return M; break;

		case L'G':
			return G; break;

		case L'C':
			return C; break;

		case L'Y':
			return Y; break;

		case L'W':
			return W; break;
	}
}


void parseStringToBuffer(CHAR_INFO* buffer, COORD bufferSize, COORD pos, const std::wstring string, bool wrap) {
	sf::Uint32 len = string.length();
	
	sf::Uint32 bufPos = bufferSize.X*pos.Y + pos.X;
	for(sf::Uint32 i = 0; i < len; i++) {
		wchar_t c = string[i];

		if(c == L'{') {
			sf::Uint32 end = string.find(L"}", i);
			if(end != -1 && (end-i)>=3) {
				short f = charToColor(string[i+1]);
				short b = charToColor(string[i+2]);


				WORD attrib = bColors[b] | fColors[f];
				for(sf::Uint32 u = bufPos; u-bufPos < (end-i-3); u++) {
					if(wrap || u < (bufferSize.X*pos.Y + bufferSize.X))
						buffer[u].Attributes = attrib;
				}

				i+=2;
			}
		} else if(c != L'}') {
			if(wrap || bufPos < (bufferSize.X*pos.Y + bufferSize.X)) {
				buffer[bufPos].Char.UnicodeChar = c;
				if(buffer[bufPos].Attributes == 0) {
					buffer[bufPos].Attributes = bColors[D] | fColors[W];
				}
			}
			
			bufPos++;
		}
	}
}



struct inputThreadArgs {
	bool enterPressed;
	std::wstring * input;

	COORD consoleSize;

	bool run;

	inputThreadArgs(COORD size) : enterPressed(false), consoleSize(size), run(true) {input = new std::wstring(L"");}
	~inputThreadArgs() {delete input;}
};


void readInput(inputThreadArgs * args) {
	wchar_t * buff = new wchar_t[args->consoleSize.X];
	while(args->run) {
		if(!args->enterPressed) {
			wchar_t c = _getch();
			if(c == L'\r') {
				args->enterPressed = true;
			} else {
				sf::Lock lock(logMutex);

				if(c == L'\b') {
					if(args->input->length() > 0) args->input->erase(args->input->begin() + args->input->length()-1);
				} else {
					std::wstringstream ss;
					ss << c;
					if(args->input->length() < args->consoleSize.X - 2) args->input->append(ss.str());
				}
			}
		}
	}
}


void uiLoop() {
	initColors();

	//Prepare console
	HANDLE wHnd;
	wHnd = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE rHnd;
	rHnd = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleTitle(L"SFML_NETWORK_SERVER");


	COORD max = GetLargestConsoleWindowSize(wHnd);


	SMALL_RECT windowSize = {0, 0, 99, 43};

	COORD bufferSize = {100, 44};
	config.bufferSize = bufferSize;

	SetConsoleScreenBufferSize(wHnd, bufferSize);
	SetConsoleScreenBufferSize(rHnd, bufferSize);

	SetConsoleWindowInfo(wHnd, TRUE, &windowSize);
	SetConsoleWindowInfo(rHnd, TRUE, &windowSize);

	

	inputThreadArgs args(bufferSize);
	sf::Thread inputThread(readInput, &args);
	inputThread.launch();


	std::wstring command(L"");
	CHAR_INFO* buffer = 0;

	///////////////// UI - loop /////////////
	sf::Clock frameClock;
	
	while(args.run) {
		if(args.enterPressed) {
			inputMutex.lock();
			
			command = *args.input;
			delete args.input;
			args.input = new std::wstring(L"");

			inputMutex.unlock();

			args.enterPressed = false;

			//Process command
			if(command.compare(L"exit") == 0) {
				args.run = false;
				inputThread.terminate();
			} else if(command.compare(L"clear") == 0) {
				logMutex.lock();
				output.clear();
				output.push_front(L"{YDLog cleared.}");
				logMutex.unlock();
			} else {
				writeToLog(std::wstring(L"{RDUnknown Command ('") + command + L"')}"); 
			}
		}

		// Fill the buffer with spaces to clear the screen
		if(buffer != 0) {delete[] buffer;}
		buffer = new CHAR_INFO[args.consoleSize.X*args.consoleSize.Y];
		
		for(sf::Uint16 i = 0; i<args.consoleSize.X*args.consoleSize.Y; i++) {
			buffer[i].Char.UnicodeChar = ' ';
			//if(rand()%2 == 0) buffer[i].Char.UnicodeChar = '0'; else buffer[i].Char.UnicodeChar = '1';
			buffer[i].Attributes = bColors[D];
		}



		{ // TOP DISPLAY
			{
				for(sf::Uint32 i=0; i < bufferSize.X; i++) {
					buffer[i].Attributes = bColors[D] | fColors[W];
				}
				{
					COORD pos = {1, 0};
					std::wstringstream ss;
					ss << L"{RD" << config.name << L"}";
					std::wstring str = ss.str();
					if(str.length() > MAX_SERVERNAME_DISPLAY_LENGTH) {str.erase(str.end()-4, str.end()); str.append(L"...}");}
					parseStringToBuffer(buffer, bufferSize, pos, str);
				}
				
				sf::Uint16 ipStrLen = 0;
				{
					COORD pos = {1+MAX_SERVERNAME_DISPLAY_LENGTH+4, 0};
					std::string ip = config.IP.toString();
					std::wstringstream ss;
					ss << L"{RD" << std::wstring(ip.begin(), ip.end()) << L":" << Server::Port << L"}";
					parseStringToBuffer(buffer, bufferSize, pos, ss.str());
					ipStrLen = ss.str().length();
				}

				{
					COORD pos = {1+MAX_SERVERNAME_DISPLAY_LENGTH+4+ipStrLen+4, 0};
					std::wstringstream ss;
					//ss << L"{RDSlots:} "; if(Server::isFull()) ss << L"{RD"; else ss << L"{GD"; ss << config.max_connections-Server::getSlotsOpen() << L"/" << config.max_connections << L"}";
					parseStringToBuffer(buffer, bufferSize, pos, ss.str());
				}


				{
					std::wstringstream ss;
					ss << "{RDUI: ";

					sf::Int32 elapsed = frameClock.restart().asMilliseconds();
					std::wstringstream ess;
					ess << elapsed;
					std::wstring eString(ess.str());
					if(eString.length()>MAX_SERVERNAME_DISPLAY_LENGTH) eString = L">999";
					else {
						for(sf::Uint16 i = 0; i<4-eString.length(); i++) {
							ss << L" ";
						}
					}
					ss << eString << L"ms}";
					
					COORD pos = {bufferSize.X - 10, 0};
					parseStringToBuffer(buffer, bufferSize, pos, ss.str());
				}
			}


			/*for(sf::Uint32 i=bufferSize.X; i < bufferSize.X*6; i++) {
				buffer[i].Attributes = bColors[W] | fColors[D];
			}*/

			for(sf::Uint32 i=bufferSize.X*6; i<bufferSize.X*8; i++) {
				buffer[i].Attributes = bColors[W] | fColors[D];
			}
		}

		{ // LOG DISPLAY
			sf::Uint16 log_display_rows = bufferSize.Y - 3 - 10;
			
			logMutex.lock();
			if(output.size()>=log_display_rows) {
				COORD pos = {LOG_INDENT, bufferSize.Y-3 - log_display_rows};
				parseStringToBuffer(buffer, bufferSize, pos, L"vvv");
			}
			
			sf::Uint16 y = 0;
			for(sf::Uint32 i = 0; i<output.size() && y<log_display_rows; i++) {
				
				sf::Uint16 u = (unsigned int)(float(output.at(i).length())/(bufferSize.X-LOG_INDENT));
				y+=u;

				COORD pos = {LOG_INDENT, bufferSize.Y-3-y};
				parseStringToBuffer(buffer, bufferSize, pos, output.at(i), true);
				y++;
			}
			logMutex.unlock();

			{
				COORD pos = {LOG_INDENT, bufferSize.Y-2};
				parseStringToBuffer(buffer, bufferSize, pos, L"^^^");
			}
		}

		{ //Bottom input line
			for(sf::Uint16 i = bufferSize.X*(bufferSize.Y-1); i < bufferSize.X*(bufferSize.Y); i++) {
				buffer[i].Attributes = bColors[W] | fColors[D];
			}

			{ //Draw input arrow
				COORD pos = {1, bufferSize.Y-1};
				parseStringToBuffer(buffer, bufferSize, pos, L"> ");
			}

			{ //Draw current input
				COORD pos = {3, bufferSize.Y-1};
				std::wstring s(L"{MW");
				parseStringToBuffer(buffer, bufferSize, pos, s.append((*args.input)).append(L"}"));
			}
		}


		COORD characterPos = {0, 0};
		SMALL_RECT writeArea = {0, 0, args.consoleSize.X-1, args.consoleSize.Y-1};

		WriteConsoleOutput(wHnd, buffer, bufferSize, characterPos, &writeArea);	
	}
}