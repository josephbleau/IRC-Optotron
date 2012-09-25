#include "HostmaskAuthorizer.h"
#include "StringHelpers.h"

#include <iostream>

namespace IRCOptotron
{

HostmaskAuthorizer::HostmaskAuthorizer(std::string db_filename)
{
	_db = 0;

	if(sqlite3_open(db_filename.c_str(), &_db) != SQLITE_OK)
	{
		// TODO: Autogenerate DB if it's not found.
		std::cerr << "Error opening database " << db_filename << std::endl;
	}

	std::cout << "Created";
}

HostmaskAuthorizer::~HostmaskAuthorizer()
{
	if(_db)
	{
		sqlite3_close(_db);
	}
}

HostmaskResponse HostmaskAuthorizer::removeHostmaskByID(const int& id, HostmaskType type)
{
	if(!_db)
		return HOSTMASK_RESPONSE_NODB;

	std::string table = "";
	if(type == HOSTMASK_AUTHORIZED)
		table = "authorized_hostmasks";
	else
		table = "banned_hostmasks";

	HostmaskResponse ret = HOSTMASK_RESPONSE_NOROW;

	std::string query = "DELETE FROM "+table+" WHERE id = ?";

	sqlite3_stmt* stmt = 0;

	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_int(stmt, 1, id);
		if(sqlite3_step(stmt) == SQLITE_DONE)
		{
			ret = HOSTMASK_RESPONSE_OK;
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}

	sqlite3_finalize(stmt);

	return ret;
}

HostmaskResponse HostmaskAuthorizer::addHostmask(const std::string& nick, const std::string& hostmask, HostmaskType type)
{
	if(!_db)
		return HOSTMASK_RESPONSE_NODB;

	std::string table = "";
	if(type == HOSTMASK_AUTHORIZED)
		table = "authorized_hostmasks";
	else
		table = "banned_hostmasks";

	HostmaskResponse ret = HOSTMASK_RESPONSE_OK;

	std::string query = "INSERT INTO "+table+" (nick, hostmask) VALUES(?,?)";

	sqlite3_stmt* stmt = 0;
	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, nick.c_str(), nick.size(), SQLITE_STATIC);
		sqlite3_bind_text(stmt, 2, hostmask.c_str(), nick.size(), SQLITE_STATIC);

		if(sqlite3_step(stmt) != SQLITE_DONE)
		{
			ret = HOSTMASK_RESPONSE_BUSY;
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}

	sqlite3_finalize(stmt);

	return ret;
}

HostmaskResponse HostmaskAuthorizer::getHostmasksByNick(const std::string& nick, const HostmaskType& type, std::vector<std::string>& masks)
{
	if(!_db)
		return HOSTMASK_RESPONSE_NODB;

	std::string table;
	if(type == HOSTMASK_AUTHORIZED)
		table = "authorized_hostmasks";
	else
		table = "banned_hostmasks";

	std::string query = "SELECT id, hostmask FROM "+table+" WHERE nick = ?";

	sqlite3_stmt* stmt = 0;
	
	if(sqlite3_prepare(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, nick.c_str(), nick.size(), SQLITE_STATIC);
		while(sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::string pushme = std::string((char*)sqlite3_column_text(stmt,0)) + ") " + std::string((char*)sqlite3_column_text(stmt, 1));
			masks.push_back(pushme);
		}
	}
	else
	{
		std::cerr << "Error in query: " << query << std::endl;
	}

	sqlite3_finalize(stmt);
	return HOSTMASK_RESPONSE_OK;
}

bool HostmaskAuthorizer::isAuthorized(const std::string& host)
{
	if(!_db)
		return false;

	std::string query = "SELECT hostmask FROM authorized_hostmasks";
	
	sqlite3_stmt* stmt = 0;
	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		while(sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::string mask = std::string((char*) sqlite3_column_text(stmt, 0));
			if(MiscStringHelpers::stringContainsAllTokens(host, MiscStringHelpers::tokenizeString(mask, '*')))
			{
				return true;
			}
		}
	}
	else
	{
		std::cerr << "Error with query: " << query;
	}

	sqlite3_finalize(stmt);

	return false;
}

bool HostmaskAuthorizer::isBanned(const std::string& host)
{
	if(!_db)
		return false;

	std::string query = "SELECT hostmask FROM banned_hostmasks";
	
	sqlite3_stmt* stmt = 0;
	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		while(sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::string mask = std::string((char*) sqlite3_column_text(stmt, 0));
			if(MiscStringHelpers::stringContainsAllTokens(host, MiscStringHelpers::tokenizeString(mask, '*')))
			{
				return true;
			}
		}
	}

	sqlite3_finalize(stmt);
	return false;
}

}