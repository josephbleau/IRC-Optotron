#include <iostream>

#include "CalcDB.h"
#include "StringHelpers.h"

namespace IRCOptotron
{

CalcDB::CalcDB(const std::string& db_filename)
{
	_db = 0;

	// Initialize sqlite calc db  
	if(sqlite3_open(db_filename.c_str(), &_db) != SQLITE_OK)
	{
		std::cerr << "Error loading calc database: " << sqlite3_errmsg(_db) << std::endl;
	}
	else 
	{
		std::cout << "Calc database opened. " << std::endl;
	}
}

CalcDB::~CalcDB()
{
	if(_db)
	{
		sqlite3_close(_db);
	}
}

CalcResponse CalcDB::getLatestVersionNumber(const std::string& keyword, int& version)
{
	if(!_db)
		return CALC_RESPONSE_NODB;

	std::string response;
	if(getCalc(keyword, response) == CALC_RESPONSE_NOCALC)
		return CALC_RESPONSE_NOCALC;

	std::string query = "SELECT MAX(version) FROM calcs WHERE keyword = ?";
	sqlite3_stmt* stmt = 0;

	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, keyword.c_str(), keyword.size(), SQLITE_STATIC);

		if(sqlite3_step(stmt) == SQLITE_ROW)
		{
			version = sqlite3_column_int(stmt, 0);	
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}

	sqlite3_finalize(stmt);

	return CALC_RESPONSE_VERSIONOK;
}

CalcResponse CalcDB::getWrapAroundVersion(const std::string& keyword, int version, char* str_version)
{
	if(!_db)
		return CALC_RESPONSE_NODB;
	
	int max_version = 0;
	
	CalcResponse ret = getLatestVersionNumber(keyword, max_version);
	
	itoa(max_version + version + 1, str_version, 10);

	if(max_version + version + 1 < 0)
		ret = CALC_RESPONSE_NOCALC;

	return ret;
}

CalcResponse CalcDB::apropos(const std::string& searchterm, std::string& response)
{
	if(!_db)
		return CALC_RESPONSE_NODB;

	CalcResponse ret = CALC_RESPONSE_NOSEARCHMATCHES;

	std::string term = "%"+searchterm+"%";
	std::string query = "SELECT keyword FROM calcs WHERE keyword LIKE ? or calc LIKE ? GROUP BY keyword ORDER BY keyword";
	
	sqlite3_stmt* stmt = 0;

	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, term.c_str(), term.size(), SQLITE_STATIC);
		sqlite3_bind_text(stmt, 2, term.c_str(), term.size(), SQLITE_STATIC);

		if(sqlite3_step(stmt) == SQLITE_ROW)
		{
			ret = CALC_RESPONSE_OK;
			response += std::string((char*) sqlite3_column_text(stmt, 0));
		}

		while(sqlite3_step(stmt) == SQLITE_ROW)
		{			
			response += ", " + std::string((char*) sqlite3_column_text(stmt, 0));
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}

	sqlite3_finalize(stmt);

	return ret;
}

CalcResponse CalcDB::apropos_all(const std::string& searchterm, std::string& response)
{
	if(!_db)
		return CALC_RESPONSE_NODB;

	CalcResponse ret = CALC_RESPONSE_NOSEARCHMATCHES;

	std::string term = "%"+searchterm+"%";
	std::string query = "SELECT keyword, version FROM calcs WHERE keyword LIKE ? or calc LIKE ? GROUP BY keyword, version ORDER BY keyword, version";
	
	sqlite3_stmt* stmt = 0;

	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, term.c_str(), term.size(), SQLITE_STATIC);
		sqlite3_bind_text(stmt, 2, term.c_str(), term.size(), SQLITE_STATIC);

		if(sqlite3_step(stmt) == SQLITE_ROW)
		{
			ret = CALC_RESPONSE_OK;
			response += "(v" + std::string((char*) sqlite3_column_text(stmt, 1)) + " " + std::string((char*) sqlite3_column_text(stmt, 0)) + ")";
		}

		while(sqlite3_step(stmt) == SQLITE_ROW)
		{			
			response += ", (v" + std::string((char*) sqlite3_column_text(stmt, 1)) + " " + std::string((char*) sqlite3_column_text(stmt, 0)) + ")";
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}

	sqlite3_finalize(stmt);

	return ret;
}

CalcResponse CalcDB::changeCalc(const std::string& keyword, const std::string& newcalc, const std::string& author)
{
	if(!_db)
		return CALC_RESPONSE_NODB;

	int latest_version = 0;
	char str_version[32];

	if(getLatestVersionNumber(keyword, latest_version) == CALC_RESPONSE_NOCALC)
		return CALC_RESPONSE_NOCALC;

	itoa(latest_version+1, str_version, 10);

	CalcResponse ret = CALC_RESPONSE_NOCALC;

	std::string calc = newcalc;
	calc = MiscStringHelpers::trim(calc);

	std::string query = "INSERT INTO calcs (calc, keyword, author, version, added) VALUES (?,?, ?,?, strftime(\"%Y-%m-%d %H:%M:%S\",\"now\"))";

	sqlite3_stmt* stmt = 0;
	
	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, calc.c_str(), calc.size(), SQLITE_STATIC);
		sqlite3_bind_text(stmt, 2, keyword.c_str(), keyword.size(), SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, author.c_str(), author.size(), SQLITE_STATIC);
		sqlite3_bind_int(stmt, 4, atoi(str_version));

		int step = sqlite3_step(stmt);
		if(step == SQLITE_DONE)
		{
			ret = CALC_RESPONSE_CALCCHANGED;
		}
		else if(step == SQLITE_BUSY)
		{
			ret = CALC_RESPONSE_DBBUSY;
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}

	sqlite3_finalize(stmt);

	return ret;
}

