#ifndef _USYMBOL_H_
#define _USYMBOL_H_

#include <string>
#include <vector>
#include <utility>
#include <iostream>

#include "types.h"
#include "section.h"

class UnresolvedSymbol {
public:
	UnresolvedSymbol(const std::string& t_name, Section* t_section) : name(t_name), section(t_section) {}

	friend class Assembler;
private:
	std::string name;

	// pair.first  -> symbol name
	// pair.second -> type of operation ("+" or "-")
	std::vector<std::pair<std::string, std::string>> dependencies;

	Section* section;
};

#endif