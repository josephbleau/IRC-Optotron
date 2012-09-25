#include "BotController.h"

int main(int argc, char* argv[])
{
	WORD wVersionRequested = MAKEWORD(1,1);
	WSADATA wsaData;

	if(WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		std::cerr << "error starting winsock";
	}

	std::vector<std::string> chanlist;
	chanlist.push_back("#chan1");
	chanlist.push_back("#chan2");

	IRCOptotron::BotController::start("bot", "208.51.40.2", chanlist);

	return 0;
}