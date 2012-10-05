#include <stdio.h>
#include <ctype.h>

#include "BotController.h"
#include "StringHelpers.h"

namespace IRCOptotron
{

typedef struct 
{
	char * channel;
	char * nick;
} irc_ctx_t;

// Static member initialization
bool BotController::_init = false;

CalcDB* BotController::_calc_db = new CalcDB("calc.db");
HostmaskAuthorizer* BotController::_hostmask_db = new HostmaskAuthorizer("hostmasks.db");

irc_session_t* BotController::_session = 0;
irc_callbacks_t BotController::_callbacks;
std::string BotController::_server;
std::string BotController::_nick;
std::string BotController::_last_chan;
std::vector<std::string> BotController::_chanlist;


// Event handler prototypes
void event_connect(irc_session_t * session, const char * event, 
				   const char * origin, const char ** params, 
				   unsigned int count);
void event_channel(irc_session_t * session, const char * event, 
				   const char * origin, const char ** params, 
				   unsigned int count);
void event_join(irc_session_t * session, const char * event, 
				   const char * origin, const char ** params, 
				   unsigned int count);
void event_numeric(irc_session_t * session, unsigned int event, 
				   const char * origin, const char ** params, 
				   unsigned int count);

bool BotController::start(const std::string& nick, const std::string& server, const std::vector<std::string>& chanlist)
{
	if(_init == false)
	{
		_chanlist = chanlist;
		_init = true;

		// Register IRC Event Callbacks
		memset(&_callbacks, 0, sizeof(_callbacks));
		_callbacks.event_connect = event_connect;
		_callbacks.event_channel = event_channel;
		_callbacks.event_join = event_join;
		_callbacks.event_numeric = event_numeric;
		_session = irc_create_session(&_callbacks);

		std::cout << "Attempting to connect to server " << server << "." << std::endl;
		if(irc_connect(_session, server.c_str() ,6667, NULL, nick.c_str(), NULL, NULL))
		{
			std::cout << "Could not connect" << irc_strerror(irc_errno(_session)) << std::endl;
		}

		// NOTE: Anything after irc_run will not be processed until irc_run returns controll
		if(irc_run(_session))
		{
			std::cout << "Could not connect or i/o error: " << irc_strerror(irc_errno(_session));
		}

		return true;
	}

	return false;
}

void BotController::parseMessage(const std::string& chan, const std::string& host, const std::string& msg)
{
	if(!_hostmask_db->isAuthorized(host))
		return;

	std::vector<std::string> tokens = MiscStringHelpers::tokenizeString(msg,' ');
	std::string cmd = tokens.at(0);

	for(unsigned i = 0; i < cmd.length(); i++)
		cmd[i] = tolower(cmd[i]);

	if(cmd == "calc")                             
	{
		doCalc(chan, host, tokens);
	}
	else if(cmd == "chcalc")
	{
		std::vector<std::string> chcalc_params = MiscStringHelpers::tokenizeString(msg,'=');
		doChangeCalc(chan, host, chcalc_params);
	}
	else if(cmd == "rmcalc")
	{
		doCalcRemove(chan, host, tokens);
	}
	else if(cmd == "mkcalc")
	{
		std::vector<std::string> mkcalc_params = MiscStringHelpers::tokenizeString(msg,'=');
		doMakeCalc(chan, host, mkcalc_params);
	}
	else if(cmd == "version")
	{
		doCalcVersion(chan, host, tokens);
	}
	else if(cmd == "apropos")
	{
		doCalcApropos(chan, host, tokens);
	}
	else if(cmd == "apropos_all")
	{
		doCalcAproposAll(chan, host, tokens);
	}
	else if(cmd == "view_hostmasks_for")   
	{
		viewHostmasksFor(chan, host, tokens);
	}
	else if(cmd == "rm_hostmask")
	{
		rmHostmask(chan, host, tokens);
	}
	else if(cmd == "add_hostmask")
	{
		addHostmask(chan, host, tokens);
	}
}


std::vector<std::string> BotController::getChanList()
{
	return _chanlist;
}

void BotController::sendMessageToHost(const std::string& host, const std::string& msg)
{
	char true_nick[256];
	irc_target_get_nick(host.c_str(), true_nick, 256);
	irc_cmd_msg(_session, true_nick, msg.c_str());
}

void BotController::sendMessageToNick(const std::string& nick, const std::string& msg)
{
	irc_cmd_msg(_session, nick.c_str(), msg.c_str());
}


// COMMAND IMPLEMENTATIONS  ----------------------------------------------------

/* doUserJoined is called when a user joins a channel. It checks our sqlite database to 
   see if that users hostmask is authorized, and if he is, he's auto-oped. */
void BotController::doUserJoined(const std::string& chan, const std::string& host)
{
	char nick[32];
	irc_target_get_nick(host.c_str(), nick, 32);

	if(_hostmask_db->isAuthorized(host))
	{
		std::string opcmd = "+o " + std::string(nick);
		irc_cmd_channel_mode(_session, chan.c_str(), opcmd.c_str());
	}
	else if(_hostmask_db->isBanned(host))
	{
		std::string opcmd = "+b " + std::string(nick);
		irc_cmd_channel_mode(_session, chan.c_str(), opcmd.c_str());
	}
}

void BotController::doCalc(const std::string& chan, const std::string& host, const std::vector<std::string>& params)
{
	std::string response;
	std::string msg;
	std::string keyword = MiscStringHelpers::detokenizeString(params, ' ', 1);

	if(params.size() == 1)
	{
		msg = "Usage: calc keyword";
	}
	else if(params.size() >= 2)
	{
		if(_calc_db->getCalc(keyword, response) == CALC_RESPONSE_OK)
		{
			msg = keyword + " = " + response;
		}
		else 
		{
			msg = "Calc '" + keyword + "' not found.";
		}
	}

	sendMessageToNick(chan, msg);
}

void BotController::doCalcVersion(const std::string& chan, const std::string& host, const std::vector<std::string>& params)
{
	std::string response;
	std::string msg1, msg2;
	std::string keyword = MiscStringHelpers::detokenizeString(params, ' ', 2);

	int version = atoi(params[1].c_str());
	
	if(params.size() == 1 || params.size() == 2)
	{
		msg1 = "Usage: version [-]version keyword";
	}
	else if(params.size() > 2)
	{
		if(_calc_db->getVersionInfo(keyword, version, response) == CALC_RESPONSE_OK)
		{
			msg1 = response;
			if(_calc_db->getCalc(keyword, version, response) == CALC_RESPONSE_OK)
			{
				msg2 = keyword + " v" + params[1] + " = " + response;
			}
		}
		else
		{
			msg1 = "Calc '" + keyword + "' v" + params[1] + " not found.";
		}
	}

	sendMessageToNick(chan, msg1);
	if(msg2.size() > 0) sendMessageToNick(chan, msg2);
}

void BotController::doCalcApropos(const std::string& chan, const std::string& host, const std::vector<std::string>& params)
{
	std::string response;
	std::string msg;
	std::string searchterm = MiscStringHelpers::detokenizeString(params, ' ', 1);

	if(params.size() == 1)
	{
		msg = "Usage: apropos search_term";
	}
	else if(params.size() >= 2)
	{
		if(_calc_db->apropos(searchterm, response) != CALC_RESPONSE_NOSEARCHMATCHES)
		{
			msg = "Search results for '"+searchterm+"': "+response;
		}
		else
		{
			msg = "No matches found for '"+searchterm+"'.";
		}
	}
	
	sendMessageToNick(chan, msg);
}

void BotController::doCalcAproposAll(const std::string& chan, const std::string& host, const std::vector<std::string>& params)
{
	std::string response;
	std::string msg;
	std::string searchterm = MiscStringHelpers::detokenizeString(params, ' ', 1);

	if(params.size() == 1)
	{
		msg = "Usage: apropos search_term";
	}
	else if(params.size() >= 2)
	{
		if(_calc_db->apropos_all(searchterm, response) != CALC_RESPONSE_NOSEARCHMATCHES)
		{
			msg = "Search results for '"+searchterm+"': "+response;
		}
		else
		{
			msg = "No matches found for '"+searchterm+"'.";
		}
	}
	
	sendMessageToNick(chan, msg);
}

void BotController::doCalcRemove(const std::string& chan, const std::string& host, const std::vector<std::string>& params)
{
	std::string response;
	std::string msg;
	std::string keyword = MiscStringHelpers::detokenizeString(params, ' ', 1);

	if(params.size() == 1)
	{
		msg = "Usage: rmcalc keyword";
	}
	else if(params.size() >= 2)
	{
		if(_calc_db->removeCalc(keyword) != CALC_RESPONSE_NOCALC)
		{
			msg = "Calc '" + keyword + "' has been deleted.";
		}
		else
		{
			msg = "Calc '" + keyword + "' not found.";
		}
	}

	sendMessageToNick(chan, msg);
}

void BotController::doChangeCalc(const std::string& chan, const std::string& host, const std::vector<std::string>& params)
{
	char nick[256];
	irc_target_get_nick(host.c_str(), nick, 256);

	std::string response;
	std::string msg;
	std::string keyword;
	std::string newcalc;

	if(params.size() != 2)
	{
		msg = "Usage: chcalc keyword = newcalc";
	}
	else
	{
		keyword = MiscStringHelpers::detokenizeString(MiscStringHelpers::tokenizeString(params[0], ' '), ' ', 1);
		newcalc = params[1];

		CalcResponse r = _calc_db->changeCalc(keyword, newcalc, std::string(nick));
		if(r == CALC_RESPONSE_CALCCHANGED)
		{
			msg = "Calc " + keyword + " changed by " + std::string(nick);
		}
		else if(r == CALC_RESPONSE_DBBUSY)
		{
			msg = "CalcDB could not lock DB for writing, busy.";
		}
		else
		{
			msg = "Calc " + keyword + " does not exist";
		}
	}

	sendMessageToNick(chan, msg);
}

void BotController::doMakeCalc(const std::string& chan, const std::string& host, const std::vector<std::string>& params)
{
	char nick[256];
	irc_target_get_nick(host.c_str(), nick, 256);

	std::string response;
	std::string msg;
	std::string keyword;
	std::string newcalc;

	if(params.size() != 2)
	{
		msg = "Usage: mkcalc keyword = newcalc";
	}
	else
	{
		keyword = MiscStringHelpers::detokenizeString(MiscStringHelpers::tokenizeString(params[0], ' '), ' ', 1);
		newcalc = params[1];

		CalcResponse r = _calc_db->makeCalc(keyword, newcalc, std::string(nick));
		if(r == CALC_RESPONSE_CALCCHANGED)
		{
			msg = "Calc " + keyword + " added by " + std::string(nick);
		}
		else if(r == CALC_RESPONSE_DBBUSY)
		{
			msg = "CalcDB could not lock DB for writing, busy.";
		}
		else
		{
			msg = "Calc " + keyword + " already exists (use chcalc)";
		}
	}

	sendMessageToNick(chan, msg);
}

void BotController::viewHostmasksFor(const std::string& chan, const std::string& host, const std::vector<std::string>& params)
{
	std::string msg;
	
	if(params.size() < 3)
	{
		msg = "Usage: view_hostmasks_for [nick] [authorized|banned]";
		sendMessageToNick(chan, msg);
		return;
	}

	std::string nick = params[1];
	std::string type = params[2];
	
	std::vector<std::string> hostmasks;
	
	HostmaskType hostmask_type = HOSTMASK_AUTHORIZED;

	if(type == "authorized")
		hostmask_type = HOSTMASK_AUTHORIZED;
	else if(type == "banned")
		hostmask_type = HOSTMASK_BANNED;
	else
	{
		msg = type + " is not a valid hostmask type.";
		sendMessageToNick(chan, msg);
		return;
	}

	_hostmask_db->getHostmasksByNick(nick, hostmask_type, hostmasks);

	if(hostmasks.size() == 0)
	{
		msg = "Nick '"+nick+"' has no "+type+" hostmasks.";
		sendMessageToNick(chan,msg);
		return;
	}
	else
	{
		msg = "Displaying "+type+" hostmasks for '"+nick+"': ";
		sendMessageToNick(chan, msg);

		for(unsigned i = 0; i < hostmasks.size(); i++)
		{
			msg = "  "+hostmasks[i];
			sendMessageToNick(chan, msg);
		}

		msg = "Use rm_hostmask [id] [authorized|banned] to remove a hostmask.";
		sendMessageToNick(chan, msg);
	}
}

void BotController::rmHostmask(const std::string& chan, const std::string& host, const std::vector<std::string>& params)
{
	std::string msg;
	
	if(params.size() < 3)
	{
		msg = "Usage: rm_hostmask [id] [authorized|banned]";
		sendMessageToNick(chan, msg);
		return;
	}

	std::string id = params[1];
	std::string type = params[2];
		
	HostmaskType hostmask_type = HOSTMASK_AUTHORIZED;

	if(type == "authorized")
		hostmask_type = HOSTMASK_AUTHORIZED;
	else if(type == "banned")
		hostmask_type = HOSTMASK_BANNED;
	else
	{
		msg = type + " is not a valid hostmask type.";
		sendMessageToNick(chan, msg);
		return;
	}

	if(_hostmask_db->removeHostmaskByID(atoi(id.c_str()), hostmask_type) == HOSTMASK_RESPONSE_OK)
	{
		msg = "Hostmask removed.";
	}
	else
	{
		msg = "Hostmask not removed (are you sure that id exists?)";
	}

	sendMessageToNick(chan, msg);
}

void BotController::addHostmask(const std::string& chan, const std::string& host, const std::vector<std::string>& params)
{
	std::string msg;
	
	if(params.size() < 4)
	{
		msg = "Usage: add_hostmask [nick] [mask] [authorized|banned]";
		sendMessageToNick(chan, msg);
		return;
	}

	std::string nick = params[1];
	std::string mask = params[2];
	std::string type = params[3];

	HostmaskType hostmask_type = HOSTMASK_AUTHORIZED;

	if(type == "authorized")
		hostmask_type = HOSTMASK_AUTHORIZED;
	else if(type == "banned")
		hostmask_type = HOSTMASK_BANNED;
	else
	{
		msg = type + " is not a valid hostmask type.";
		sendMessageToNick(chan, msg);
		return;
	}

	if(_hostmask_db->addHostmask(nick, mask, hostmask_type) == HOSTMASK_RESPONSE_OK)
	{
		msg = "Hostmask added.";
	}

	sendMessageToNick(chan, msg);
}

void BotController::doWhoisNicklist(const std::string& chan, const std::string& nicklist)
{
	_last_chan = chan;

	std::vector<std::string> nicks = MiscStringHelpers::tokenizeString(nicklist, ' ');
	for(unsigned i = 0; i < nicks.size(); i++)
	{
		irc_cmd_whois(_session, nicks[i].c_str());
	}
}

void BotController::doWhoisReceivedCheckAuth(const std::string& host)
{
	doUserJoined(_last_chan, host);
}

// EVENT CALLBACKS  ------------------------------------------------------
void event_connect(irc_session_t * session, const char * event, 
				   const char * origin, const char ** params, 
				   unsigned int count)
{
	std::cout << "Connected to server.\n";

	irc_ctx_t * ctx = (irc_ctx_t *) irc_get_ctx(session);

	std::vector<std::string> chanlist = BotController::getChanList();

	for(unsigned i = 0; i < chanlist.size(); i++)
	{
		std::cout << "Attempting to join channel: " << chanlist[i].c_str() << std::endl;
		irc_cmd_join(session, chanlist[i].c_str() , 0);
	}
}

void event_channel(irc_session_t * session, const char * event, 
				   const char * origin, const char ** params, 
				   unsigned int count)
{
	std::string chan = params[0];
	std::string msg = params[1];
	std::string host = origin;

	BotController::parseMessage(chan, host, msg);
}

void event_join(irc_session_t * session, const char * event, 
				   const char * origin, const char ** params, 
				   unsigned int count)
{
	std::string chan = params[0];
	std::string host = origin;

	BotController::doUserJoined(chan, host);
}
void event_numeric( irc_session_t * session, unsigned int event,
				 const char * origin, const char ** params,
				 unsigned int count)
{
	if(count > 0)
	{
		// Odds are we just joined a channel and they've 
		// sent us a list of names. We need to determine who of these
		// people are authorized
		if(event == LIBIRC_RFC_RPL_NAMREPLY)
		{
			std::string chan((char*) params[2]);
			std::string nicklist((char*) params[3]);

			//Annoyingly they send the nicklist with +s and @'s in the names.
			nicklist.erase(std::remove(nicklist.begin(), nicklist.end(), '+'), nicklist.end());
			nicklist.erase(std::remove(nicklist.begin(), nicklist.end(), '@'), nicklist.end());

			BotController::doWhoisNicklist(chan, nicklist);
		}
		else if(event == LIBIRC_RFC_RPL_WHOISUSER)
		{
			BotController::doWhoisReceivedCheckAuth(params[3]);
		}
	}
}

}