CalcResponse CalcDB::getCalc(const std::string& keyword, std::string& response)
{
	if(!_db)
		return CALC_RESPONSE_NODB;

	CalcResponse ret = CALC_RESPONSE_NOCALC;

	std::string query = "SELECT calc FROM calcs WHERE keyword = ? ORDER BY version DESC LIMIT 0,1";
	
	sqlite3_stmt* stmt = 0;

	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, keyword.c_str(), keyword.size(), SQLITE_STATIC);

		if(sqlite3_step(stmt) == SQLITE_ROW)
		{
			const unsigned char* calc = sqlite3_column_text(stmt, 0);
			response = std::string((char*) calc);
			ret = CALC_RESPONSE_OK;
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}

	sqlite3_finalize(stmt);
	return ret;
}

CalcResponse CalcDB::getCalc(const std::string& keyword, int version, std::string& response)
{
	if(!_db)
		return CALC_RESPONSE_NODB;

	CalcResponse ret = CALC_RESPONSE_NOCALC;
	
	char str_version[32];
	itoa(version, str_version, 10);

	if(version < 0)
	{
		if(getWrapAroundVersion(keyword, version, str_version) != CALC_RESPONSE_VERSIONOK)
		{
			return CALC_RESPONSE_NOCALC;
		}
	}

	std::string query = "SELECT calc FROM calcs WHERE keyword = ? AND version = ? LIMIT 0,1";

	sqlite3_stmt* stmt = 0;

	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1,  keyword.c_str(), keyword.size(), SQLITE_STATIC);
		sqlite3_bind_int(stmt, 2, atoi(str_version));

		if(sqlite3_step(stmt) == SQLITE_ROW)
		{
			const unsigned char* calc = sqlite3_column_text(stmt, 0);
			response = std::string((char*) calc);
			ret = CALC_RESPONSE_OK;
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}

	sqlite3_finalize(stmt);
	return ret;
}

CalcResponse CalcDB::getVersionInfo(const std::string& keyword, int version, std::string& response)
{
	if(!_db)
		return CALC_RESPONSE_NODB;

	CalcResponse ret = CALC_RESPONSE_NOCALC;

	char str_version[32];
	itoa(version, str_version, 10);

	if(version < 0)
	{
		if(getWrapAroundVersion(keyword, version, str_version) != CALC_RESPONSE_VERSIONOK)
		{
			return CALC_RESPONSE_NOCALC;
		}
	}

	std::string query = "SELECT author, added FROM calcs WHERE keyword = ? AND version = ?";
	sqlite3_stmt* stmt = 0;

	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, keyword.c_str(), keyword.size(), SQLITE_STATIC);
		sqlite3_bind_int(stmt, 2, atoi(str_version));

		if(sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::string author = std::string((char*)sqlite3_column_text(stmt, 0));
			std::string added = std::string((char*)sqlite3_column_text(stmt, 1));
			response = "Calc '" + keyword + "' changed at " + added + " by " + author;
			ret = CALC_RESPONSE_OK;
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}

	return ret;
}

CalcResponse CalcDB::makeCalc(const std::string& keyword, const std::string& newcalc, const std::string& author)
{
	if(!_db)
		return CALC_RESPONSE_NODB;

	std::string response;
	if(getCalc(keyword, response) == CALC_RESPONSE_OK)
		return CALC_RESPONSE_CALCALREADYEXISTS;

	CalcResponse ret = CALC_RESPONSE_CALCALREADYEXISTS;

	std::string calc = newcalc;
	calc = MiscStringHelpers::trim(calc);

	std::string query = "INSERT INTO calcs (calc, keyword, author, version, added) VALUES (?,?,?,'0', strftime(\"%Y-%m-%d %H:%M:%S\",\"now\"))";

	sqlite3_stmt* stmt = 0;
	
	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, calc.c_str(), calc.size(), SQLITE_STATIC);
		sqlite3_bind_text(stmt, 2, keyword.c_str(), keyword.size(), SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, author.c_str(), author.size(), SQLITE_STATIC);
		
		int step = sqlite3_step(stmt);
		
		if(step == SQLITE_DONE)
		{
			ret = CALC_RESPONSE_CALCCHANGED;
		}
		else if(step == SQLITE_BUSY)
		{
			ret = CALC_RESPONSE_DBBUSY;
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}

	sqlite3_finalize(stmt);

	return ret;
}

CalcResponse CalcDB::removeCalc(const std::string& keyword)
{
	if(!_db)
		return CALC_RESPONSE_NODB;

	CalcResponse ret = CALC_RESPONSE_NOCALC;

	// First we need to know the calc exists.
	std::string response;
	if(getCalc(keyword, response) == CALC_RESPONSE_NOCALC)
		return ret;

	std::string query = "DELETE FROM calcs WHERE keyword = ?";
	sqlite3_stmt* stmt = 0;

	if(sqlite3_prepare_v2(_db, query.c_str(), query.size(), &stmt, 0) == SQLITE_OK)
	{
		sqlite3_bind_text(stmt, 1, keyword.c_str(), keyword.size(), SQLITE_STATIC);

		if(sqlite3_step(stmt) == SQLITE_DONE)
		{
			ret = CALC_RESPONSE_OK;
		}
	}
	else
	{
		std::cerr << "Error with query: " << query << std::endl;
	}
	
	return ret;
}

}