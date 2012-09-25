#include "StringHelpers.h"

namespace IRCOptotron
{
	namespace MiscStringHelpers
	{
		// trim from start
		std::string &ltrim(std::string &s) {
				s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
				return s;
		}

		// trim from end
		std::string &rtrim(std::string &s) {
				s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
				return s;
		}

		// trim from both ends
		std::string &trim(std::string &s) {
				return ltrim(rtrim(s));
		}

		std::vector<std::string> tokenizeString(const std::string& s, const char& delimiter)
		{
			std::string token = "";
			std::vector<std::string> tokens;

			for(unsigned i = 0; i < s.length(); i++)
			{
				if(s.at(i) == delimiter)
				{
					if(token.length() > 0){
						tokens.push_back(token);
						token = "";
					}
				}
				else
				{
					token += s.at(i);
				}
			}

			if(token.length() > 0)
				tokens.push_back(token);

			return tokens;
		}

		std::string detokenizeString(const std::vector<std::string>& tokens, const char& combiner, unsigned start)
		{
			if(tokens.size() == 0 || tokens.size() < start+1)
				return "";

			std::string combined = tokens[start];
			for(unsigned i = start+1; i < tokens.size(); i++)
			{
				combined += combiner + tokens[i];
			}

			return combined;
		}

		bool stringContainsAllTokens(const std::string& haystack, const std::vector<std::string>& tokens)
		{
			for(size_t i = 0; i < tokens.size(); i++)
			{
				if(haystack.find(tokens[i]) == std::string::npos)
					return false;
			}

			return true;
		}
	}
}