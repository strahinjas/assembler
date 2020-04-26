#include <string>
#include <iomanip>
#include <iostream>

#include "types.h"
#include "relocation.h"


Relocation::Relocation() : offset(0), type(R_386_16) {}


Relocation::Relocation(const std::string& t_symbol,
					   const std::string& t_section,
					   int16_t t_offset,
					   RelocationType t_type) :
	symbol(t_symbol), section(t_section), offset(t_offset), type(t_type) {}


void Relocation::serialize(std::ostream& out) const {
	size_t size = symbol.size();
	out.write((char*)& size, sizeof(size_t));
	out.write(&symbol[0], size);

	size = section.size();
	out.write((char*)& size, sizeof(size_t));
	out.write(&section[0], size);

	out.write((char*)& offset, sizeof(int16_t));

	uint8_t enumType = type;
	out.write((char*)& enumType, sizeof(uint8_t));
}


void Relocation::deserialize(std::istream& in) {
	size_t size;

	in.read((char*)& size, sizeof(size_t));
	char* temp = new char[size];
	in.read(temp, size);
	symbol.assign(temp, size);
	delete[] temp;

	in.read((char*)& size, sizeof(size_t));
	temp = new char[size];
	in.read(temp, size);
	section.assign(temp, size);
	delete[] temp;

	in.read((char*)& offset, sizeof(int16_t));

	uint8_t enumType;
	in.read((char*)& enumType, sizeof(uint8_t));
	type = static_cast<RelocationType>(enumType);
}


std::ostream& operator<<(std::ostream& out, const Relocation& relocation) {
	out << std::left
		<< std::setw(WIDTH) << relocation.symbol
		<< std::setw(WIDTH) << relocation.section
		<< std::setw(WIDTH) << relocation.offset
		<< std::setw(WIDTH) << relocation.type;

	return out;
}