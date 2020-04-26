#ifndef _RELOCATION_H_
#define _RELOCATION_H_

#include <string>
#include <iostream>

#include "types.h"

class Relocation {
public:
	Relocation();

	Relocation(const std::string& t_symbol,
			   const std::string& t_section,
			   int16_t t_offset,
			   RelocationType t_type);

	void serialize(std::ostream& out) const;

	void deserialize(std::istream& in);

	friend class Loader;
	friend std::ostream& operator<<(std::ostream& out, const Relocation& relocation);
private:
	std::string symbol;

	std::string section;
	int16_t offset;

	RelocationType type;
};

#endif