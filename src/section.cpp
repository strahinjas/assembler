#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "types.h"
#include "section.h"


uint16_t Section::lastSectionTableEntry = 0;


Section::Section() :
	sectionTableEntry(0),
	symbolTableEntry(0), size(0) {}


Section::Section(const std::string& t_name, uint16_t t_entry, const std::string& t_flags) :
	sectionTableEntry(lastSectionTableEntry++),
	name(t_name), symbolTableEntry(t_entry), flags(t_flags), size(0) {}


void Section::write(int position, std::vector<uint8_t>& bytes) {
	auto begin = this->bytes.begin();
	this->bytes.insert(begin + position, bytes.begin(), bytes.end());
}


void Section::writeValue(int position, size_t count, uint8_t value) {
	auto begin = bytes.begin();
	bytes.insert(begin + position, count, value);
}


std::string Section::getBytes() const {
	std::stringstream out;
	const uint8_t bytesPerLine = 16;

	for (int i = 0; i < size; i++) {
		if (i > 0 && i % bytesPerLine == 0) out << std::endl;

		out << std::right
			<< std::setfill('0')
			<< std::setw(2)
			<< std::hex << (uint32_t)bytes[i] << ' ';
	}
	out << std::endl;

	std::string result = out.str();
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);

	return result;
}


void Section::serialize(std::ostream& out) const {
	out.write((char*)& sectionTableEntry, sizeof(uint16_t));

	size_t size = name.size();
	out.write((char*)& size, sizeof(size_t));
	out.write(&name[0], size);

	size = flags.size();
	out.write((char*)& size, sizeof(size_t));
	out.write(&flags[0], size);

	out.write((char*)& symbolTableEntry, sizeof(uint16_t));

	out.write((char*)& this->size, sizeof(size_t));

	for (const uint8_t& byte : bytes)
		out.write((char*)& byte, sizeof(uint8_t));
}


void Section::deserialize(std::istream& in) {
	in.read((char*)& sectionTableEntry, sizeof(uint16_t));

	size_t size;
	in.read((char*)& size, sizeof(size_t));
	char* temp = new char[size];
	in.read(temp, size);
	name.assign(temp, size);
	delete[] temp;

	in.read((char*)& size, sizeof(size_t));
	temp = new char[size];
	in.read(temp, size);
	flags.assign(temp, size);
	delete[] temp;

	in.read((char*)& symbolTableEntry, sizeof(uint16_t));

	in.read((char*)& this->size, sizeof(size_t));

	for (int i = 0; i < this->size; i++) {
		uint8_t byte;
		in.read((char*)& byte, sizeof(uint8_t));
		bytes.push_back(byte);
	}
}


std::ostream& operator<<(std::ostream& out, const Section& section) {
	out << std::left
		<< std::setw(WIDTH) << section.sectionTableEntry
		<< std::setw(WIDTH) << section.name
		<< std::setw(WIDTH) << section.size
		<< std::setw(WIDTH) << section.flags
		<< std::setw(WIDTH) << section.symbolTableEntry;

	return out;
}