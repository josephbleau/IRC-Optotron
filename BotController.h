#pragma once

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <stdio.h>
#include <stdlib.h>

#include <libircclient\libircclient.h>
#include <libircclient\libirc_rfcnumeric.h>
#include <sqlite\sqlite3.h>

#include "CalcDB.h"
#include "HostmaskAuthorizer.h"

namespace IRCOptotron
{

class BotController
{
	static bool _init;

	static irc_callbacks_t _callbacks;
	static irc_session_t* _session;

	static CalcDB* _calc_db;
	static HostmaskAuthorizer* _hostmask_db;

	static std::string _server;
	static std::string _nick;
	static std::string _last_chan;
	static std::vector<std::string> _chanlist;
	
	static void doCalc(const std::string& chan, const std::string& host, const std::vector<std::string>& params);
	static void doCalcVersion(const std::string& chan, const std::string& host, const std::vector<std::string>& params);
	static void doCalcApropos(const std::string& chan, const std::string& host, const std::vector<std::string>& params);
	static void doCalcAproposAll(const std::string& chan, const std::string& host, const std::vector<std::string>& params);
	static void doCalcRemove(const std::string& chan, const std::string& host, const std::vector<std::string>& params);
	static void doChangeCalc(const std::string& chan, const std::string& host, const std::vector<std::string>& params);
	static void doMakeCalc(const std::string& chan, const std::string& host, const std::vector<std::string>& params);

	static void viewHostmasksFor(const std::string& chan, const std::string& host, const std::vector<std::string>& params);
	static void rmHostmask(const std::string& chan, const std::string& host, const std::vector<std::string>& params);
	static void addHostmask(const std::string& chan, const std::string& host, const std::vector<std::string>& params);

	static void sendMessageToHost(const std::string& host, const std::string& msg);
	static void sendMessageToNick(const std::string& nick, const std::string& msg);
	
	BotController(){}
	~BotController(){}
public:
	static void doUserJoined(const std::string& chan, const std::string& host); 
	static void doWhoisNicklist(const std::string& chan, const std::string& nicklist);
	static void doWhoisReceivedCheckAuth(const std::string& host);

	static void parseMessage(const std::string& chan, const std::string& host, const std::string& msg);
	
	static bool start(const std::string& nick, const std::string& server, const std::vector<std::string>& chanlist);

	static std::vector<std::string> getChanList();
};

}