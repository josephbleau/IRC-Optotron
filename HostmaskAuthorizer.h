#pragma once

#include <string>
#include <vector>

#include <sqlite\sqlite3.h>

namespace IRCOptotron
{

enum HostmaskType
{
	HOSTMASK_BANNED,
	HOSTMASK_AUTHORIZED
};

enum HostmaskResponse
{
	HOSTMASK_RESPONSE_NODB,
	HOSTMASK_RESPONSE_OK,
	HOSTMASK_RESPONSE_NOROW,
	HOSTMASK_RESPONSE_BUSY
};

class HostmaskAuthorizer
{
private:
	sqlite3* _db;

public:
	HostmaskResponse removeHostmaskByID(const int& id, HostmaskType type);
	HostmaskResponse addHostmask(const std::string& nick, const std::string& mask, HostmaskType type);
	HostmaskResponse getHostmasksByNick(const std::string& nick, const HostmaskType& type, std::vector<std::string>& masks);

	bool isAuthorized(const std::string& host);
	bool isBanned(const std::string& host);

	HostmaskAuthorizer(std::string db_filename);
	~HostmaskAuthorizer();
};

}