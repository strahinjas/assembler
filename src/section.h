#ifndef _SECTION_H_
#define _SECTION_H_

#include <string>
#include <vector>
#include <iostream>

class Section {
public:
	Section();

	Section(const std::string& t_name, uint16_t t_entry, const std::string& t_flags);

	std::string getName() const {
		return this->name;
	}

	void write(int position, std::vector<uint8_t>& bytes);
	void writeValue(int position, size_t count, uint8_t value);

	std::string getBytes() const;

	void serialize(std::ostream& out) const;

	void deserialize(std::istream& in);

	friend class Loader;
	friend class Assembler;
	friend std::ostream& operator<<(std::ostream& out, const Section& section);
private:
	static uint16_t lastSectionTableEntry;

	uint16_t sectionTableEntry;

	std::string name;
	std::string flags;
	uint16_t symbolTableEntry;

	size_t size;
	std::vector<uint8_t> bytes;
};

#endif