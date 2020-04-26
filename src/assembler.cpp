#include <fstream>
#include <queue>
#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <exception>
#include <regex>
#include <cmath>
#include <cstdlib>
#include <iomanip>

#include "assembler.h"
#include "exceptions.h"
#include "types.h"
#include "utils.h"
#include "symbol.h"
#include "section.h"
#include "instruction.h"
#include "relocation.h"

using namespace std;


Assembler::~Assembler() {
	for (auto& entry : symbolTable)  delete entry.second;

	for (auto& entry : sectionTable) delete entry.second;

	for (auto& entry : UST) delete entry.second;

	for (auto& relocation : relocationTable) delete relocation;
}


void Assembler::assemble(const string& input, string& output) {
	readAssembly(input);

	firstPass();

	resolveSymbols();

	secondPass();

	writeELF(output);

	writeText(output.replace(output.size() - 1, 1, "txt"));

	cout << "Assembling finished successfully!\n\n";
}


void Assembler::readAssembly(const string& file) {
	if (!std::regex_search(file, assemblyFile))
		throw AssemblingException("Invalid input file type -> assembly file (.s) expected!");

	ifstream input(file, ifstream::in);

	if (!input.is_open())
		throw AssemblingException("Can't open file " + file + "!");

	string line;

	while (getline(input, line)) {
		line = line.substr(0, line.find('#'));

		vector<string> tokens;
		Utils::split(line, " ,\n\t", tokens);

		if (tokens.size() == 0) continue;

		if (tokens[0] == ".end") break;

		assembly.push_back(tokens);
	}
}


