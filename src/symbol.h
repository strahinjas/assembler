#ifndef _SYMBOL_H_
#define _SYMBOL_H_

#include <string>
#include <iostream>

#include "types.h"
#include "section.h"

class Symbol {
public:
	Symbol();

	Symbol(const std::string& t_name,
		   const std::string& t_section,
		   int16_t t_value,
		   ScopeType t_scope,
		   SymbolType t_type,
		   bool t_defined);

	void setData(const std::string& section, int16_t value, ScopeType scope, SymbolType type, bool defined);

	void serialize(std::ostream& out) const;

	void deserialize(std::istream& in);

	friend class Loader;
	friend class Assembler;
	friend std::ostream& operator<<(std::ostream& out, const Symbol& symbol);
private:
	static uint16_t lastSymbolTableEntry;

	uint16_t symbolTableEntry;

	std::string name;

	std::string section;
	int16_t value;

	ScopeType scope;
	SymbolType type;

	bool defined;
};

#endif