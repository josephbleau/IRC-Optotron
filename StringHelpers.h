#pragma once

#include <algorithm>
#include <vector>
#include <string>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

namespace IRCOptotron
{
	namespace MiscStringHelpers
	{
		std::string &ltrim(std::string &s);
		std::string &rtrim(std::string &s);
		std::string &trim(std::string &s);
		std::vector<std::string> tokenizeString(const std::string& s, const char& delimiter);
		std::string detokenizeString(const std::vector<std::string>& tokens, const char& combiner, unsigned start = 0);
		bool stringContainsAllTokens(const std::string& haystack, const std::vector<std::string>& tokens);
	}
}