#include <iostream>
#include <cstring>
#include <exception>

#include "assembler.h"

int main(int argc, char* argv[]) {
	const uint8_t args = 4;
	const char* option = "-o";

	if (argc != args) {
		std::cerr << "ERROR: Wrong number of arguments!\n";
		std::cout << "Program should be called as: assembler -o output_file input_file.\n\n";
		return 1;
	}

	if (strcmp(argv[1], option)) {
		std::cerr << "ERROR: Unrecognized option \"" << argv[1] << "\"!\n";
		std::cout << "Program should be called as: assembler -o output_file input_file.\n\n";
		return 1;
	}

	std::string input  = argv[3];
	std::string output = argv[2];

	try {
		Assembler assembler;
		
		assembler.assemble(input, output);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl << std::endl;
		return 1;
	}

	return 0;
}