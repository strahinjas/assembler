#include <vector>
#include <string>
#include <utility>
#include <regex>
#include <sstream>

#include "types.h"
#include "utils.h"

using namespace std;


pair<TokenType, regex> Utils::tokenMatcher[] = {
	{ GLOBAL_EXTERN,		regex("^\\.(global|extern)$") },
	{ LABEL,				regex("^([a-zA-Z_]\\w*):$") },
	{ SECTION,				regex("^\\.(text|data|bss|section)$") },
	{ SECTION_FLAGS,		regex("^\"([waxmsilgte]+)\"$") },
	{ DIRECTIVE,			regex("^\\.(equ|byte|word|align|skip)$") },
	{ INSTRUCTION,			regex("^(halt|int|jmp|jeq|jne|jgt|call|ret|iret)$") },
	{ INSTRUCTION,			regex("^(xchg|mov|add|sub|mul|div|cmp|not|and|or|xor|test|shl|shr|push|pop)(b|w)?$") },
	{ SYMBOL_IMMED,			regex("^&([a-zA-Z_]\\w*)$") },
	{ SYMBOL_PCREL,			regex("^\\$([a-zA-Z_]\\w*)$") },
	{ OPERAND_REG,			regex("^(pc|sp|psw|r[01234567])(h|l)?$") },
	{ OPERAND_REGIND,		regex("^\\[(pc|sp|psw|r[01234567])\\]$") },
	{ OPERAND_REGIND,		regex("^(pc|sp|psw|r[01234567])\\[0\\]$") },
	{ OPERAND_REGINDDISP,	regex("^(pc|sp|psw|r[01234567])\\[([0-9]+|0x[0-9abcdefABCDEF]+|[a-zA-Z_]\\w*)\\]$") },
	{ OPERAND_REGINDDISP,	regex("^\\[(pc|sp|psw|r[01234567])\\]([0-9]+|0x[0-9abcdefABCDEF]+|[a-zA-Z_]\\w*)$") },
	{ OPERAND_IMMED,		regex("^(-?[0-9]+|0x[0-9abcdefABCDEF]+)$") },
	{ OPERAND_MEMORY,		regex("^\\*([0-9]+|0x[0-9abcdefABCDEF]+)$") },
	{ SYMBOL,				regex("^([a-zA-Z_]\\w*)$") },
	{ SECTION_NAME,			regex("^(\\.?[a-zA-Z]\\w*)$") },
	{ EXPRESSION,			regex("^(\\w+)([\\+-])(\\w+)$") }
};


void Utils::split(const string& s, const char* delimiter, vector<string>& tokens) {
	size_t begin = s.find_first_not_of(delimiter);

	while (begin != string::npos) {
		size_t end = s.find_first_of(delimiter, begin);

		tokens.push_back(s.substr(begin, end - begin));

		begin = s.find_first_not_of(delimiter, end);
	}
}


TokenType Utils::getTokenType(const string& token, smatch& matches) {
	TokenType result = INVALID;

	for (const auto& entry : tokenMatcher) {
		if (regex_match(token, matches, entry.second)) {
			result = entry.first;
			break;
		}
	}

	return result;
}


bool Utils::isJump(const std::string& mnemonic) {
	return mnemonic == "jmp" || mnemonic == "jeq" ||
		   mnemonic == "jne" || mnemonic == "jgt" || mnemonic == "call";
}


bool Utils::isExpression(const std::string& token) {
	return regex_match(token, tokenMatcher[14].second) ||
		   regex_match(token, tokenMatcher[16].second) ||
		   regex_match(token, tokenMatcher[18].second);
}


void Utils::setFlags(string& flags, const string& match) {
	if (match.find('w') != string::npos) flags[W] = '1';
	if (match.find('a') != string::npos) flags[A] = '1';
	if (match.find('x') != string::npos) flags[X] = '1';
	if (match.find('m') != string::npos) flags[M] = '1';
	if (match.find('s') != string::npos) flags[S] = '1';
	if (match.find('i') != string::npos) flags[I] = '1';
	if (match.find('l') != string::npos) flags[L] = '1';
	if (match.find('g') != string::npos) flags[G] = '1';
	if (match.find('t') != string::npos) flags[T] = '1';
	if (match.find('e') != string::npos) flags[E] = '1';
}


string Utils::toHexString(int16_t number) {
	stringstream stream;

	if ((number & 0xFF00) == 0) stream << "0";

	stream << hex << number;
	return string(stream.str());
}