void Assembler::firstPass() {
	line = 0;

	bool labelDefined = false;

	Section* currentSection = nullptr;

	smatch matches;
	string currentToken;
	TokenType currentTokenType;

	for (const auto& tokens : assembly) {
		++line;

		queue<string> queue;
		for (const auto& token : tokens) queue.push(token);

		currentToken = queue.front();
		queue.pop();

		currentTokenType = Utils::getTokenType(currentToken, matches);

		if (currentTokenType == LABEL) {
			if (labelDefined)
				throw AssemblingException(line, "Double label definition!");

			labelDefined = true;
			string label = matches[1];

			if (!currentSection)
				throw AssemblingException(line, "Label \"" + label + "\" defined outside any section!");

			addSymbol(label, currentSection->name, locationCounter, LOCAL, SymbolType::LABEL, true);

			if (queue.empty()) continue;

			currentToken = queue.front();
			queue.pop();

			currentTokenType = Utils::getTokenType(currentToken, matches);
		}

		labelDefined = false;

		switch (currentTokenType) {
		case GLOBAL_EXTERN: {
			string directive = currentToken;

			if (queue.empty())
				throw AssemblingException(line, "Directive \"" + directive + "\" has no arguments!");

			while (!queue.empty()) {
				currentToken = queue.front();
				queue.pop();

				currentTokenType = Utils::getTokenType(currentToken, matches);

				if (currentTokenType != SYMBOL)
					throw AssemblingException(line, "Directives \".global/.extern\" expect symbol!");

				if (symbolTable.count(currentToken) &&
					symbolTable[currentToken]->defined) {
					if (directive == ".extern")
						throw AssemblingException(line, "Symbol \"" + currentToken + "\" defined in file but flaged as extern!");

					symbolTable[currentToken]->scope = GLOBAL;
				}
				else {
					addSymbol(currentToken, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
				}
			}

			break;
		}
		case LABEL:
			throw AssemblingException(line, "Double label definition!");

			break;
		case SECTION: {
			if (currentSection)
				currentSection->size = locationCounter;

			locationCounter = 0;

			bool flagsSet = false;

			string flags(10, '0');
			string name = matches[0];

			if (name == ".section") {
				if (queue.empty())
					throw AssemblingException(line, "Section name missing!");

				currentToken = queue.front();
				queue.pop();

				currentTokenType = Utils::getTokenType(currentToken, matches);

				if (currentTokenType != SYMBOL && currentTokenType != SECTION && currentTokenType != SECTION_NAME)
					throw AssemblingException(line, "Illegal section name!");

				name = matches[0];

				if (!queue.empty()) {
					currentToken = queue.front();
					queue.pop();

					currentTokenType = Utils::getTokenType(currentToken, matches);
					if (currentTokenType != SECTION_FLAGS)
						throw AssemblingException(line, "Illegal section flags!");

					Utils::setFlags(flags, matches[0]);
					flagsSet = true;
				}
			}

			if (!flagsSet) {
				if (name == ".text") {
					flags[A] = flags[X] = '1';
				}
				else if (name == ".data") {
					flags[A] = flags[W] = '1';
				}
				else if (name == ".bss") {
					flags[W] = '1';
				}
				else if (name == ".rodata") {
					flags[A] = '1';
				}
				else {
					flags[A] = flags[W] = flags[X] = '1';
				}
			}

			addSymbol(name, name, 0, LOCAL, SymbolType::SECTION, true);
			uint16_t entry = symbolTable[name]->symbolTableEntry;

			currentSection = new Section(name, entry, flags);

			addSection(currentSection);
			break;
		}
		case DIRECTIVE: {
			if (!currentSection)
				throw AssemblingException(line, "Directives are only allowed inside a section!");

			string directive = matches[0];

			if (directive == ".equ") {
				if (queue.empty())
					throw AssemblingException(line, "Directive \".equ\" expects symbol and expression!");

				currentToken = queue.front();
				queue.pop();

				currentTokenType = Utils::getTokenType(currentToken, matches);

				if (currentTokenType != SYMBOL)
					throw AssemblingException(line, "Directive \".equ\" expects symbol and expression!");

				if (queue.empty())
					throw AssemblingException(line, "Missing expression in \".equ\" directive!");

				string expression;
				while (!queue.empty()) {
					expression += queue.front();
					queue.pop();
				}

				evaluateEQU(currentToken, expression, currentSection);

				break;
			}
			else if (directive == ".align") {
				int alignment = 1;
				if (!queue.empty()) {
					currentToken = queue.front();
					queue.pop();

					currentTokenType = Utils::getTokenType(currentToken, matches);

					if (currentTokenType != OPERAND_IMMED)
						throw AssemblingException(line, "Directive .align needs immediate operand!");

					alignment = strtol(matches[0].str().c_str(), NULL, 0);
				}
				alignment = (int)pow(2, alignment);

				if (!(locationCounter % alignment == 0))
					locationCounter = locationCounter / alignment * alignment + alignment;

				break;
			}
			else if (directive == ".skip") {
				int bytes = 1;
				if (!queue.empty()) {
					currentToken = queue.front();
					queue.pop();

					currentTokenType = Utils::getTokenType(currentToken, matches);

					if (currentTokenType != OPERAND_IMMED)
						throw AssemblingException(line, "Directive .skip needs immediate operand!");

					bytes = strtol(matches[0].str().c_str(), NULL, 0);
				}
				locationCounter += bytes;

				if (!queue.empty()) {
					currentToken = queue.front();
					queue.pop();

					currentTokenType = Utils::getTokenType(currentToken, matches);

					if (currentTokenType != OPERAND_IMMED)
						throw AssemblingException(line, "Illegal fill value!");
				}

				break;
			}

			if (currentSection->flags[A] != '1')
				throw AssemblingException(line, "Memory initialization in BSS section!");

			if (queue.empty())
				throw AssemblingException(line, "Missing initial value(s)!");

			int bytes = 0;
			bool symbolPreceds = true;
			
			while (!queue.empty()) {
				if (Utils::isExpression(queue.front())) {
					if (symbolPreceds) ++bytes;
					else symbolPreceds = true;
				}
				else symbolPreceds = false;
				queue.pop();
			}

			if (directive == ".byte")
				locationCounter += bytes * BYTE;
			else if (directive == ".word")
				locationCounter += bytes * WORD;
			else
				throw AssemblingException(line, "Unexpected error!");

			break;
		}
		case INSTRUCTION: {
			if (!currentSection || currentSection->flags[X] != '1')
				throw AssemblingException(line, "Instruction declared outside an executable section!");

			Instruction* instruction = Instruction::extract(queue, matches, line);

			locationCounter += instruction->size;
			instructions.push(instruction);

			break;
		}
		default:
			throw AssemblingException(line, "Invalid token!");
			break;
		}

		if (!queue.empty())
			throw AssemblingException(line, "Only one directive/instruction is allowed per line!");
	}

	if (currentSection)
		currentSection->size = locationCounter;
}


void Assembler::resolveSymbols() {
	if (hasCycle(UST))
		throw AssemblingException(line, "Cyclic equivalence detected!");

	for (const auto& entry : UST) {
		bool defined = true;

		for (auto& relocation : entry.second->dependencies) {
			defined &= symbolTable[relocation.first]->defined;

			if (symbolTable[relocation.first]->defined) {
				if (relocation.second == "+")
					symbolTable[entry.first]->value += symbolTable[relocation.first]->value;
				else
					symbolTable[entry.first]->value -= symbolTable[relocation.first]->value;

				relocation.first = symbolTable[relocation.first]->section;
			}
		}

		symbolTable[entry.first]->defined = defined;
	}
}


bool Assembler::hasCycle(const unordered_map<string, UnresolvedSymbol*>& UST) {
	unordered_set<std::string> visited;
	unordered_set<std::string> recursionStack;

	for (const auto& entry : UST)
		if (cycle(entry.first, visited, recursionStack))
			return true;

	return false;
}


bool Assembler::cycle(const string& symbol,
					  unordered_set<string>& visited,
					  unordered_set<string>& recursionStack) {
	if (!UST.count(symbol))
		return false;

	if (!visited.count(symbol)) {
		visited.insert(symbol);
		recursionStack.insert(symbol);

		for (const auto& dependency : UST[symbol]->dependencies) {
			if (!visited.count(dependency.first) && cycle(dependency.first, visited, recursionStack))
				return true;
			else if (recursionStack.count(dependency.first))
				return true;
		}
	}
	recursionStack.erase(symbol);
	return false;
}


void Assembler::secondPass() {
	line = 0;

	Section* currentSection = nullptr;

	smatch matches;
	string currentToken;
	TokenType currentTokenType;

	for (const auto& tokens : assembly) {
		++line;

		queue<string> queue;
		for (const auto& token : tokens) queue.push(token);

		currentToken = queue.front();
		queue.pop();

		currentTokenType = Utils::getTokenType(currentToken, matches);

		if (currentTokenType == LABEL) {
			if (queue.empty()) continue;

			currentToken = queue.front();
			queue.pop();

			currentTokenType = Utils::getTokenType(currentToken, matches);
		}

		switch (currentTokenType) {
		case GLOBAL_EXTERN: {
			string directive = currentToken;

			if (queue.empty())
				throw AssemblingException(line, "Directive \"" + directive + "\" has no arguments!");

			while (!queue.empty()) {
				currentToken = queue.front();
				queue.pop();

				currentTokenType = Utils::getTokenType(currentToken, matches);

				if (currentTokenType != SYMBOL)
					throw AssemblingException(line, "Directives \".global/.extern\" expect symbol!");

				if (symbolTable.count(currentToken) &&
					symbolTable[currentToken]->defined) {
					if (directive == ".extern")
						throw AssemblingException(line, "Symbol \"" + currentToken + "\" defined in file but flaged as extern!");

					symbolTable[currentToken]->scope = GLOBAL;
				}
				else {
					if (directive == ".global")
						throw AssemblingException(line, "Symbol \"" + currentToken + "\" not defined in file but flaged as global!");

					addSymbol(currentToken, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
				}
			}

			break;
		}
		case SECTION:
			locationCounter = 0;

			if (currentToken == ".section") {
				currentToken = queue.front();
				queue.pop();
			}

			currentSection = sectionTable[currentToken];
			break;
		case DIRECTIVE: {
			if (!currentSection)
				throw AssemblingException(line, "Directives are only allowed inside a section!");

			string directive = matches[0];

			if (directive == ".equ") continue;

			if (directive == ".align") {
				int alignment = 1;
				if (!queue.empty()) {
					currentToken = queue.front();
					queue.pop();

					alignment = strtol(currentToken.c_str(), NULL, 0);
				}
				alignment = (int)pow(2, alignment);

				if (!(locationCounter % alignment == 0)) {
					uint16_t start = locationCounter;

					locationCounter = locationCounter / alignment * alignment + alignment;

					if (currentSection->flags[A] == '1')
						currentSection->writeValue(start, locationCounter - start, 0);
				}

				break;
			}
			else if (directive == ".skip") {
				int bytes = 1;
				if (!queue.empty()) {
					currentToken = queue.front();
					queue.pop();

					bytes = strtol(currentToken.c_str(), NULL, 0);
				}

				uint16_t start = locationCounter;
				locationCounter += bytes;

				int value = 0;

				if (!queue.empty()) {
					currentToken = queue.front();
					queue.pop();

					value = strtol(currentToken.c_str(), NULL, 0);
				}

				if (currentSection->flags[A] == '1')
					currentSection->writeValue(start, locationCounter - start, value);

				break;
			}

			while (!queue.empty()) {
				string expression;
				bool symbolPreceds = false;

				while (!queue.empty()) {
					if (Utils::isExpression(queue.front())) {
						if (symbolPreceds) {
							break;
						}
						symbolPreceds = true;
					}
					else symbolPreceds = false;
					expression += queue.front();
					queue.pop();
				}

				evaluate(directive, expression, currentSection);
			}

			break;
		}
		case INSTRUCTION: {
			Instruction* instruction = instructions.front();
			instructions.pop();

			generateInstructionCode(instruction, currentSection);

			locationCounter += instruction->size;
			delete instruction;

			break;
		}
		default:
			throw AssemblingException(line, "Invalid token!");
			break;
		}
	}
}


void Assembler::writeELF(const string& file) {
	if (!std::regex_search(file, objectFile))
		throw AssemblingException("Invalid output file type -> object file (.o) expected!");

	ofstream output(file, ofstream::out | ofstream::trunc | ofstream::binary);

	if (!output.is_open())
		throw AssemblingException("Can't open file " + file + "!");

	size_t size = symbolTable.size();
	output.write((char*)& size, sizeof(size_t));

	for (const auto& entry : symbolTable)
		entry.second->serialize(output);

	size = sectionTable.size();
	output.write((char*)& size, sizeof(size_t));

	for (const auto& entry : sectionTable)
		entry.second->serialize(output);

	size = relocationTable.size();
	output.write((char*)& size, sizeof(size_t));

	for (const auto& relocation : relocationTable)
		relocation->serialize(output);

	output.flush();
}


void Assembler::writeText(const string& file) {
	ofstream output(file, ofstream::out | ofstream::trunc);

	if (!output.is_open())
		throw AssemblingException("Can't open file " + file + "!");

	for (const auto& entry : sectionTable) {
		if (entry.second->bytes.empty()) continue;

		output << "/*** Section \"" + entry.first + "\" ***/\n\n";
		output << entry.second->getBytes() << endl;
	}

	output << "/*** Symbol Table ***/\n\n";
	output << left
		   << setw(WIDTH) << "Entry"
		   << setw(WIDTH) << "Name"
		   << setw(WIDTH) << "Section"
		   << setw(WIDTH) << "Value"
		   << setw(WIDTH) << "Scope"
		   << setw(WIDTH) << "Type" << endl;

	for (int i = 0; i < symbolTable.size(); i++) {
		for (const auto& entry : symbolTable)
			if (entry.second->symbolTableEntry == i) {
				output << *entry.second << endl;
				break;
			}
	}

	output << endl;
	output << "/*** Section Table ***/\n\n";
	output << left
		   << setw(WIDTH) << "Entry"
		   << setw(WIDTH) << "Name"
		   << setw(WIDTH) << "Size"
		   << setw(WIDTH) << "WAXMSILGTE"
		   << setw(WIDTH) << "SymbolTableEntry" << endl;

	for (int i = 0; i < sectionTable.size(); i++) {
		for (const auto& entry : sectionTable)
			if (entry.second->sectionTableEntry == i) {
				output << *entry.second << endl;
				break;
			}
	}

	if (!relocationTable.empty()) {
		output << endl;
		output << "/*** Relocation Table ***/\n\n";
		output << left
			<< setw(WIDTH) << "Symbol"
			<< setw(WIDTH) << "Section"
			<< setw(WIDTH) << "Offset"
			<< setw(WIDTH) << "Type" << endl;

		for (const auto& relocation : relocationTable)
			output << *relocation << endl;
	}

	output.flush();
}


void Assembler::addSymbol(const string& name,
						  const string& section,
						  int16_t value,
						  ScopeType scope,
						  SymbolType type,
						  bool defined) {
	if ((symbolTable.count(name) && symbolTable[name]->defined) || UST.count(name))
		throw AssemblingException(line, "Symbol \"" + name + "\" is already defined!");

	if (symbolTable.count(name)) {
		symbolTable[name]->setData(section, value, scope, type, defined);
	}
	else {
		Symbol* symbol = new Symbol(name, section, value, scope, type, defined);
		symbolTable.insert({ name, symbol });
	}
}


void Assembler::addSection(Section* section) {
	if (sectionTable.count(section->name))
		throw AssemblingException(line, "Section \"" + section->name + "\" is already defined!");

	sectionTable.insert({ section->name, section });
}


void Assembler::evaluate(const string& directive, const string& expression, Section* section) {
	smatch matches;
	TokenType type = Utils::getTokenType(expression, matches);

	int16_t value = 0;
	RelocationType relocationType = directive == ".byte" ? R_386_8 : R_386_16;

	switch (type) {
	case OPERAND_IMMED: {
		value = strtol(expression.c_str(), NULL, 0);

		break;
	}
	case SYMBOL: {
		string symbol = expression;

		if (UST.count(symbol)) {
			value = symbolTable[symbol]->value;

			for (const auto& relocation : UST[symbol]->dependencies) {
				RelocationType type = relocationType;

				if (relocation.second == "-")
					type = (RelocationType)(type + 1);

				relocationTable.push_back(new Relocation(relocation.first, section->name, locationCounter, type));
			}
		}
		else {
			if (symbolTable.count(symbol)) {
				if (symbolTable[symbol]->scope == LOCAL) {
					value = symbolTable[symbol]->value;

					if (symbolTable[symbol]->type == SymbolType::CONSTANT) break;

					symbol = symbolTable[symbol]->section;
				}
			}
			else {
				addSymbol(symbol, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
			}

			relocationTable.push_back(new Relocation(symbol, section->name, locationCounter, relocationType));
		}
		

		break;
	}
	case EXPRESSION: {
		smatch match;

		string first = matches[1];
		TokenType firstType = Utils::getTokenType(first, match);

		string operation = matches[2];

		string second = matches[3];
		TokenType secondType = Utils::getTokenType(second, match);

		switch (firstType) {
		case OPERAND_IMMED:
			switch (secondType) {
			case OPERAND_IMMED: {
				int16_t operand1 = strtol(first.c_str(),  NULL, 0);
				int16_t operand2 = strtol(second.c_str(), NULL, 0);

				if (operation == "+")
					value = operand1 + operand2;

				else if (operation == "-")
					value = operand1 = operand2;

				else
					throw AssemblingException(line, "Unexpected error!");
			}
			case SYMBOL: {
				value = strtol(first.c_str(), NULL, 0);

				if (operation == "-" && directive == ".byte")
					relocationType = R_386_SUB_8;

				else if (operation == "-")
					relocationType = R_386_SUB_16;

				if (UST.count(second)) {
					if (operation == "+")
						value += symbolTable[second]->value;
					else
						value -= symbolTable[second]->value;

					for (const auto& relocation : UST[second]->dependencies) {
						RelocationType type = relocationType;

						if (operation == "+" && relocation.second == "-")
							type = (RelocationType)(type + 1);

						else if (operation == "-" && relocation.second == "-")
							type = (RelocationType)(type - 1);

						relocationTable.push_back(new Relocation(relocation.first, section->name, locationCounter, type));
					}
				}
				else {
					if (symbolTable.count(second)) {
						if (symbolTable[second]->scope == LOCAL) {
							if (operation == "+")
								value += symbolTable[second]->value;
							else
								value -= symbolTable[second]->value;

							if (symbolTable[second]->type == SymbolType::CONSTANT) break;

							second = symbolTable[second]->section;
						}
					}
					else {
						addSymbol(second, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					relocationTable.push_back(new Relocation(second, section->name, locationCounter, relocationType));
				}

				break;
			}
			default:
				throw AssemblingException(line, "Invalid operand type in expression!");
				break;
			}

			break;
		case SYMBOL:
			switch (secondType) {
			case OPERAND_IMMED: {
				value = strtol(second.c_str(), NULL, 0);

				if (operation == "-") value = ~value;

				if (UST.count(first)) {
					value += symbolTable[first]->value;

					for (const auto& relocation : UST[first]->dependencies) {
						RelocationType type = relocationType;

						if (relocation.second == "-")
							type = (RelocationType)(type + 1);

						relocationTable.push_back(new Relocation(relocation.first, section->name, locationCounter, type));
					}
				}
				else {
					if (symbolTable.count(first)) {
						if (symbolTable[first]->scope == LOCAL) {
							value += symbolTable[first]->value;

							if (symbolTable[first]->type == SymbolType::CONSTANT) break;

							first = symbolTable[first]->section;
						}
					}
					else {
						addSymbol(first, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					relocationTable.push_back(new Relocation(first, section->name, locationCounter, relocationType));
				}

				break;
			}
			case SYMBOL: {
				Symbol* symbol1 = symbolTable.count(first)  ? symbolTable[first]  : nullptr;
				Symbol* symbol2 = symbolTable.count(second) ? symbolTable[second] : nullptr;

				if (symbol1 &&
					symbol2 &&
					!UST.count(first)  &&
					!UST.count(second) &&
					symbol1->section == symbol2->section &&
					operation == "-") {
					value = symbol1->value - symbol2->value;
					break;
				}

				if (UST.count(first)) {
					value += symbolTable[first]->value;

					for (const auto& relocation : UST[first]->dependencies) {
						RelocationType type = relocationType;

						if (relocation.second == "-")
							type = (RelocationType)(type + 1);

						relocationTable.push_back(new Relocation(relocation.first, section->name, locationCounter, type));
					}
				}
				else {
					bool constant = false;

					if (symbol1) {
						if (symbol1->scope == LOCAL) {
							value += symbol1->value;

							if (symbol1->type == SymbolType::CONSTANT)
								constant = true;

							first = symbol1->section;
						}
					}
					else {
						addSymbol(first, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					if (!constant)
						relocationTable.push_back(new Relocation(first, section->name, locationCounter, relocationType));
				}
				if (operation == "-" && directive == ".byte")
					relocationType = R_386_SUB_8;

				else if (operation == "-")
					relocationType = R_386_SUB_16;

				if (UST.count(second)) {
					if (operation == "+")
						value += symbolTable[second]->value;
					else
						value -= symbolTable[second]->value;

					for (const auto& relocation : UST[second]->dependencies) {
						RelocationType type = relocationType;

						if (operation == "+" && relocation.second == "-")
							type = (RelocationType)(type + 1);

						else if (operation == "-" && relocation.second == "-")
							type = (RelocationType)(type - 1);

						relocationTable.push_back(new Relocation(relocation.first, section->name, locationCounter, type));
					}
				}
				else {
					bool constant = false;

					if (symbol2) {
						if (symbol2->scope == LOCAL) {
							if (operation == "+")
								value += symbol2->value;
							else
								value -= symbol2->value;

							if (symbol2->type == SymbolType::CONSTANT)
								constant = true;

							second = symbol2->section;
						}
					}
					else {
						addSymbol(second, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					if (!constant)
						relocationTable.push_back(new Relocation(second, section->name, locationCounter, relocationType));
				}

				break;
			}
			default:
				throw AssemblingException(line, "Invalid operand type in expression!");
				break;
			}

			break;
		default:
			throw AssemblingException(line, "Invalid operand type in expression!");
			break;
		}

		break;
	}
	default:
		throw AssemblingException(line, "Invalid expression!");
		break;
	}

	uint8_t lower  =  value & 0x00FF;
	uint8_t higher = (value & 0xFF00) >> 8;

	if (directive == ".byte") {
		if (higher > 0)
			throw AssemblingException(line, "Byte sized initial value expected!");

		vector<uint8_t> bytes = { lower };
		section->write(locationCounter, bytes);
		locationCounter += BYTE;
	}
	else if (directive == ".word") {
		vector<uint8_t> bytes = { lower, higher };
		section->write(locationCounter, bytes);
		locationCounter += WORD;
	}
	else
		throw AssemblingException(line, "Unexpected error!");
}


void Assembler::evaluateEQU(const std::string& symbol, const std::string& expression, Section* section) {
	smatch matches;
	TokenType type = Utils::getTokenType(expression, matches);

	switch (type) {
	case OPERAND_IMMED: {
		int16_t value = strtol(expression.c_str(), NULL, 0);

		addSymbol(symbol, section->name, value, LOCAL, SymbolType::CONSTANT, true);

		break;
	}
	case SYMBOL: {
		int16_t value = 0;
		string source = expression;

		bool defined = false;

		if (UST.count(source)) {
			value = symbolTable[source]->value;
			defined = symbolTable[source]->defined;
		}
		else if (symbolTable.count(source)) {
			if (symbolTable[source]->scope == LOCAL) {
				value = symbolTable[source]->value;

				if (symbolTable[source]->type == SymbolType::CONSTANT) {
					addSymbol(symbol, section->name, value, LOCAL, SymbolType::CONSTANT, true);
					break;
				}

				source = symbolTable[source]->section;
				defined = true;
			}
		}
		else {
			addSymbol(source, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
		}

		addSymbol(symbol, section->name, value, LOCAL, SymbolType::ALIAS, defined);

		UST.insert({ symbol, new UnresolvedSymbol(symbol, section) });

		if (UST.count(source))
			UST[symbol]->dependencies = UST[source]->dependencies;
		else
			UST[symbol]->dependencies.push_back({ source, "+" });

		break;
	}
	case EXPRESSION: {
		smatch match;

		string first = matches[1];
		TokenType firstType = Utils::getTokenType(first, match);

		string operation = matches[2];

		string second = matches[3];
		TokenType secondType = Utils::getTokenType(second, match);

		switch (firstType) {
		case OPERAND_IMMED:
			switch (secondType) {
			case OPERAND_IMMED: {
				int16_t operand1 = strtol(first.c_str(),  NULL, 0);
				int16_t operand2 = strtol(second.c_str(), NULL, 0);

				if (operation == "+")
					operand1 += operand2;
				else
					operand1 -= operand2;

				addSymbol(symbol, section->name, operand1, LOCAL, SymbolType::CONSTANT, true);

				break;
			}
			case SYMBOL: {
				int16_t value = strtol(first.c_str(), NULL, 0);

				bool defined = false;

				if (UST.count(second)) {
					if (operation == "+")
						value += symbolTable[second]->value;
					else
						value -= symbolTable[second]->value;

					defined = symbolTable[second]->defined;
				}
				else if (symbolTable.count(second)) {
					if (symbolTable[second]->scope == LOCAL) {
						if (operation == "+")
							value += symbolTable[second]->value;
						else
							value -= symbolTable[second]->value;

						if (symbolTable[second]->type == SymbolType::CONSTANT) {
							addSymbol(symbol, section->name, value, LOCAL, SymbolType::CONSTANT, true);
							break;
						}

						second = symbolTable[second]->section;
						defined = true;
					}
				}
				else {
					addSymbol(second, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
				}

				addSymbol(symbol, section->name, value, LOCAL, SymbolType::ALIAS, defined);

				UST.insert({ symbol, new UnresolvedSymbol(symbol, section) });

				if (UST.count(second))
					UST[symbol]->dependencies = UST[second]->dependencies;
				else
					UST[symbol]->dependencies.push_back({ second, operation });

				break;
			}
			default:
				throw AssemblingException(line, "Invalid operand type in expression!");
				break;
			}

			break;
		case SYMBOL:
			switch (secondType) {
			case OPERAND_IMMED: {
				int16_t value = strtol(second.c_str(), NULL, 0);

				if (operation == "-") value = ~value;

				bool defined = false;

				if (UST.count(first)) {
					value += symbolTable[first]->value;
					defined = symbolTable[first]->defined;
				}
				else if (symbolTable.count(first)) {
					if (symbolTable[first]->scope == LOCAL) {
						value += symbolTable[first]->value;

						if (symbolTable[first]->type == SymbolType::CONSTANT) {
							addSymbol(symbol, section->name, value, LOCAL, SymbolType::CONSTANT, true);
							break;
						}

						first = symbolTable[first]->section;
						defined = true;
					}
				}
				else {
					addSymbol(first, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
				}

				addSymbol(symbol, section->name, value, LOCAL, SymbolType::ALIAS, defined);

				UST.insert({ symbol, new UnresolvedSymbol(symbol, section) });

				if (UST.count(first))
					UST[symbol]->dependencies = UST[first]->dependencies;
				else
					UST[symbol]->dependencies.push_back({ first, "+" });

				break;
			}
			case SYMBOL: {
				int16_t value = 0;

				Symbol* symbol1 = symbolTable.count(first)  ? symbolTable[first]  : nullptr;
				Symbol* symbol2 = symbolTable.count(second) ? symbolTable[second] : nullptr;

				if (symbol1 &&
					symbol2 &&
					!UST.count(first)  &&
					!UST.count(second) &&
					symbol1->section == symbol2->section &&
					operation == "-") {
					value = symbol1->value - symbol2->value;

					addSymbol(symbol, section->name, value, LOCAL, SymbolType::CONSTANT, true);

					break;
				}

				bool defined = false;
				bool constant1 = false;
				bool constant2 = false;

				if (symbol1) {
					if (UST.count(first)) {
						value += symbol1->value;
						defined = symbol1->defined;
					}
					else if (symbol1->scope == LOCAL) {
						value += symbol1->value;

						if (symbol1->type == SymbolType::CONSTANT)
							constant1 = true;

						first = symbol1->section;
						defined = true;
					}
				}
				else {
					addSymbol(first, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
				}

				if (symbol2) {
					if (UST.count(second)) {
						if (operation == "+")
							value += symbol2->value;
						else
							value -= symbol2->value;

						defined &= symbol2->defined;
					}
					else if (symbol2->scope == LOCAL) {
						if (operation == "+")
							value += symbol2->value;
						else
							value -= symbol2->value;

						if (symbol2->type == SymbolType::CONSTANT)
							constant2 = true;

						second = symbol2->section;
					}
				}
				else {
					addSymbol(second, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					defined = false;
				}

				addSymbol(symbol, section->name, value, LOCAL, SymbolType::ALIAS, defined);

				UST.insert({ symbol, new UnresolvedSymbol(symbol, section) });

				if (UST.count(first))
					UST[symbol]->dependencies = UST[first]->dependencies;
				else if (!constant1)
					UST[symbol]->dependencies.push_back({ first, "+" });

				if (UST.count(second))
					UST[symbol]->dependencies = UST[second]->dependencies;
				else if (!constant2)
					UST[symbol]->dependencies.push_back({ second, operation });

				break;
			}
			default:
				throw AssemblingException(line, "Invalid operand type in expression!");
				break;
			}

			break;
		default:
			throw AssemblingException(line, "Invalid operand type in expression!");
			break;
		}

		break;
	}
	default:
		throw AssemblingException(line, "Invalid expression!");
		break;
	}
}


void Assembler::generateInstructionCode(Instruction* instruction, Section* section) {
	vector<uint8_t> bytes;

	uint8_t byte = instruction->code << CODE_OFFSET | (instruction->operandSize - 1) << SIZE_OFFSET;
	bytes.push_back(byte);

	if (instruction->destination) {
		const int16_t offset = locationCounter + BYTE * 2;

		Instruction::Operand* destination = instruction->destination;

		byte = destination->addressing << ADDR_OFFSET;

		switch (destination->addressing) {
		case IMMED: {
			bytes.push_back(byte);

			if (destination->type == IMMED_VALUE) {
				int16_t value = strtol(destination->value.c_str(), NULL, 0);

				uint8_t lower  =  value & 0x00FF;
				uint8_t higher = (value & 0xFF00) >> 8;

				if (instruction->operandSize == BYTE) {
					if (higher > 0)
						throw AssemblingException(line, "Byte sized operand expected!");

					bytes.push_back(lower);
				}
				else {
					bytes.push_back(lower);
					bytes.push_back(higher);
				}
			}
			else if (destination->type == IMMED_SYMBOL) {
				string symbol = destination->value;
				int16_t value = 0;

				if (UST.count(symbol)) {
					value = symbolTable[symbol]->value;

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					if (instruction->operandSize == BYTE) {
						if (higher > 0)
							throw AssemblingException(line, "Byte sized operand expected!");

						bytes.push_back(lower);
					}
					else {
						bytes.push_back(lower);
						bytes.push_back(higher);
					}

					for (const auto& relocation : UST[symbol]->dependencies) {
						RelocationType type;

						if (relocation.second == "+") {
							if (instruction->operandSize == BYTE)
								type = R_386_8;
							else
								type = R_386_16;
						}
						else {
							if (instruction->operandSize == BYTE)
								type = R_386_SUB_8;
							else
								type = R_386_SUB_16;
						}

						relocationTable.push_back(new Relocation(relocation.first, section->name, offset, type));
					}
				}
				else {
					bool constant = false;

					if (symbolTable.count(symbol)) {
						if (symbolTable[symbol]->scope == LOCAL) {
							value = symbolTable[symbol]->value;

							if (symbolTable[symbol]->type == SymbolType::CONSTANT)
								constant = true;
							else
								symbol = symbolTable[symbol]->section;
						}
					}
					else {
						addSymbol(symbol, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					if (instruction->operandSize == BYTE) {
						if (higher > 0)
							throw AssemblingException(line, "Byte sized operand expected!");

						bytes.push_back(lower);

						if (!constant)
							relocationTable.push_back(new Relocation(symbol, section->name, offset, R_386_8));
					}
					else {
						bytes.push_back(lower);
						bytes.push_back(higher);

						if (!constant)
							relocationTable.push_back(new Relocation(symbol, section->name, offset, R_386_16));
					}
				}
			}
			else
				throw AssemblingException(line, "Unexpected error!");

			break;
		}
		case REG_DIR: {
			uint8_t registerNumber;
			if (destination->type == REGISTER)
				registerNumber = destination->value[1] - '0';
			else if (destination->type == PSW)
				registerNumber = PSW_CODE;
			else
				throw AssemblingException(line, "Unexpected error!");

			byte |= registerNumber << REGS_OFFSET;

			if (instruction->operandSize == BYTE &&
				destination->displacement == "h") byte |= 1;

			bytes.push_back(byte);

			break;
		}
		case REG_IND: {
			uint8_t registerNumber;
			if (destination->type == REGISTER)
				registerNumber = destination->value[1] - '0';
			else if (destination->type == PSW)
				registerNumber = PSW_CODE;
			else
				throw AssemblingException(line, "Unexpected error!");

			byte |= registerNumber << REGS_OFFSET;
			bytes.push_back(byte);

			break;
		}
		case REG_IND_8:
		case REG_IND_16: {
			uint8_t registerNumber;
			if (destination->value == "psw")
				registerNumber = PSW_CODE;
			else
				registerNumber = destination->value[1] - '0';

			byte |= registerNumber << REGS_OFFSET;
			bytes.push_back(byte);

			if (destination->type == DISPL_VALUE) {
				int16_t displacement = strtol(destination->displacement.c_str(), NULL, 0);

				uint8_t lower  =  displacement & 0x00FF;
				uint8_t higher = (displacement & 0xFF00) >> 8;

				if (destination->addressing == REG_IND_8)
					bytes.push_back(lower);
				else {
					bytes.push_back(lower);
					bytes.push_back(higher);
				}
			}
			else if (destination->type == DISPL_SYMBOL) {
				string symbol = destination->displacement;
				int16_t value = 0;

				if (UST.count(symbol)) {
					value = symbolTable[symbol]->value;

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					for (const auto& relocation : UST[symbol]->dependencies) {
						RelocationType type;

						if (relocation.second == "+")
							type = R_386_16;
						else
							type = R_386_SUB_16;

						relocationTable.push_back(new Relocation(relocation.first, section->name, offset, type));
					}
				}
				else {
					bool constant = false;

					if (symbolTable.count(symbol)) {
						if (symbolTable[symbol]->scope == LOCAL) {
							value = symbolTable[symbol]->value;

							if (symbolTable[symbol]->type == SymbolType::CONSTANT)
								constant = true;
							else
								symbol = symbolTable[symbol]->section;
						}
					}
					else {
						addSymbol(symbol, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					if (!constant)
						relocationTable.push_back(new Relocation(symbol, section->name, offset, R_386_16));
				}
			}
			else if (destination->type == PCRELATIVE) {
				string symbol = destination->displacement;
				int16_t value = offset - (locationCounter + instruction->size);

				if (UST.count(symbol)) {
					value += symbolTable[symbol]->value;
					const auto& relocations = UST[symbol]->dependencies;

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					for (int i = 0; i < relocations.size(); i++) {
						if (i == 0) {
							if (relocations[i].second == "+") {
								relocationTable.push_back(new Relocation(relocations[i].first, section->name, offset, R_386_PC16));
							}
							else {
								relocationTable.push_back(new Relocation(relocations[i].first, section->name, offset, R_386_SUB_PC16));
							}
						}
						else {
							RelocationType type;

							if (relocations[i].second == "+")
								type = R_386_16;
							else
								type = R_386_SUB_16;

							relocationTable.push_back(new Relocation(relocations[i].first, section->name, offset, type));
						}
					}
				}
				else {
					if (symbolTable.count(symbol)) {
						if (symbolTable[symbol]->scope == LOCAL) {
							value += symbolTable[symbol]->value;
							symbol = symbolTable[symbol]->section;
						}

						if (symbolTable[symbol]->type == SymbolType::CONSTANT)
							throw AssemblingException(line, "PC relative addressing of constant symbol!");
					}
					else {
						addSymbol(symbol, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					relocationTable.push_back(new Relocation(symbol, section->name, offset, R_386_PC16));
				}
			}
			else
				throw AssemblingException(line, "Unexpected error!");

			break;
		}
		case MEMORY: {
			bytes.push_back(byte);

			if (destination->type == MEMORY_VALUE) {
				uint16_t address = strtol(destination->value.c_str(), NULL, 0);

				uint8_t lower  =  address & 0x00FF;
				uint8_t higher = (address & 0xFF00) >> 8;

				bytes.push_back(lower);
				bytes.push_back(higher);
			}
			else if (destination->type == MEMORY_SYMBOL) {
				string symbol = destination->value;
				int16_t value = 0;

				if (UST.count(symbol)) {
					value = symbolTable[symbol]->value;

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					for (const auto& relocation : UST[symbol]->dependencies) {
						RelocationType type;

						if (relocation.second == "+")
							type = R_386_16;
						else
							type = R_386_SUB_16;

						relocationTable.push_back(new Relocation(relocation.first, section->name, offset, type));
					}
				}
				else {
					bool constant = false;

					if (symbolTable.count(symbol)) {
						if (symbolTable[symbol]->scope == LOCAL) {
							value = symbolTable[symbol]->value;

							if (symbolTable[symbol]->type == SymbolType::CONSTANT)
								constant = true;
							else
								symbol = symbolTable[symbol]->section;
						}
					}
					else {
						addSymbol(symbol, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					if (!constant)
						relocationTable.push_back(new Relocation(symbol, section->name, offset, R_386_16));
				}
			}
			else
				throw AssemblingException(line, "Unexpected error!");

			break;
		}
		default:
			throw AssemblingException(line, "Unexpected error!");
			break;
		}
	}
	
	if (instruction->source) {
		const int16_t offset = locationCounter + BYTE * 2 +
							  (instruction->destination ? instruction->destination->size : 0);

		Instruction::Operand* source = instruction->source;

		byte = source->addressing << ADDR_OFFSET;

		switch (source->addressing) {
		case IMMED: {
			bytes.push_back(byte);

			if (source->type == IMMED_VALUE) {
				int16_t value = strtol(source->value.c_str(), NULL, 0);

				uint8_t lower  =  value & 0x00FF;
				uint8_t higher = (value & 0xFF00) >> 8;

				if (instruction->operandSize == BYTE) {
					if (higher > 0)
						throw AssemblingException(line, "Byte sized operand expected!");

					bytes.push_back(lower);
				}
				else {
					bytes.push_back(lower);
					bytes.push_back(higher);
				}
			}
			else if (source->type == IMMED_SYMBOL) {
				string symbol = source->value;
				int16_t value = 0;

				if (UST.count(symbol)) {
					value = symbolTable[symbol]->value;

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					if (instruction->operandSize == BYTE) {
						if (higher > 0)
							throw AssemblingException(line, "Byte sized operand expected!");

						bytes.push_back(lower);
					}
					else {
						bytes.push_back(lower);
						bytes.push_back(higher);
					}

					for (const auto& relocation : UST[symbol]->dependencies) {
						RelocationType type;

						if (relocation.second == "+") {
							if (instruction->operandSize == BYTE)
								type = R_386_8;
							else
								type = R_386_16;
						}
						else {
							if (instruction->operandSize == BYTE)
								type = R_386_SUB_8;
							else
								type = R_386_SUB_16;
						}

						relocationTable.push_back(new Relocation(relocation.first, section->name, offset, type));
					}
				}
				else {
					bool constant = false;

					if (symbolTable.count(symbol)) {
						if (symbolTable[symbol]->scope == LOCAL) {
							value = symbolTable[symbol]->value;

							if (symbolTable[symbol]->type == SymbolType::CONSTANT)
								constant = true;
							else
								symbol = symbolTable[symbol]->section;
						}
					}
					else {
						addSymbol(symbol, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					uint8_t lower = value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					if (instruction->operandSize == BYTE) {
						if (higher > 0)
							throw AssemblingException(line, "Byte sized operand expected!");

						bytes.push_back(lower);

						if (!constant)
							relocationTable.push_back(new Relocation(symbol, section->name, offset, R_386_8));
					}
					else {
						bytes.push_back(lower);
						bytes.push_back(higher);

						if (!constant)
							relocationTable.push_back(new Relocation(symbol, section->name, offset, R_386_16));
					}
				}
			}
			else
				throw AssemblingException(line, "Unexpected error!");

			break;
		}
		case REG_DIR: {
			uint8_t registerNumber;

			if (source->type == REGISTER)
				registerNumber = source->value[1] - '0';
			else if (source->type == PSW)
				registerNumber = PSW_CODE;
			else
				throw AssemblingException(line, "Unexpected error!");

			byte |= registerNumber << REGS_OFFSET;

			if (instruction->operandSize == BYTE &&
				source->displacement == "h") byte |= 1;

			bytes.push_back(byte);

			break;
		}
		case REG_IND: {
			uint8_t registerNumber;

			if (source->type == REGISTER)
				registerNumber = source->value[1] - '0';
			else if (source->type == PSW)
				registerNumber = PSW_CODE;
			else
				throw AssemblingException(line, "Unexpected error!");

			byte |= registerNumber << REGS_OFFSET;
			bytes.push_back(byte);

			break;
		}
		case REG_IND_8:
		case REG_IND_16: {
			uint8_t registerNumber;

			if (source->value == "psw")
				registerNumber = PSW_CODE;
			else
				registerNumber = source->value[1] - '0';

			byte |= registerNumber << REGS_OFFSET;
			bytes.push_back(byte);

			if (source->type == DISPL_VALUE) {
				int16_t displacement = strtol(source->displacement.c_str(), NULL, 0);

				uint8_t lower  =  displacement & 0x00FF;
				uint8_t higher = (displacement & 0xFF00) >> 8;

				if (source->addressing == REG_IND_8)
					bytes.push_back(lower);
				else {
					bytes.push_back(lower);
					bytes.push_back(higher);
				}
			}
			else if (source->type == DISPL_SYMBOL) {
				string symbol = source->displacement;
				int16_t value = 0;

				if (UST.count(symbol)) {
					value = symbolTable[symbol]->value;

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					for (const auto& relocation : UST[symbol]->dependencies) {
						RelocationType type;

						if (relocation.second == "+")
							type = R_386_16;
						else
							type = R_386_SUB_16;

						relocationTable.push_back(new Relocation(relocation.first, section->name, offset, type));
					}
				}
				else {
					bool constant = false;

					if (symbolTable.count(symbol)) {
						if (symbolTable[symbol]->scope == LOCAL) {
							value = symbolTable[symbol]->value;

							if (symbolTable[symbol]->type == SymbolType::CONSTANT)
								constant = true;
							else
								symbol = symbolTable[symbol]->section;
						}
					}
					else {
						addSymbol(symbol, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					if (!constant)
						relocationTable.push_back(new Relocation(symbol, section->name, offset, R_386_16));
				}
			}
			else if (source->type == PCRELATIVE) {
				string symbol = source->displacement;
				int16_t value = offset - (locationCounter + instruction->size);

				if (UST.count(symbol)) {
					value += symbolTable[symbol]->value;
					const auto& relocations = UST[symbol]->dependencies;

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					for (int i = 0; i < relocations.size(); i++) {
						if (i == 0) {
							if (relocations[i].second == "+") {
								relocationTable.push_back(new Relocation(relocations[i].first, section->name, offset, R_386_PC16));
							}
							else {
								relocationTable.push_back(new Relocation(relocations[i].first, section->name, offset, R_386_SUB_PC16));
							}
						}
						else {
							RelocationType type;

							if (relocations[i].second == "+")
								type = R_386_16;
							else
								type = R_386_SUB_16;

							relocationTable.push_back(new Relocation(relocations[i].first, section->name, offset, type));
						}
					}
				}
				else {
					if (symbolTable.count(symbol)) {
						if (symbolTable[symbol]->scope == LOCAL) {
							value += symbolTable[symbol]->value;
							symbol = symbolTable[symbol]->section;
						}

						if (symbolTable[symbol]->type == SymbolType::CONSTANT)
							throw AssemblingException(line, "PC relative addressing of constant symbol!");
					}
					else {
						addSymbol(symbol, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					relocationTable.push_back(new Relocation(symbol, section->name, offset, R_386_PC16));
				}
			}
			else
				throw AssemblingException(line, "Unexpected error!");

			break;
		}
		case MEMORY: {
			bytes.push_back(byte);

			if (source->type == MEMORY_VALUE) {
				uint16_t address = strtol(source->value.c_str(), NULL, 0);

				uint8_t lower  =  address & 0x00FF;
				uint8_t higher = (address & 0xFF00) >> 8;

				bytes.push_back(lower);
				bytes.push_back(higher);
			}
			else if (source->type == MEMORY_SYMBOL) {
				string symbol = source->value;
				int16_t value = 0;

				if (UST.count(symbol)) {
					value = symbolTable[symbol]->value;

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					for (const auto& relocation : UST[symbol]->dependencies) {
						RelocationType type;

						if (relocation.second == "+")
							type = R_386_16;
						else
							type = R_386_SUB_16;

						relocationTable.push_back(new Relocation(relocation.first, section->name, offset, type));
					}
				}
				else {
					bool constant = false;

					if (symbolTable.count(symbol)) {
						if (symbolTable[symbol]->scope == LOCAL) {
							value = symbolTable[symbol]->value;

							if (symbolTable[symbol]->type == SymbolType::CONSTANT)
								constant = true;
							else
								symbol = symbolTable[symbol]->section;
						}
					}
					else {
						addSymbol(symbol, UNDEFINED, 0, GLOBAL, SymbolType::EXTERN, false);
					}

					uint8_t lower  =  value & 0x00FF;
					uint8_t higher = (value & 0xFF00) >> 8;

					bytes.push_back(lower);
					bytes.push_back(higher);

					if (!constant)
						relocationTable.push_back(new Relocation(symbol, section->name, offset, R_386_16));
				}
			}
			else
				throw AssemblingException(line, "Unexpected error!");

			break;
		}
		default:
			throw AssemblingException(line, "Unexpected error!");
			break;
		}
	}

	section->write(locationCounter, bytes);
}
