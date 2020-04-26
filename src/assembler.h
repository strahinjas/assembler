#ifndef _ASSEMBLER_H_
#define _ASSEMBLER_H_

#include <fstream>
#include <queue>
#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>

#include "usymbol.h"
#include "types.h"
#include "symbol.h"
#include "section.h"
#include "instruction.h"
#include "relocation.h"

class Assembler {
public:
	~Assembler();

	void assemble(const std::string& input, std::string& output);
private:
	void readAssembly(const std::string& file);

	void firstPass();

	void resolveSymbols();
	bool hasCycle(const std::unordered_map<std::string, UnresolvedSymbol*>& UST);

	bool cycle(const std::string& symbol,
			   std::unordered_set<std::string>& visited,
			   std::unordered_set<std::string>& recursionStack);

	void secondPass();

	void writeELF(const std::string& file);

	void writeText(const std::string& file);

	void addSymbol(const std::string& name,
				   const std::string& section,
				   int16_t value,
				   ScopeType scope,
				   SymbolType type,
				   bool defined);

	void addSection(Section* section);

	void evaluate(const std::string& directive, const std::string& expression, Section* section);

	void evaluateEQU(const std::string& symbol, const std::string& expression, Section* section);

	void generateInstructionCode(Instruction* instruction, Section* section);
	
	uint32_t line;
	uint16_t locationCounter;

	std::queue<Instruction*> instructions;

	std::vector<std::vector<std::string>> assembly;

	std::unordered_map<std::string, Symbol*>  symbolTable;
	std::unordered_map<std::string, Section*> sectionTable;

	// Unresolved Symbol Table
	std::unordered_map<std::string, UnresolvedSymbol*> UST;

	std::vector<Relocation*> relocationTable;
};

#endif
