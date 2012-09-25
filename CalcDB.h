#pragma once

#include <string>
#include <sqlite\sqlite3.h>

namespace IRCOptotron
{

enum CalcResponse
{
	CALC_RESPONSE_OK,
	CALC_RESPONSE_NOCALC,
	CALC_RESPONSE_VERSIONOK,
	CALC_RESPONSE_NOSEARCHMATCHES,
	CALC_RESPONSE_CALCCHANGED,
	CALC_RESPONSE_CALCALREADYEXISTS,
	CALC_RESPONSE_NODB,
	CALC_RESPONSE_DBBUSY
};

class CalcDB
{
private:
	sqlite3* _db;

	CalcResponse getLatestVersionNumber(const std::string& keyword, int& version);
	CalcResponse getWrapAroundVersion(const std::string& keyword, int version, char *str_version);

public:
	CalcResponse apropos(const std::string& searchterm, std::string& response);
	CalcResponse apropos_all(const std::string& searchterm, std::string& response);
	CalcResponse changeCalc(const std::string& keyword, const std::string& newcalc, const std::string& author);
	CalcResponse getCalc(const std::string& keyword, std::string& response);
	CalcResponse getCalc(const std::string& keyword, int version, std::string& response);
	CalcResponse getVersionInfo(const std::string& keyword, int version, std::string& response);
	CalcResponse makeCalc(const std::string& keyword, const std::string& newcalc, const std::string& author);
	CalcResponse removeCalc(const std::string& keyword);

	CalcDB(const std::string& db_filename);
	~CalcDB();
};

}