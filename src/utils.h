#ifndef _UTILS_H_
#define _UTILS_H_

#include <vector>
#include <string>
#include <utility>
#include <regex>
#include <unordered_map>

#include "types.h"

class Utils {
public:
	static void split(const std::string& s, const char* delimiter, std::vector<std::string>& tokens);

	static TokenType getTokenType(const std::string& token, std::smatch& matches);

	static bool isJump(const std::string& mnemonic);
	static bool isExpression(const std::string& token);

	static void setFlags(std::string& flags, const std::string& match);

	static std::string toHexString(int16_t number);
private:
	static std::pair<TokenType, std::regex> tokenMatcher[];
};

#endif