#include <string>
#include <iostream>
#include <iomanip>

#include "types.h"
#include "symbol.h"


uint16_t Symbol::lastSymbolTableEntry = 0;


Symbol::Symbol() :
	symbolTableEntry(0), value(0),
	scope(LOCAL), type(SymbolType::UNRESOLVED), defined(false) {}


Symbol::Symbol(const std::string& t_name,
			   const std::string& t_section,
			   int16_t t_value,
			   ScopeType t_scope,
			   SymbolType t_type,
			   bool t_defined) :
	symbolTableEntry(lastSymbolTableEntry++),
	name(t_name), section(t_section), value(t_value), scope(t_scope), type(t_type), defined(t_defined) {}


void Symbol::setData(const std::string& section, int16_t value, ScopeType scope, SymbolType type, bool defined) {
	this->section = section;
	this->value = value;
	this->scope = scope;
	this->type = type;
	this->defined = defined;
}


void Symbol::serialize(std::ostream& out) const {
	out.write((char*)& symbolTableEntry, sizeof(uint16_t));

	size_t size = name.size();
	out.write((char*)& size, sizeof(size_t));
	out.write(&name[0], size);

	size = section.size();
	out.write((char*)& size, sizeof(size_t));
	out.write(&section[0], size);

	out.write((char*)& value, sizeof(int16_t));

	uint8_t enumType = scope;
	out.write((char*)& enumType, sizeof(uint8_t));
	enumType = (uint8_t)type;
	out.write((char*)& enumType, sizeof(uint8_t));

	out.write((char*)& defined, sizeof(bool));
}


void Symbol::deserialize(std::istream& in) {
	in.read((char*)& symbolTableEntry, sizeof(uint16_t));

	size_t size;
	in.read((char*)& size, sizeof(size_t));
	char* temp = new char[size];
	in.read(temp, size);
	name.assign(temp, size);
	delete[] temp;

	in.read((char*)& size, sizeof(size_t));
	temp = new char[size];
	in.read(temp, size);
	section.assign(temp, size);
	delete[] temp;

	in.read((char*)& value, sizeof(int16_t));

	uint8_t enumType;
	in.read((char*)& enumType, sizeof(uint8_t));
	scope = static_cast<ScopeType>(enumType);
	in.read((char*)& enumType, sizeof(uint8_t));
	type = static_cast<SymbolType>(enumType);

	in.read((char*)& defined, sizeof(bool));
}


std::ostream& operator<<(std::ostream& out, const Symbol& symbol) {
	out << std::left
		<< std::setw(WIDTH) << symbol.symbolTableEntry
		<< std::setw(WIDTH) << symbol.name
		<< std::setw(WIDTH) << symbol.section
		<< std::setw(WIDTH) << (symbol.section == UNDEFINED ? UNDEFINED : std::to_string(symbol.value))
		<< std::setw(WIDTH) << symbol.scope
		<< std::setw(WIDTH) << symbol.type;

	return out;
}